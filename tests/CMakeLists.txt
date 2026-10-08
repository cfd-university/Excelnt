include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(Catch2)

add_executable(excelnt_tests
    test_detail.cpp
    test_parse_record.cpp
    test_table.cpp
    test_read.cpp
    test_write.cpp
    test_end_to_end.cpp)

target_link_libraries(excelnt_tests PRIVATE
    excelnt::excelnt
    Catch2::Catch2WithMain)

# The library itself only needs C++11; the tests use std::filesystem.
target_compile_features(excelnt_tests PRIVATE cxx_std_17)

target_compile_definitions(excelnt_tests PRIVATE
    EXCELNT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(MSVC)
    target_compile_options(excelnt_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(excelnt_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(excelnt_tests)
