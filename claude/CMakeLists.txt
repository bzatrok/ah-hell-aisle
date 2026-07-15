cmake_minimum_required(VERSION 3.16)
project(ah_hell_aisle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# raylib comes from Homebrew. CMake does not always carry /opt/homebrew on its search
# path, so if the config package is not found straight away, ask brew where it lives.
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    find_program(BREW_EXECUTABLE brew)
    if(BREW_EXECUTABLE)
        execute_process(
            COMMAND ${BREW_EXECUTABLE} --prefix raylib
            OUTPUT_VARIABLE RAYLIB_BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        list(APPEND CMAKE_PREFIX_PATH "${RAYLIB_BREW_PREFIX}")
    endif()
    find_package(raylib REQUIRED)
endif()

add_executable(ah_hell_aisle
    src/main.cpp
    src/game.cpp
    src/world.cpp
    src/map.cpp
    src/player.cpp
    src/enemy.cpp
    src/pickup.cpp
    src/render.cpp
    src/hud.cpp
    src/assets.cpp
    src/audio.cpp
    src/web_input.cpp
    src/net.cpp
    src/web_net.cpp)

target_link_libraries(ah_hell_aisle PRIVATE raylib)
target_compile_options(ah_hell_aisle PRIVATE -Wall -Wextra)
