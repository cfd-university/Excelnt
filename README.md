Exceln't (pronounced excellent); yet another CSV parser library in C++ no one has asked (or has use) for. But don't worry, the purpose of this library is not to be of actual use to anyone, it's sole purpose is to teach a fellow parser library author that a clean library interface is not just possible, but the goal!

In the process, I have accidentally come up with something that is actually useful, so I am distributing it as a header-only, MIT-licensed library so you can knock yourself out and parse CSV files until you loose the will to live.

## Installation

It's header-only, download the `src/excelnt.hpp` file and throw it into your project, include it in your source files, and it'll work. But, if you prefer the slightly more tedious approach, you can also install it with CMake's `FetchContent` if you must:

```CMake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    excelnt
    GIT_REPOSITORY https://github.com/cfd-university/Exceln-t.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(excelnt)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE excelnt::excelnt)
```

### Executing the test suite 

Both unit and system (end-to-end) tests are provided with this repository. You can execute them doing the following:

```
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

This section shows you how to work with this library. Its goal is to be as small as possible, to be as intuitive as possible, and to be as clean as possible. You will be the judge of whether I have achieved that goal.

In the following examples, I will just show the syntax for how to use the library. If you want to test it yourself, you will need to embed that into the following boilerplate C++ code:

```c++
#include <vector>
#include <string>
#include "excelnt.hpp"

int main() {

    // your code goes below this line

    return 0;
}
```

### Creating a CSV table

We start by defining a table that we can later write into a file:

```c++
excelnt::Table table;
```

### Writing data to the table

We can now append columns with data using the 4 supported datatypes like so:

```c++
// adding column with integer data
std::vector<int> ID = {1, 2, 3};
table["ID"] = ID;

// adding column with floating point data
std::vector<double> score = {82.0, 63.0, 48.0};
table["score"] = score;

// adding column with string data
std::vector<std::string> names = {"student_name_1", "student_name_2", "student_name_3"};
table["name"] = names;

// adding column with floating point data
std::vector<bool> pass = {true, true, false};
table["pass"] = pass;
```

### Writing a CSV file to disk

To write a CSV file from an ```excelnt::Table``` object, we call:

```c++
excelnt::Table table;

// populate table with data
// ...

excelnt::write("grades.csv", table);
```

### Reading a CSV file

To open a CSV file, we run:

```c++
const auto table = excelnt::read("grades.csv");
```

### Getting a column from a CSV file

Once a table has been opened, we can read any column using:

```c++
// reading integer columns
const auto IDs = table["ID"].asInt();

// reading floating point columns
const auto scores = table["score"].asDouble();

// reading string columns
const auto names = table["name"].asString();

// reading boolean columns
const auto pass = table["pass"].asBool();
```

### Broadcasting

`Excelnt` isn't a complicated parser, and the above covers 99% of the use cases. Don't expect fire breathing dragons, magic, or enhanced social status by using this library. It does, however, implement broadcasting (~~a term definitly not stolen from numpy~~).

If you have a CSV file like so:

```csv
year,price,product,in_stock
2024,19.99,Notebook,true
2024,24.50,Backpack,true
2024,7.25,Pen Set,true
```

The ```year``` and ```in_stock``` is the same in all three rows. We could write this CSV file now as:

```c++
std::vector<int> year = {2024, 2024, 2024};
table["year"] = year;

std::vector<double> price = {3.49, 79.99, 0.99};
table["price"] = price;

std::vector<std::string> product = {"Notebook", "Backpack", "Pen"};
table["product"] = product;

std::vector<bool> in_stock = {true, false, true};
table["in_stock"] = in_stock;
```

But, since both ```year``` and ```in_stock``` is the same, we could also simplify this to the following:

```c++
table["year"] = 2024;

std::vector<double> price = {3.49, 79.99, 0.99};
table["price"] = price;

std::vector<std::string> product = {"Notebook", "Backpack", "Pen"};
table["product"] = product;

table["in_stock"] = true;
```

If all the columns have the same value, we can simply write the scalar value and then ```excelnt``` will *broadcast* this value into all the other rows so we have type less and can spend more time doing other improtant stuff. See, ```excelnt``` really is *excellent (by design)*.