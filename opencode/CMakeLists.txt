cmake_minimum_required(VERSION 3.10)
project(AH_HellAisle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find raylib
find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED raylib)

# Create executable
add_executable(ah_hell_aisle
    src/main.cpp
    src/engine.cpp
    src/player.cpp
    src/map.cpp
    src/enemy.cpp
    src/weapon.cpp
    src/hud.cpp
)

# Include directories
target_include_directories(ah_hell_aisle PRIVATE 
    ${RAYLIB_INCLUDE_DIRS}
    src/
)

# Link libraries
target_link_libraries(ah_hell_aisle 
    ${RAYLIB_LIBRARIES}
    m
)