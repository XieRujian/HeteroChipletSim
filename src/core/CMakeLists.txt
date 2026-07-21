cmake_minimum_required(VERSION 3.10)

project(core)

add_subdirectory(arch)
add_subdirectory(task)

set(BASE_SOURCES
    arch.cpp
    data_block.cpp
    network.cpp
    task.cpp
    $<TARGET_OBJECTS:task_obj>
    $<TARGET_OBJECTS:arch_obj>
)

add_library(core STATIC ${BASE_SOURCES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(core
    PUBLIC util
)

target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
