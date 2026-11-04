cmake_minimum_required(VERSION 3.20)
project(armulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(armulator ${SOURCES})

target_include_directories(armulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)