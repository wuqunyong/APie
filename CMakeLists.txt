# CMake initialisation.
cmake_minimum_required(VERSION 3.1)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")

# Project name
project(APie_Project)

# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_STANDARD 17)


message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CMAKE_VERBOSE_MAKEFILEON ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

message(STATUS "Set CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)


message(STATUS "APie_Project PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "APie_Project PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")


# -----------------------------------------------------------------------------
# ----- LibAPie LIBRARY ---------------------------------------------------------
# ----------------------------------------------------------------------------- 


# Build LibAPie.so
set(LIBAPIE_DIR "libapie")
add_subdirectory(${LIBAPIE_DIR})
add_subdirectory(server)


