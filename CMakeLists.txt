cmake_minimum_required(VERSION 3.16)
project(http C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wconversion -Wextra -pedantic -fopenmp")

add_executable(http main.c server.c server.h)
