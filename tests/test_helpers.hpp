#pragma once

#include <cstddef>
#include <excelnt.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace excelnt_test {

// A uniquely named file in the system temp directory that is removed when the
// object goes out of scope. The file itself is not created.
class TempFile {
 public:
  explicit TempFile(const std::string& suffix = ".csv") {
    static std::mt19937_64 rng{std::random_device{}()};

    path_ = std::filesystem::temp_directory_path() /
            ("excelnt_test_" + std::to_string(rng()) + suffix);
  }

  ~TempFile() {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;

  std::string path() const { return path_.string(); }

  bool exists() const { return std::filesystem::exists(path_); }

 private:
  std::filesystem::path path_;
};

// Writes the content byte for byte, so tests control the exact line endings.
inline void write_raw(const std::string& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  output << content;
}

// Reads a file in text mode, so platform line endings come back as '\n'.
inline std::string read_text(const std::string& path) {
  std::ifstream input(path);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

// Parses every record in the given string with detail::parse_record.
inline std::vector<std::vector<std::string>> parse_all(const std::string& csv) {
  std::istringstream input(csv);
  std::size_t row = 1;
  std::vector<std::vector<std::string>> records;

  for (;;) {
    auto record = excelnt::detail::parse_record(input, row);

    if (record.empty()) break;

    records.push_back(std::move(record));
  }

  return records;
}

inline std::string data_file(const std::string& name) {
  return std::string(EXCELNT_TEST_DATA_DIR) + "/" + name;
}

}  // namespace excelnt_test
