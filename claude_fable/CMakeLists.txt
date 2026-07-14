cmake_minimum_required(VERSION 3.16)
project(ah_hell_aisle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib REQUIRED)

add_executable(ah_hell_aisle
  src/main.cpp
  src/assets.cpp
  src/level.cpp
  src/game.cpp
  src/render.cpp
  src/hud.cpp
)

target_link_libraries(ah_hell_aisle PRIVATE raylib)
target_compile_options(ah_hell_aisle PRIVATE -Wall -Wextra)
