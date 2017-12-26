cmake_minimum_required(VERSION 3.10)
project(my-battlesnake)

set(CMAKE_CXX_STANDARD 17)

add_executable(my-battlesnake main.cpp)
add_subdirectory (Simple-Web-Server)
target_include_directories (my-battlesnake PUBLIC Simple-Web-Server)
target_include_directories (my-battlesnake PUBLIC /usr/local/Cellar/nlohmann_json/3.0.0/include)
target_link_libraries (my-battlesnake simple-web-server)
