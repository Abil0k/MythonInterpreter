cmake_minimum_required(VERSION 3.0.0)
project(Main VERSION 0.1.0)

add_executable(Main main.cpp lexer.cpp lexer_test_open.cpp runtime.cpp runtime_test.cpp parse.cpp parse_test.cpp statement.cpp statement_test.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++17 -O3 -Wall -g")