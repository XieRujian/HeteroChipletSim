cmake_minimum_required(VERSION 3.10)

project(util)

set(UTIL_SOURCES
    cJSON.cpp
    common.cpp
    ini.cpp
    random.cpp
)

add_library(util STATIC ${UTIL_SOURCES})

target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(util PRIVATE -Wall -Wextra -Wpedantic)
