#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

#include "test_helpers.hpp"

using namespace excelnt;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using excelnt_test::data_file;
using excelnt_test::read_text;
using excelnt_test::TempFile;
using excelnt_test::write_raw;
using Strings = std::vector<std::string>;

TEST_CASE("A table survives a write/read round trip", "[e2e]") {
  const std::vector<int> ids{1, 2, 3};
  const std::vector<double> values{0.5, -2.25, 1000.125};
  const std::vector<bool> flags{true, false, true};
  const Strings labels{"plain", "comma, inside", "quote \"inside\""};
  const Strings notes{"multi\nline", "caf\xC3\xA9", "x"};

  Table original;
  original["id"] = ids;
  original["value"] = values;
  original["flag"] = flags;
  original["label"] = labels;
  original["notes"] = notes;
  original["source"] = "sensor-7";

  TempFile file;
  write(file.path(), original);
  const Table loaded = read(file.path());

  REQUIRE(loaded.size() == 6);
  CHECK(loaded["id"].asInt() == ids);
  CHECK(loaded["flag"].asBool() == flags);
  CHECK(loaded["label"].asString() == labels);
  CHECK(loaded["notes"].asString() == notes);
  CHECK(loaded["source"].asString() == Strings(3, "sensor-7"));

  const auto loaded_values = loaded["value"].asDouble();
  REQUIRE(loaded_values.size() == values.size());
  for (std::size_t i = 0; i < values.size(); ++i)
    CHECK_THAT(loaded_values[i], WithinRel(values[i]));
}

TEST_CASE("read followed by write reproduces the file", "[e2e]") {
  const std::string content =
      "zeta,alpha,\"a, b\"\n"
      "1,x,\"say \"\"hi\"\"\"\n"
      "2,y,\"two\nlines\"\n";

  TempFile source;
  write_raw(source.path(), content);

  TempFile copy;
  write(copy.path(), read(source.path()));

  CHECK(read_text(copy.path()) == content);
}

TEST_CASE("A real-world style file can be read, extended and written back",
          "[e2e]") {
  const Table inventory = read(data_file("inventory.csv"));

  REQUIRE(inventory.size() == 5);
  CHECK(inventory["sku"].asString() ==
        Strings{"A-100", "A-101", "B-200", "B-201"});
  CHECK(inventory["name"].asString() ==
        Strings{"Widget", "Gadget, large", "The \"Best\" Gizmo", "Sprocket"});
  CHECK(inventory["quantity"].asInt() == std::vector<int>{12, 3, 7, 0});
  CHECK(inventory["in_stock"].asBool() ==
        std::vector<bool>{true, false, true, false});

  const auto quantity = inventory["quantity"].asInt();
  const auto price = inventory["price"].asDouble();

  std::vector<double> stock_value;
  for (std::size_t i = 0; i < quantity.size(); ++i)
    stock_value.push_back(quantity[i] * price[i]);

  const double total =
      std::accumulate(stock_value.begin(), stock_value.end(), 0.0);
  CHECK_THAT(total, WithinRel(12 * 2.50 + 3 * 19.99 + 7 * 5.00));

  Table report;
  report["sku"] = inventory["sku"].asString();
  report["name"] = inventory["name"].asString();
  report["stock_value"] = stock_value;
  report["currency"] = "EUR";

  TempFile output;
  write(output.path(), report);

  const Table reloaded = read(output.path());
  CHECK(reloaded["sku"].asString() == inventory["sku"].asString());
  CHECK(reloaded["name"].asString() == inventory["name"].asString());
  CHECK(reloaded["currency"].asString() == Strings(4, "EUR"));

  const auto reloaded_value = reloaded["stock_value"].asDouble();
  REQUIRE(reloaded_value.size() == stock_value.size());
  for (std::size_t i = 0; i < stock_value.size(); ++i)
    CHECK_THAT(reloaded_value[i], WithinAbs(stock_value[i], 1e-6));
}

TEST_CASE("A file written with CRLF line endings is read and rewritten",
          "[e2e]") {
  TempFile source;
  write_raw(source.path(), "a,b\r\n1,\"x\r\ny\"\r\n2,z\r\n");

  const Table table = read(source.path());
  CHECK(table["a"].asInt() == std::vector<int>{1, 2});
  REQUIRE(table["b"].asString().size() == 2);
  CHECK(table["b"].asString()[1] == "z");

  TempFile copy;
  write(copy.path(), table);
  const Table reloaded = read(copy.path());

  CHECK(reloaded["a"].asInt() == table["a"].asInt());
  CHECK(reloaded["b"].asString() == table["b"].asString());
}

TEST_CASE("Large tables round trip without loss", "[e2e]") {
  constexpr int rows = 10000;

  std::vector<int> ids(rows);
  std::iota(ids.begin(), ids.end(), -rows / 2);

  Strings names;
  names.reserve(rows);
  for (int i = 0; i < rows; ++i)
    names.push_back("row " + std::to_string(i) + (i % 7 == 0 ? ", seven" : ""));

  Table table;
  table["id"] = ids;
  table["name"] = names;

  TempFile file;
  write(file.path(), table);
  const Table loaded = read(file.path());

  CHECK(loaded["id"].asInt() == ids);
  CHECK(loaded["name"].asString() == names);
}
