cmake_minimum_required(VERSION 3.10)
project(battlesnake-recv)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(battlesnake-recv main.cpp)
target_include_directories (battlesnake-recv PUBLIC json/include)
target_include_directories (battlesnake-recv PUBLIC cpp-httplib)
target_include_directories (battlesnake-recv PUBLIC argh)
target_link_libraries(battlesnake-recv Threads::Threads)
