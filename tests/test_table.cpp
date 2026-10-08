#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <excelnt.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using namespace excelnt;
using Catch::Matchers::Message;
using Catch::Matchers::WithinRel;
using Strings = std::vector<std::string>;

TEST_CASE("Column can be constructed and appended to", "[unit][column]") {
  SECTION("default constructed column is empty") {
    CHECK(Column{}.values().empty());
  }

  SECTION("single value constructor") {
    CHECK(Column{std::string("x")}.values() == Strings{"x"});
  }

  SECTION("vector constructor") {
    CHECK(Column{Strings{"a", "b"}}.values() == Strings{"a", "b"});
  }

  SECTION("append adds to the end") {
    Column column;
    column.append("1");
    column.append("2");

    CHECK(column.values() == Strings{"1", "2"});
    CHECK(column.asString() == Strings{"1", "2"});
  }
}

TEST_CASE("Column converts its values to typed vectors", "[unit][column]") {
  CHECK(Column{Strings{"1", "-2", "30"}}.asInt() ==
        std::vector<int>{1, -2, 30});

  const auto doubles = Column{Strings{"1.5", "-0.25", "3"}}.asDouble();
  REQUIRE(doubles.size() == 3);
  CHECK_THAT(doubles[0], WithinRel(1.5));
  CHECK_THAT(doubles[1], WithinRel(-0.25));
  CHECK_THAT(doubles[2], WithinRel(3.0));

  CHECK(Column{Strings{"true", "False", "TRUE"}}.asBool() ==
        std::vector<bool>{true, false, true});

  CHECK(Column{}.asInt().empty());
}

TEST_CASE("Column conversions fail on any invalid value", "[unit][column]") {
  const Column column{Strings{"1", "two", "3"}};

  CHECK_THROWS_AS(column.asInt(), std::invalid_argument);
  CHECK_THROWS_AS(column.asDouble(), std::invalid_argument);
  CHECK_THROWS_AS(column.asBool(), std::invalid_argument);
  CHECK(column.asString() == Strings{"1", "two", "3"});
}

TEST_CASE("make_column turns scalars into single-value columns",
          "[unit][make_column]") {
  CHECK(make_column(5).values() == Strings{"5"});
  CHECK(make_column(-12L).values() == Strings{"-12"});
  CHECK(make_column(2.5).values() == Strings{std::to_string(2.5)});
  CHECK(make_column(true).values() == Strings{"true"});
  CHECK(make_column(false).values() == Strings{"false"});
  CHECK(make_column("text").values() == Strings{"text"});
  CHECK(make_column(std::string("text")).values() == Strings{"text"});

  const std::string lvalue = "lvalue";
  CHECK(make_column(lvalue).values() == Strings{"lvalue"});
}

TEST_CASE("make_column turns vectors into multi-value columns",
          "[unit][make_column]") {
  CHECK(make_column(std::vector<int>{1, 2, 3}).values() ==
        Strings{"1", "2", "3"});
  CHECK(make_column(std::vector<double>{0.5, 1.0}).values() ==
        Strings{std::to_string(0.5), std::to_string(1.0)});
  CHECK(make_column(std::vector<bool>{true, false}).values() ==
        Strings{"true", "false"});
  CHECK(make_column(Strings{"a", "b"}).values() == Strings{"a", "b"});
  CHECK(make_column(std::vector<const char*>{"x", "y"}).values() ==
        Strings{"x", "y"});

  const Strings lvalue{"l", "v"};
  CHECK(make_column(lvalue).values() == Strings{"l", "v"});
}

TEST_CASE("make_column rejects null C strings", "[unit][make_column]") {
  const char* null_string = nullptr;

  CHECK_THROWS_MATCHES(make_column(null_string), std::invalid_argument,
                       Message("cannot assign a null string"));
  CHECK_THROWS_MATCHES(make_column(std::vector<const char*>{"ok", nullptr}),
                       std::invalid_argument,
                       Message("cannot assign a null string"));
}

TEST_CASE("A default constructed Table is empty", "[unit][table]") {
  const Table table;

  CHECK(table.size() == 0);
  CHECK_FALSE(table.has("anything"));
}

TEST_CASE("Table assignment through a column proxy", "[unit][table]") {
  Table table;

  SECTION("scalar values") {
    table["i"] = 7;
    table["d"] = 1.25;
    table["b"] = true;
    table["s"] = "hello";

    CHECK(table.size() == 4);
    CHECK(table["i"].asInt() == std::vector<int>{7});
    CHECK_THAT(table["d"].asDouble().at(0), WithinRel(1.25));
    CHECK(table["b"].asBool() == std::vector<bool>{true});
    CHECK(table["s"].asString() == Strings{"hello"});
  }

  SECTION("vector values") {
    table["i"] = std::vector<int>{1, 2, 3};
    table["s"] = Strings{"a", "b", "c"};

    CHECK(table["i"].asInt() == std::vector<int>{1, 2, 3});
    CHECK(table["s"].asString() == Strings{"a", "b", "c"});
  }

  SECTION("reassigning replaces the column rather than adding one") {
    table["x"] = std::vector<int>{1, 2, 3};
    table["x"] = 9;

    CHECK(table.size() == 1);
    CHECK(table["x"].asInt() == std::vector<int>{9});
  }

  SECTION("a null C string is rejected") {
    const char* null_string = nullptr;
    CHECK_THROWS_AS(table["x"] = null_string, std::invalid_argument);
  }
}

TEST_CASE("Non-const Table::operator[] does not create columns on read",
          "[unit][table]") {
  Table table;
  table["present"] = std::vector<int>{1, 2};

  SECTION("every read accessor throws for a missing column") {
    CHECK_THROWS_MATCHES(table["missing"].asString(), std::out_of_range,
                         Message("column not found: missing"));
    CHECK_THROWS_MATCHES(table["missing"].asInt(), std::out_of_range,
                         Message("column not found: missing"));
    CHECK_THROWS_MATCHES(table["missing"].asDouble(), std::out_of_range,
                         Message("column not found: missing"));
    CHECK_THROWS_MATCHES(table["missing"].asBool(), std::out_of_range,
                         Message("column not found: missing"));
  }

  SECTION("a failed read leaves the table unchanged") {
    CHECK_THROWS(table["missing"].asInt());
    CHECK_FALSE(table.has("missing"));
    CHECK(table.size() == 1);
  }

  SECTION("taking a proxy without using it does not add a column") {
    (void)table["unused"];
    CHECK_FALSE(table.has("unused"));
    CHECK(table.size() == 1);
  }

  SECTION("an existing column can still be read") {
    CHECK(table["present"].asInt() == std::vector<int>{1, 2});
  }

  SECTION("assigning creates the column, after which it can be read") {
    table["missing"] = 3;
    CHECK(table.has("missing"));
    CHECK(table["missing"].asInt() == std::vector<int>{3});
  }
}

TEST_CASE("Const Table::operator[] looks up existing columns only",
          "[unit][table]") {
  Table table;
  table["present"] = std::vector<int>{4, 5};

  const Table& view = table;

  CHECK(view["present"].asInt() == std::vector<int>{4, 5});
  CHECK_THROWS_MATCHES(view["missing"], std::out_of_range,
                       Message("column not found: missing"));
  CHECK_FALSE(view.has("missing"));
}
