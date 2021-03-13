cmake_minimum_required(VERSION 3.12.4)
project(chess-server C)
set(CMAKE_C_STANDARD 99)

add_executable(chess-server src/main.c src/ll.c src/ll.h)

find_library(pthread pthread)
target_link_libraries(chess-server pthread)

find_library(sodium sodium.a)
target_link_libraries(chess-server -static sodium)
