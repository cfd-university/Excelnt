#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_helpers.hpp"

using namespace excelnt;
using Catch::Matchers::Message;
using Catch::Matchers::StartsWith;
using excelnt_test::read_text;
using excelnt_test::TempFile;
using Strings = std::vector<std::string>;

namespace {

std::string write_string(const Table& table) {
  TempFile file;
  write(file.path(), table);
  return read_text(file.path());
}

}  // namespace

TEST_CASE("write emits a header and one line per row", "[unit][write]") {
  Table table;
  table["id"] = std::vector<int>{1, 2, 3};
  table["name"] = Strings{"a", "b", "c"};

  CHECK(write_string(table) ==
        "id,name\n"
        "1,a\n"
        "2,b\n"
        "3,c\n");
}

TEST_CASE("write keeps columns in insertion order", "[unit][write]") {
  Table table;
  table["zeta"] = 1;
  table["alpha"] = 2;
  table["mid"] = 3;

  CHECK(write_string(table) == "zeta,alpha,mid\n1,2,3\n");
}

TEST_CASE("write broadcasts single values to every row", "[unit][write]") {
  Table table;
  table["unit"] = "m";
  table["x"] = std::vector<int>{1, 2, 3};
  table["one"] = std::vector<int>{7};

  CHECK(write_string(table) ==
        "unit,x,one\n"
        "m,1,7\n"
        "m,2,7\n"
        "m,3,7\n");
}

TEST_CASE("write emits a single row when every column is a scalar",
          "[unit][write]") {
  Table table;
  table["a"] = 1;
  table["b"] = "two";

  CHECK(write_string(table) == "a,b\n1,two\n");
}

TEST_CASE("write formats typed values", "[unit][write]") {
  Table table;
  table["flag"] = std::vector<bool>{true, false};
  table["ratio"] = std::vector<double>{0.5, -1.0};

  CHECK(write_string(table) ==
        "flag,ratio\n"
        "true," +
            std::to_string(0.5) +
            "\n"
            "false," +
            std::to_string(-1.0) + "\n");
}

TEST_CASE("write escapes headers and values that need quoting",
          "[unit][write]") {
  Table table;
  table["last, first"] = Strings{"Doe, Jane", "say \"hi\""};
  table["notes"] = Strings{"plain", "two\nlines"};

  CHECK(write_string(table) ==
        "\"last, first\",notes\n"
        "\"Doe, Jane\",plain\n"
        "\"say \"\"hi\"\"\",\"two\nlines\"\n");
}

TEST_CASE("write overwrites an existing file", "[unit][write]") {
  TempFile file;

  Table first;
  first["a"] = std::vector<int>{1, 2, 3, 4, 5};
  write(file.path(), first);

  Table second;
  second["b"] = 9;
  write(file.path(), second);

  CHECK(read_text(file.path()) == "b\n9\n");
}

TEST_CASE("write rejects tables it cannot represent", "[unit][write][error]") {
  TempFile file;
  Table table;

  SECTION("empty table") {
    CHECK_THROWS_MATCHES(write(file.path(), table), std::runtime_error,
                         Message("cannot write an empty table"));
  }

  SECTION("column assigned an empty vector") {
    table["a"] = std::vector<int>{};

    CHECK_THROWS_MATCHES(write(file.path(), table), std::runtime_error,
                         Message("cannot write column 'a': empty column"));
  }

  SECTION("vector columns of different lengths") {
    table["a"] = std::vector<int>{1, 2, 3};
    table["b"] = std::vector<int>{1, 2};

    CHECK_THROWS_MATCHES(
        write(file.path(), table), std::runtime_error,
        Message("vector length mismatch: column 'b' has length 2, "
                "expected 3"));
  }

  // All of the above are detected before the output file is opened.
  CHECK_FALSE(file.exists());
}

TEST_CASE("write rejects empty values", "[unit][write][error]") {
  TempFile file;
  Table table;

  SECTION("empty value inside a vector") {
    table["a"] = Strings{"x", ""};

    CHECK_THROWS_MATCHES(
        write(file.path(), table), std::runtime_error,
        Message("cannot write empty value at row 3, column 'a'"));
  }

  SECTION("empty scalar value") {
    table["a"] = std::vector<int>{1, 2};
    table["b"] = "";

    CHECK_THROWS_MATCHES(
        write(file.path(), table), std::runtime_error,
        Message("cannot write empty value at row 2, column 'b'"));
  }
}

TEST_CASE("write reports a path that cannot be opened",
          "[unit][write][error]") {
  const auto path = (std::filesystem::temp_directory_path() /
                     "excelnt_test_missing_directory" / "out.csv")
                        .string();

  Table table;
  table["a"] = 1;

  CHECK_THROWS_WITH(write(path, table),
                    StartsWith("could not open CSV file for writing: "));
}
