#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_helpers.hpp"

using namespace excelnt;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::Message;
using excelnt_test::TempFile;
using excelnt_test::write_raw;
using Strings = std::vector<std::string>;

namespace {

Table read_string(const std::string& content) {
  TempFile file;
  write_raw(file.path(), content);
  return read(file.path());
}

}  // namespace

TEST_CASE("read loads every column of a well-formed file", "[unit][read]") {
  const Table table = read_string(
      "id,name,score,active\n"
      "1,alice,9.5,true\n"
      "2,bob,7.25,false\n");

  CHECK(table.size() == 4);
  CHECK(table["id"].asInt() == std::vector<int>{1, 2});
  CHECK(table["name"].asString() == Strings{"alice", "bob"});
  CHECK(table["score"].asDouble() == std::vector<double>{9.5, 7.25});
  CHECK(table["active"].asBool() == std::vector<bool>{true, false});
}

TEST_CASE("read does not require a trailing line break", "[unit][read]") {
  const Table table = read_string("a,b\n1,2");

  CHECK(table["a"].asInt() == std::vector<int>{1});
  CHECK(table["b"].asInt() == std::vector<int>{2});
}

TEST_CASE("read accepts CRLF and CR line endings", "[unit][read]") {
  const std::string content =
      GENERATE(as<std::string>{}, "a,b\r\n1,2\r\n3,4\r\n", "a,b\r1,2\r3,4\r");

  const Table table = read_string(content);

  CHECK(table["a"].asInt() == std::vector<int>{1, 3});
  CHECK(table["b"].asInt() == std::vector<int>{2, 4});
}

TEST_CASE("read ignores empty lines at the end of the file", "[unit][read]") {
  const std::string content = GENERATE(
      as<std::string>{}, "a,b\n1,2\n3,4\n\n", "a,b\n1,2\n3,4\n\n\n\n",
      "a,b\n1,2\n3,4\n\n\n", "a,b\r\n1,2\r\n3,4\r\n\r\n\r\n",
      "a,b\r1,2\r3,4\r\r", "a,b\n1,2\n3,4\n   \n\t\n", "a,b\n1,2\n3,4\n\n\n  ");
  CAPTURE(content);

  const Table table = read_string(content);

  CHECK(table["a"].asInt() == std::vector<int>{1, 3});
  CHECK(table["b"].asInt() == std::vector<int>{2, 4});
}

TEST_CASE("read ignores empty lines at the end of a single-column file",
          "[unit][read]") {
  const Table table = read_string("a\n1\n2\n\n\n");

  CHECK(table["a"].asInt() == std::vector<int>{1, 2});
}

TEST_CASE("read treats a header followed by empty lines as having no data",
          "[unit][read][error]") {
  CHECK_THROWS_MATCHES(
      read_string("a,b\n\n\n"), std::runtime_error,
      Message("CSV error: file contains a header but no data rows"));
}

TEST_CASE("read trims whitespace around headers and values", "[unit][read]") {
  const Table table = read_string(
      "  first , second\t\n"
      "  x  ,\t y \n");

  REQUIRE(table.has("first"));
  REQUIRE(table.has("second"));
  CHECK(table["first"].asString() == Strings{"x"});
  CHECK(table["second"].asString() == Strings{"y"});
}

TEST_CASE("read handles quoted headers and values", "[unit][read]") {
  const Table table = read_string(
      "\"last, first\",quote,notes\n"
      "\"Doe, Jane\",\"\"\"hi\"\"\",\"two\nlines\"\n");

  REQUIRE(table.has("last, first"));
  CHECK(table["last, first"].asString() == Strings{"Doe, Jane"});
  CHECK(table["quote"].asString() == Strings{"\"hi\""});
  CHECK(table["notes"].asString() == Strings{"two\nlines"});
}

TEST_CASE("read reports a missing file", "[unit][read][error]") {
  TempFile file;
  REQUIRE_FALSE(file.exists());

  CHECK_THROWS_MATCHES(read(file.path()), std::runtime_error,
                       Message("could not open CSV file: " + file.path()));
}

TEST_CASE("read rejects structurally invalid files", "[unit][read][error]") {
  SECTION("empty file") {
    CHECK_THROWS_MATCHES(read_string(""), std::runtime_error,
                         Message("CSV error: file is empty"));
  }

  SECTION("header without data rows") {
    CHECK_THROWS_MATCHES(
        read_string("a,b\n"), std::runtime_error,
        Message("CSV error: file contains a header but no data rows"));
  }

  SECTION("empty column header") {
    CHECK_THROWS_MATCHES(
        read_string("a, ,c\n1,2,3\n"), std::runtime_error,
        Message("CSV error at row 1, column 2: empty column header"));
  }

  SECTION("duplicate column header") {
    CHECK_THROWS_MATCHES(read_string("a,b,a\n1,2,3\n"), std::runtime_error,
                         Message("CSV error at row 1, column 3: "
                                 "duplicate column header 'a'"));
  }

  SECTION("duplicate column header after trimming") {
    CHECK_THROWS_WITH(read_string("a, a\n1,2\n"),
                      ContainsSubstring("duplicate column header 'a'"));
  }
}

TEST_CASE("read rejects rows that do not match the header",
          "[unit][read][error]") {
  SECTION("too many fields") {
    CHECK_THROWS_MATCHES(
        read_string("a,b\n1,2\n1,2,3\n"), std::runtime_error,
        Message("CSV error at row 3: expected 2 columns, found 3"));
  }

  SECTION("too few fields") {
    CHECK_THROWS_MATCHES(
        read_string("a,b,c\n1,2\n"), std::runtime_error,
        Message("CSV error at row 2: expected 3 columns, found 2"));
  }

  SECTION("empty line between data rows") {
    CHECK_THROWS_MATCHES(read_string("a,b\n1,2\n\n3,4\n"), std::runtime_error,
                         Message("CSV error at row 3: empty line"));
  }

  SECTION("several empty lines between data rows report the first") {
    CHECK_THROWS_MATCHES(read_string("a,b\n1,2\n\n\n3,4\n"), std::runtime_error,
                         Message("CSV error at row 3: empty line"));
  }

  SECTION("empty line between rows of a single-column file") {
    CHECK_THROWS_MATCHES(read_string("a\n1\n\n2\n"), std::runtime_error,
                         Message("CSV error at row 3: empty line"));
  }

  SECTION("empty unquoted field") {
    CHECK_THROWS_MATCHES(read_string("a,b\n1,2\n3,\n"), std::runtime_error,
                         Message("CSV error at row 3, column 2: empty field"));
  }

  SECTION("empty quoted field") {
    CHECK_THROWS_MATCHES(read_string("a,b\n\"\",2\n"), std::runtime_error,
                         Message("CSV error at row 2, column 1: empty field"));
  }
}

TEST_CASE("read surfaces parse errors with their location",
          "[unit][read][error]") {
  CHECK_THROWS_MATCHES(
      read_string("a,b\n1,\"2\n"), std::runtime_error,
      Message("CSV error at row 2: unterminated quoted field"));

  CHECK_THROWS_MATCHES(read_string("a,b\n1,2x\"\n"), std::runtime_error,
                       Message("CSV error at row 2, column 2: "
                               "unexpected quote in unquoted field"));
}
