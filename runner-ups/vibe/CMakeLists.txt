cmake_minimum_required(VERSION 3.10)
project(ah_hell_aisle)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find raylib
find_path(RAYLIB_INCLUDE_DIR raylib.h PATHS /opt/homebrew/include /usr/local/include /usr/include)
find_library(RAYLIB_LIBRARY raylib PATHS /opt/homebrew/lib /usr/local/lib /usr/lib)

if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "raylib not found. Please install with: brew install raylib")
endif()

include_directories(${RAYLIB_INCLUDE_DIR})

# Add executable
add_executable(ah_hell_aisle src/main.cpp)

# Link raylib and required frameworks
target_link_libraries(ah_hell_aisle ${RAYLIB_LIBRARY})

# On macOS, link frameworks
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(OPENGL_LIBRARY OpenGL)
    
    if(COCOA_LIBRARY)
        target_link_libraries(ah_hell_aisle ${COCOA_LIBRARY})
    endif()
    if(IOKIT_LIBRARY)
        target_link_libraries(ah_hell_aisle ${IOKIT_LIBRARY})
    endif()
    if(COREVIDEO_LIBRARY)
        target_link_libraries(ah_hell_aisle ${COREVIDEO_LIBRARY})
    endif()
    if(OPENGL_LIBRARY)
        target_link_libraries(ah_hell_aisle ${OPENGL_LIBRARY})
    endif()
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
