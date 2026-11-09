cmake_minimum_required(VERSION 3.10)

project(app)

add_executable(arch_search arch_search.cpp)

target_link_libraries(arch_search PRIVATE
    planner
    util
)

set_target_properties(arch_search PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)