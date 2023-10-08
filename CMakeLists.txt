cmake_minimum_required (VERSION 2.6)
project (ctinyjson_test C)
set(CMAKE_C_FLAGS "-std=c99")

add_library(ctinyjson ctinyjson.c)
add_executable(ctinyjson_test test.c)
target_link_libraries(ctinyjson_test ctinyjson)