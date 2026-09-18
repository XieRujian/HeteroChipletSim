cmake_minimum_required(VERSION 3.10)

project(planner)

add_subdirectory(data_distributor)

set(PLANNER_SOURCES
    data_distributor.cpp
    dataflow_scheduler.cpp
    framework.cpp
    phase.cpp
    task_initializer.cpp
    task_scheduler.cpp
    $<TARGET_OBJECTS:data_distributor_obj>
)

add_library(planner STATIC ${PLANNER_SOURCES})

target_include_directories(planner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(planner 
    PUBLIC core
    PUBLIC util
    PUBLIC math
)

target_compile_options(planner PRIVATE -Wall -Wextra -Wpedantic)
