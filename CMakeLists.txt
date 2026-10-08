cmake_minimum_required(VERSION 3.21)

project(excelnt
    VERSION 0.1.0
    DESCRIPTION "Header-only C++ library for reading and writing CSV files"
    LANGUAGES CXX)

add_library(excelnt INTERFACE)
add_library(excelnt::excelnt ALIAS excelnt)

target_include_directories(excelnt INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_compile_features(excelnt INTERFACE cxx_std_11)

option(EXCELNT_BUILD_TESTS "Build the excelnt test suite" ${PROJECT_IS_TOP_LEVEL})

if(EXCELNT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
