cmake_minimum_required(VERSION 3.17)
project(chess-server C)
set(CMAKE_C_STANDARD 99)

add_executable(chess-server main.c ll.c ll.h)

find_library(pthread pthread)
target_link_libraries(chess-server pthread)