# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(Backend LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all .cpp files
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")

# Include header files
include_directories(${CMAKE_SOURCE_DIR})

# Add executable
add_executable(backend ${SOURCES})

