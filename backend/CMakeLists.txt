set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(Backend LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMONGOCXX REQUIRED libmongocxx)

include_directories(${LIBMONGOCXX_INCLUDE_DIRS})
link_directories(${LIBMONGOCXX_LIBRARY_DIRS})

# Collect all .cpp files
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")

# Include header files
include_directories(${CMAKE_SOURCE_DIR})

# Add executable
add_executable(backend ${SOURCES})

target_link_libraries(backend ${LIBMONGOCXX_LIBRARIES})

target_compile_options(backend PRIVATE ${LIBMONGOCXX_CFLAGS_OTHER})

