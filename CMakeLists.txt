cmake_minimum_required(VERSION 2.8.4)
project(simple_filesystem)
set(CMAKE_C_FLAGS -std=c99)

add_subdirectory(src)
add_subdirectory(tests)