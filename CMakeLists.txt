# CMake initialisation.
cmake_minimum_required(VERSION 3.1)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

# Project name
project(APie_Project)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CMAKE_VERBOSE_MAKEFILEON ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

message(STATUS "Set CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

message(STATUS "APie_Project PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "APie_Project PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")


# -----------------------------------------------------------------------------
# ----- LibAPie LIBRARY ---------------------------------------------------------
# ----------------------------------------------------------------------------- 


# Build LibAPie.so
set(LIBAPIE_DIR "LibAPie")
add_subdirectory(${LIBAPIE_DIR})
add_subdirectory("ServiceRegistry")


