#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <excelnt.hpp>
#include <stdexcept>
#include <string>

using namespace excelnt;
using Catch::Matchers::WithinRel;

TEST_CASE("trim removes leading and trailing whitespace", "[unit][detail]") {
  CHECK(detail::trim("abc") == "abc");
  CHECK(detail::trim("  abc  ") == "abc");
  CHECK(detail::trim("\t\r\n abc \n") == "abc");
  CHECK(detail::trim("  a  b  ") == "a  b");
  CHECK(detail::trim("") == "");
  CHECK(detail::trim(" \t ") == "");
}

TEST_CASE("error helpers format row and cell locations", "[unit][detail]") {
  CHECK(detail::row_error(3, "boom") == "CSV error at row 3: boom");
  CHECK(detail::cell_error(3, 2, "boom") ==
        "CSV error at row 3, column 2: boom");
}

TEST_CASE("csv_escape quotes only when necessary", "[unit][detail]") {
  SECTION("plain values are left untouched") {
    CHECK(detail::csv_escape("abc") == "abc");
    CHECK(detail::csv_escape("") == "");
    CHECK(detail::csv_escape("with space") == "with space");
    CHECK(detail::csv_escape("3.14") == "3.14");
  }

  SECTION("values containing a comma are quoted") {
    CHECK(detail::csv_escape("a,b") == "\"a,b\"");
  }

  SECTION("embedded quotes are doubled") {
    CHECK(detail::csv_escape("say \"hi\"") == "\"say \"\"hi\"\"\"");
    CHECK(detail::csv_escape("\"") == "\"\"\"\"");
  }

  SECTION("values containing line breaks are quoted") {
    CHECK(detail::csv_escape("a\nb") == "\"a\nb\"");
    CHECK(detail::csv_escape("a\rb") == "\"a\rb\"");
    CHECK(detail::csv_escape("a\r\nb") == "\"a\r\nb\"");
  }
}

TEST_CASE("is_integer recognises optionally signed digit strings",
          "[unit][detail]") {
  CHECK(detail::is_integer("0"));
  CHECK(detail::is_integer("42"));
  CHECK(detail::is_integer("-7"));
  CHECK(detail::is_integer("+7"));
  CHECK(detail::is_integer("007"));

  CHECK_FALSE(detail::is_integer(""));
  CHECK_FALSE(detail::is_integer("+"));
  CHECK_FALSE(detail::is_integer("-"));
  CHECK_FALSE(detail::is_integer("1.0"));
  CHECK_FALSE(detail::is_integer("1e3"));
  CHECK_FALSE(detail::is_integer("12a"));
  CHECK_FALSE(detail::is_integer("1 "));
  CHECK_FALSE(detail::is_integer("--1"));
}

TEST_CASE("is_double recognises complete floating-point strings",
          "[unit][detail]") {
  CHECK(detail::is_double("1.5"));
  CHECK(detail::is_double("-2.25"));
  CHECK(detail::is_double(".5"));
  CHECK(detail::is_double("1e10"));
  CHECK(detail::is_double("42"));

  CHECK_FALSE(detail::is_double(""));
  CHECK_FALSE(detail::is_double("abc"));
  CHECK_FALSE(detail::is_double("1.5x"));
  CHECK_FALSE(detail::is_double("1.5 "));
}

TEST_CASE("is_bool and to_bool accept three spellings each", "[unit][detail]") {
  for (const char* value : {"true", "TRUE", "True"}) {
    CAPTURE(value);
    CHECK(detail::is_bool(value));
    CHECK(detail::to_bool(value) == true);
  }

  for (const char* value : {"false", "FALSE", "False"}) {
    CAPTURE(value);
    CHECK(detail::is_bool(value));
    CHECK(detail::to_bool(value) == false);
  }

  for (const char* value : {"", "yes", "1", "0", "tRUE", " true"}) {
    CAPTURE(value);
    CHECK_FALSE(detail::is_bool(value));
    CHECK_THROWS_AS(detail::to_bool(value), std::invalid_argument);
  }
}

TEST_CASE("convert<std::string> returns the value unchanged",
          "[unit][detail]") {
  CHECK(detail::convert<std::string>("hello, world") == "hello, world");
  CHECK(detail::convert<std::string>("") == "");
}

TEST_CASE("convert<int> parses whole strings only", "[unit][detail]") {
  CHECK(detail::convert<int>("42") == 42);
  CHECK(detail::convert<int>("-5") == -5);
  CHECK(detail::convert<int>("+5") == 5);

  CHECK_THROWS_AS(detail::convert<int>("4.2"), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<int>("12abc"), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<int>("abc"), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<int>(""), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<int>("99999999999999999999"),
                  std::out_of_range);
}

TEST_CASE("convert<double> parses whole strings only", "[unit][detail]") {
  CHECK_THAT(detail::convert<double>("2.5"), WithinRel(2.5));
  CHECK_THAT(detail::convert<double>("-0.125"), WithinRel(-0.125));
  CHECK_THAT(detail::convert<double>("1e3"), WithinRel(1000.0));
  CHECK_THAT(detail::convert<double>("7"), WithinRel(7.0));

  CHECK_THROWS_AS(detail::convert<double>("abc"), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<double>("1.5x"), std::invalid_argument);
  CHECK_THROWS_AS(detail::convert<double>(""), std::invalid_argument);
}

TEST_CASE("convert<bool> delegates to to_bool", "[unit][detail]") {
  CHECK(detail::convert<bool>("True") == true);
  CHECK(detail::convert<bool>("FALSE") == false);
  CHECK_THROWS_AS(detail::convert<bool>("maybe"), std::invalid_argument);
}
