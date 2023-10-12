cmake_minimum_required (VERSION 2.6)
project (ctinyjson_test C)
set(CMAKE_C_FLAGS "-std=c11")

# add_library(ctinyjson ctinyjson.c)
add_executable(ctinyjson_test  test.c  ctinyjson.c)
# target_link_libraries(ctinyjson_test ctinyjson)
set_target_properties(ctinyjson_test PROPERTIES COMPILE_FLAGS "-g")