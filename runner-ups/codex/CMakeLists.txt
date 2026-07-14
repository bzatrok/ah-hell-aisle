cmake_minimum_required(VERSION 3.16)
project(ah_hell_aisle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(raylib REQUIRED)

add_executable(ah_hell_aisle main.cpp)
target_link_libraries(ah_hell_aisle PRIVATE raylib)
