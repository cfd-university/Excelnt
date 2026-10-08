#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace excelnt {

namespace detail {

inline std::string trim(std::string value) {
  const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(),
                           [&](unsigned char c) { return !is_space(c); }));

  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [&](unsigned char c) { return !is_space(c); })
                  .base(),
              value.end());

  return value;
}

inline std::string row_error(std::size_t row, const std::string& message) {
  return "CSV error at row " + std::to_string(row) + ": " + message;
}

inline std::string cell_error(std::size_t row, std::size_t column,
                              const std::string& message) {
  return "CSV error at row " + std::to_string(row) + ", column " +
         std::to_string(column) + ": " + message;
}

// Parses one CSV record. A record may contain quoted fields and commas/newlines
// inside quoted fields. Newlines inside quoted fields are intentionally
// supported because they are part of standard CSV quoting semantics.
inline std::vector<std::string> parse_record(std::istream& input,
                                             std::size_t& row_number) {
  std::vector<std::string> fields;
  std::string field;
  bool quoted = false;
  bool quote_closed = false;
  bool have_anything = false;

  for (;;) {
    const int ch = input.get();

    if (ch == EOF) {
      if (!have_anything && field.empty() && fields.empty()) return {};

      if (quoted) {
        throw std::runtime_error(
            row_error(row_number, "unterminated quoted field"));
      }

      fields.push_back(trim(field));
      ++row_number;
      return fields;
    }

    have_anything = true;

    if (quoted) {
      if (ch == '"') {
        if (input.peek() == '"') {
          input.get();
          field += '"';
        } else {
          quoted = false;
          quote_closed = true;
        }
      } else {
        field += static_cast<char>(ch);
      }

      continue;
    }

    if (quote_closed) {
      if (ch == ',') {
        fields.push_back(trim(field));
        field.clear();
        quote_closed = false;
      } else if (ch == '\n') {
        fields.push_back(trim(field));
        ++row_number;
        return fields;
      } else if (ch == '\r') {
        if (input.peek() == '\n') input.get();

        fields.push_back(trim(field));
        ++row_number;
        return fields;
      } else if (!std::isspace(static_cast<unsigned char>(ch))) {
        throw std::runtime_error(
            cell_error(row_number, fields.size() + 1,
                       "unexpected characters after closing quote"));
      }

      continue;
    }

    if (ch == ',') {
      fields.push_back(trim(field));
      field.clear();
    } else if (ch == '"') {
      // Quotes are recognized as the beginning of a quoted field only
      // when they are the first non-whitespace character in the field.
      if (trim(field).empty()) {
        field.clear();
        quoted = true;
      } else {
        throw std::runtime_error(
            cell_error(row_number, fields.size() + 1,
                       "unexpected quote in unquoted field"));
      }
    } else if (ch == '\n') {
      fields.push_back(trim(field));
      ++row_number;
      return fields;
    } else if (ch == '\r') {
      if (input.peek() == '\n') input.get();

      fields.push_back(trim(field));
      ++row_number;
      return fields;
    } else {
      field += static_cast<char>(ch);
    }
  }
}

inline std::string csv_escape(const std::string& value) {
  bool quote = false;

  for (const char c : value) {
    if (c == ',' || c == '"' || c == '\n' || c == '\r') {
      quote = true;
      break;
    }
  }

  if (!quote) return value;

  std::string result;
  result.reserve(value.size() + 2);

  result += '"';

  for (const char c : value) {
    if (c == '"')
      result += "\"\"";
    else
      result += c;
  }

  result += '"';
  return result;
}

inline bool is_integer(const std::string& value) {
  if (value.empty()) return false;

  std::size_t i = 0;

  if (value[i] == '+' || value[i] == '-') ++i;

  if (i == value.size()) return false;

  for (; i < value.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
  }

  return true;
}

inline bool is_double(const std::string& value) {
  if (value.empty()) return false;

  char* end = nullptr;
  const char* begin = value.c_str();

  // strtod accepts integers too, so this is intentionally checked only
  // after is_integer() when choosing the final column type.
  std::strtod(begin, &end);

  return end == begin + value.size();
}

inline bool is_bool(const std::string& value) {
  return value == "true" || value == "false" || value == "TRUE" ||
         value == "FALSE" || value == "True" || value == "False";
}

inline bool to_bool(const std::string& value) {
  if (value == "true" || value == "TRUE" || value == "True") return true;

  if (value == "false" || value == "FALSE" || value == "False") return false;

  throw std::invalid_argument("invalid boolean value: " + value);
}

template <typename T>
T convert(const std::string& value) {
  static_assert(!std::is_same<T, T>::value,
                "Unsupported excelnt conversion type");
}

template <>
inline std::string convert<std::string>(const std::string& value) {
  return value;
}

template <>
inline int convert<int>(const std::string& value) {
  std::size_t position = 0;
  const int result = std::stoi(value, &position);

  if (position != value.size())
    throw std::invalid_argument("invalid integer value: " + value);

  return result;
}

template <>
inline double convert<double>(const std::string& value) {
  std::size_t position = 0;
  const double result = std::stod(value, &position);

  if (position != value.size())
    throw std::invalid_argument("invalid floating-point value: " + value);

  return result;
}

template <>
inline bool convert<bool>(const std::string& value) {
  return to_bool(value);
}

}  // namespace detail

class Table;

class Column {
 public:
  Column() = default;

  explicit Column(std::string value) : values_{std::move(value)} {}

  explicit Column(std::vector<std::string> values)
      : values_(std::move(values)) {}

  std::vector<std::string> asString() const { return values_; }

  std::vector<int> asInt() const {
    std::vector<int> result;
    result.reserve(values_.size());

    for (const auto& value : values_)
      result.push_back(detail::convert<int>(value));

    return result;
  }

  std::vector<double> asDouble() const {
    std::vector<double> result;
    result.reserve(values_.size());

    for (const auto& value : values_)
      result.push_back(detail::convert<double>(value));

    return result;
  }

  std::vector<bool> asBool() const {
    std::vector<bool> result;
    result.reserve(values_.size());

    for (const auto& value : values_)
      result.push_back(detail::convert<bool>(value));

    return result;
  }

  void append(const std::string& value) { values_.push_back(value); }

  const std::vector<std::string>& values() const noexcept { return values_; }

 private:
  std::vector<std::string> values_;

  friend class Table;
  friend Table read(const std::string& filename);
  friend void write(const std::string& filename, const Table& table);
};

class ColumnProxy {
 public:
  ColumnProxy(Table& table, std::string name)
      : table_(&table), name_(std::move(name)) {}

  template <typename T>
  ColumnProxy& operator=(T&& value);

  std::vector<std::string> asString() const;
  std::vector<int> asInt() const;
  std::vector<double> asDouble() const;
  std::vector<bool> asBool() const;

 private:
  Table* table_;
  std::string name_;
};

class Table {
 public:
  Table() = default;

  // The column is created on assignment through the proxy; reading a column
  // that does not exist throws std::out_of_range.
  ColumnProxy operator[](const std::string& name) {
    return ColumnProxy(*this, name);
  }

  const Column& operator[](const std::string& name) const { return get(name); }

  bool has(const std::string& name) const {
    return columns_.find(name) != columns_.end();
  }

  std::size_t size() const noexcept { return columns_.size(); }

 private:
  std::map<std::string, Column> columns_;
  std::vector<std::string> order_;

  Column& get(const std::string& name) {
    const auto it = columns_.find(name);

    if (it == columns_.end())
      throw std::out_of_range("column not found: " + name);

    return it->second;
  }

  const Column& get(const std::string& name) const {
    const auto it = columns_.find(name);

    if (it == columns_.end())
      throw std::out_of_range("column not found: " + name);

    return it->second;
  }

  void set(const std::string& name, Column column) {
    auto it = columns_.find(name);

    if (it == columns_.end()) {
      order_.push_back(name);
      columns_.emplace(name, std::move(column));
    } else {
      it->second = std::move(column);
    }
  }

  friend class ColumnProxy;
  friend Table read(const std::string& filename);
  friend void write(const std::string& filename, const Table& table);
};

// Internal representation used while constructing a Table for writing.
// It deliberately remains string-based so CSV serialization has one simple
// path.
template <typename T, typename std::enable_if<std::is_arithmetic<T>::value &&
                                                  !std::is_same<T, bool>::value,
                                              int>::type = 0>
Column make_column(T value) {
  return Column(std::to_string(value));
}

inline Column make_column(bool value) {
  return Column(value ? "true" : "false");
}

inline Column make_column(const char* value) {
  if (value == nullptr)
    throw std::invalid_argument("cannot assign a null string");

  return Column(std::string(value));
}

inline Column make_column(const std::string& value) { return Column(value); }

inline Column make_column(std::string&& value) {
  return Column(std::move(value));
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>::value &&
                                                  !std::is_same<T, bool>::value,
                                              int>::type = 0>
Column make_column(const std::vector<T>& values) {
  std::vector<std::string> strings;
  strings.reserve(values.size());

  for (const auto& value : values) strings.push_back(std::to_string(value));

  return Column(std::move(strings));
}

inline Column make_column(const std::vector<bool>& values) {
  std::vector<std::string> strings;
  strings.reserve(values.size());

  for (const bool value : values) strings.push_back(value ? "true" : "false");

  return Column(std::move(strings));
}

inline Column make_column(const std::vector<std::string>& values) {
  return Column(values);
}

inline Column make_column(std::vector<std::string>&& values) {
  return Column(std::move(values));
}

inline Column make_column(const std::vector<const char*>& values) {
  std::vector<std::string> strings;
  strings.reserve(values.size());

  for (const auto* value : values) {
    if (value == nullptr)
      throw std::invalid_argument("cannot assign a null string");

    strings.emplace_back(value);
  }

  return Column(std::move(strings));
}

inline Table read(const std::string& filename) {
  std::ifstream input(filename);

  if (!input) throw std::runtime_error("could not open CSV file: " + filename);

  Table table;

  std::size_t row_number = 1;
  auto headers = detail::parse_record(input, row_number);

  if (headers.empty()) throw std::runtime_error("CSV error: file is empty");

  for (std::size_t i = 0; i < headers.size(); ++i) {
    headers[i] = detail::trim(headers[i]);

    if (headers[i].empty()) {
      throw std::runtime_error("CSV error at row 1, column " +
                               std::to_string(i + 1) + ": empty column header");
    }

    if (table.has(headers[i])) {
      throw std::runtime_error(
          "CSV error at row 1, column " + std::to_string(i + 1) +
          ": duplicate column header '" + headers[i] + "'");
    }

    table[headers[i]] = std::vector<std::string>{};
  }

  bool have_data_row = false;

  // Empty lines are only allowed at the end of the file, so the first one is
  // remembered and reported only if another data row follows it.
  std::size_t empty_line_row = 0;

  for (;;) {
    const auto fields = detail::parse_record(input, row_number);

    if (fields.empty()) break;

    if (fields.size() == 1 && fields.front().empty()) {
      if (empty_line_row == 0) empty_line_row = row_number - 1;

      continue;
    }

    if (empty_line_row != 0)
      throw std::runtime_error(detail::row_error(empty_line_row, "empty line"));

    have_data_row = true;

    if (fields.size() != headers.size()) {
      throw std::runtime_error(
          "CSV error at row " + std::to_string(row_number - 1) + ": expected " +
          std::to_string(headers.size()) + " columns, found " +
          std::to_string(fields.size()));
    }

    for (std::size_t column = 0; column < fields.size(); ++column) {
      if (fields[column].empty()) {
        throw std::runtime_error(
            detail::cell_error(row_number - 1, column + 1, "empty field"));
      }

      auto& values = table.get(headers[column]).values_;
      values.push_back(fields[column]);
    }
  }

  if (!have_data_row)
    throw std::runtime_error(
        "CSV error: file contains a header but no data rows");

  return table;
}

inline void write(const std::string& filename, const Table& table) {
  if (table.columns_.empty())
    throw std::runtime_error("cannot write an empty table");

  std::size_t row_count = 1;
  bool found_vector = false;

  for (const auto& name : table.order_) {
    const auto& values = table.columns_.at(name).values_;

    if (values.empty())
      throw std::runtime_error("cannot write column '" + name +
                               "': empty column");

    if (values.size() > 1) {
      if (!found_vector) {
        row_count = values.size();
        found_vector = true;
      } else if (values.size() != row_count) {
        throw std::runtime_error("vector length mismatch: column '" + name +
                                 "' has length " +
                                 std::to_string(values.size()) + ", expected " +
                                 std::to_string(row_count));
      }
    }
  }

  std::ofstream output(filename);

  if (!output)
    throw std::runtime_error("could not open CSV file for writing: " +
                             filename);

  for (std::size_t column = 0; column < table.order_.size(); ++column) {
    if (column != 0) output << ',';

    output << detail::csv_escape(table.order_[column]);
  }

  output << '\n';

  for (std::size_t row = 0; row < row_count; ++row) {
    for (std::size_t column = 0; column < table.order_.size(); ++column) {
      if (column != 0) output << ',';

      const auto& name = table.order_[column];
      const auto& values = table.columns_.at(name).values_;

      const std::string& value =
          values.size() == 1 ? values.front() : values[row];

      if (value.empty()) {
        throw std::runtime_error("cannot write empty value at row " +
                                 std::to_string(row + 2) + ", column '" + name +
                                 "'");
      }

      output << detail::csv_escape(value);
    }

    output << '\n';
  }

  if (!output)
    throw std::runtime_error("error while writing CSV file: " + filename);
}

inline std::vector<std::string> ColumnProxy::asString() const {
  return table_->get(name_).asString();
}

inline std::vector<int> ColumnProxy::asInt() const {
  return table_->get(name_).asInt();
}

inline std::vector<double> ColumnProxy::asDouble() const {
  return table_->get(name_).asDouble();
}

inline std::vector<bool> ColumnProxy::asBool() const {
  return table_->get(name_).asBool();
}

template <typename T>
ColumnProxy& ColumnProxy::operator=(T&& value) {
  table_->set(name_, make_column(std::forward<T>(value)));
  return *this;
}

}  // namespace excelnt
