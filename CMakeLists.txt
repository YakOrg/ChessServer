cmake_minimum_required(VERSION 3.17)
project(untitled19 C)

set(CMAKE_C_STANDARD 99)



add_executable(untitled19 main.c ll.c ll.h)

find_library(pthread pthread)
target_link_libraries(untitled19 pthread)