#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_helpers.hpp"

using namespace excelnt;
using Catch::Matchers::Message;
using excelnt_test::parse_all;
using Strings = std::vector<std::string>;

TEST_CASE("parse_record splits a simple record", "[unit][parse]") {
  std::istringstream input("a,b,c\n");
  std::size_t row = 1;

  CHECK(detail::parse_record(input, row) == Strings{"a", "b", "c"});
  CHECK(row == 2);
}

TEST_CASE("parse_record returns an empty record at end of input",
          "[unit][parse]") {
  std::istringstream input("");
  std::size_t row = 1;

  CHECK(detail::parse_record(input, row).empty());
  CHECK(row == 1);
}

TEST_CASE("parse_record reads consecutive records and counts rows",
          "[unit][parse]") {
  std::istringstream input("a,b\n1,2\n3,4\n");
  std::size_t row = 1;

  CHECK(detail::parse_record(input, row) == Strings{"a", "b"});
  CHECK(detail::parse_record(input, row) == Strings{"1", "2"});
  CHECK(detail::parse_record(input, row) == Strings{"3", "4"});
  CHECK(detail::parse_record(input, row).empty());
  CHECK(row == 4);
}

TEST_CASE("parse_record accepts a final record without a line break",
          "[unit][parse]") {
  CHECK(parse_all("a,b\n1,2") == std::vector<Strings>{{"a", "b"}, {"1", "2"}});
}

TEST_CASE("parse_record handles LF, CRLF and CR line endings",
          "[unit][parse]") {
  const std::vector<Strings> expected{{"a", "b"}, {"1", "2"}};

  CHECK(parse_all("a,b\n1,2\n") == expected);
  CHECK(parse_all("a,b\r\n1,2\r\n") == expected);
  CHECK(parse_all("a,b\r1,2\r") == expected);
}

TEST_CASE("parse_record trims whitespace around unquoted fields",
          "[unit][parse]") {
  CHECK(parse_all("  a ,\tb\t, c d \n") ==
        std::vector<Strings>{{"a", "b", "c d"}});
}

TEST_CASE("parse_record keeps empty fields", "[unit][parse]") {
  CHECK(parse_all("a,,b\n") == std::vector<Strings>{{"a", "", "b"}});
  CHECK(parse_all(",\n") == std::vector<Strings>{{"", ""}});
  CHECK(parse_all("\n") == std::vector<Strings>{{""}});
}

TEST_CASE("parse_record understands quoted fields", "[unit][parse]") {
  SECTION("commas inside quotes") {
    CHECK(parse_all("\"a,b\",c\n") == std::vector<Strings>{{"a,b", "c"}});
  }

  SECTION("doubled quotes become a single quote") {
    CHECK(parse_all("\"say \"\"hi\"\"\",x\n") ==
          std::vector<Strings>{{"say \"hi\"", "x"}});
  }

  SECTION("line breaks inside quotes are part of the field") {
    std::istringstream input("\"line 1\nline 2\",x\nnext,row\n");
    std::size_t row = 1;

    CHECK(detail::parse_record(input, row) == Strings{"line 1\nline 2", "x"});
    CHECK(row == 2);
    CHECK(detail::parse_record(input, row) == Strings{"next", "row"});
  }

  SECTION(
      "whitespace before the opening and after the closing quote is allowed") {
    CHECK(parse_all("  \"a\"  ,  \"b\"\t\n") ==
          std::vector<Strings>{{"a", "b"}});
  }

  SECTION("a quoted field may end the record with CRLF or CR") {
    CHECK(parse_all("\"a\"\r\n\"b\"\r\n") ==
          std::vector<Strings>{{"a"}, {"b"}});
    CHECK(parse_all("\"a\"\r\"b\"\r") == std::vector<Strings>{{"a"}, {"b"}});
  }

  SECTION("a quoted field may end the input") {
    CHECK(parse_all("x,\"y\"") == std::vector<Strings>{{"x", "y"}});
  }
}

TEST_CASE("parse_record passes UTF-8 bytes through unchanged",
          "[unit][parse]") {
  // "café,naïve" spelled out as bytes to avoid source-encoding issues.
  CHECK(parse_all("caf\xC3\xA9,na\xC3\xAFve\n") ==
        std::vector<Strings>{{"caf\xC3\xA9", "na\xC3\xAFve"}});
}

TEST_CASE("parse_record reports malformed quoting", "[unit][parse]") {
  SECTION("unterminated quoted field") {
    CHECK_THROWS_MATCHES(
        parse_all("\"abc"), std::runtime_error,
        Message("CSV error at row 1: unterminated quoted field"));
  }

  SECTION("unterminated quoted field on a later row") {
    CHECK_THROWS_MATCHES(
        parse_all("a\n\"b\n"), std::runtime_error,
        Message("CSV error at row 2: unterminated quoted field"));
  }

  SECTION("quote in the middle of an unquoted field") {
    CHECK_THROWS_MATCHES(parse_all("ab\"c\n"), std::runtime_error,
                         Message("CSV error at row 1, column 1: "
                                 "unexpected quote in unquoted field"));
  }

  SECTION("characters after a closing quote") {
    CHECK_THROWS_MATCHES(parse_all("x,\"a\"b\n"), std::runtime_error,
                         Message("CSV error at row 1, column 2: "
                                 "unexpected characters after closing quote"));
  }
}
