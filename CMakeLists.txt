cmake_minimum_required(VERSION 3.10)
project(my-battlesnake)

set(CMAKE_CXX_STANDARD 17)

add_executable(my-battlesnake main.cpp draw.cpp)
target_include_directories (my-battlesnake PUBLIC json/include)
target_include_directories (my-battlesnake PUBLIC cpp-httplib)
