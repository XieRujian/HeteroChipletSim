cmake_minimum_required(VERSION 3.10)

project(app)

add_executable(dataflow_schedule dataflow_schedule.cpp)
add_executable(task_schedule task_schedule.cpp)
add_executable(compute_energy compute_energy.cpp)

target_link_libraries(dataflow_schedule PRIVATE 
    planner
    util
)

target_link_libraries(task_schedule PRIVATE
    planner
    util
)

target_link_libraries(compute_energy PRIVATE
    core
    planner
    util
)

set_target_properties(dataflow_schedule PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(task_schedule PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(compute_energy PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)