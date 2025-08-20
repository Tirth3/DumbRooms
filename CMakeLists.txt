cmake_minimum_required(VERSION 3.15)
project(Arctic3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL3 (must be installed on system or provided with CMAKE_PREFIX_PATH)
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

# Include headers from include/
include_directories(include)

# Collect all source files (main.cpp + everything in src/)
file(GLOB_RECURSE PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Build executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Link SDL3
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)


