# PARANOIA GDExtension — CMake build (alternative to SCons).
# Requires godot-cpp checked out at ./godot-cpp (matching the Godot version).
cmake_minimum_required(VERSION 3.20)
project(paranoia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(godot-cpp)

file(GLOB_RECURSE PARANOIA_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(paranoia SHARED ${PARANOIA_SOURCES})
target_include_directories(paranoia PRIVATE src src/core)
target_link_libraries(paranoia PRIVATE godot-cpp)

set_target_properties(paranoia PROPERTIES
    PREFIX "lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
