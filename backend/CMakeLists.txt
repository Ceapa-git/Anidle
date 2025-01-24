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
pkg_check_modules(LIBBSONCXX REQUIRED libbsoncxx)

include_directories(${LIBMONGOCXX_INCLUDE_DIRS} ${LIBBSONCXX_INCLUDE_DIRS})
link_directories(${LIBMONGOCXX_LIBRARY_DIRS} ${LIBBSONCXX_LIBRARY_DIRS})

find_package(jwt-cpp CONFIG REQUIRED)

find_package(OpenSSL REQUIRED)

# Collect all .cpp files
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")

# Include header files
include_directories(${CMAKE_SOURCE_DIR})

# Add executable
add_executable(backend ${SOURCES})

target_link_libraries(backend ${LIBMONGOCXX_LIBRARIES} ${LIBBSONCXX_LIBRARIES}  jwt-cpp::jwt-cpp OpenSSL::Crypto)

target_compile_options(backend PRIVATE ${LIBMONGOCXX_CFLAGS_OTHER} ${LIBMONGOCXX_CFLAGS_OTHER})

