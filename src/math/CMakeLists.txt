cmake_minimum_required(VERSION 3.10)

project(math)

set(GUROBI_DIR "/opt/gurobi1301/linux64")
find_path(GUROBI_INCLUDE_DIRS gurobi_c++.h PATHS "${GUROBI_DIR}/include")
find_library(GUROBI_LIBRARY NAMES gurobi130 PATHS "${GUROBI_DIR}/lib")
find_library(GUROBI_CPP_LIBRARY NAMES gurobi_c++ PATHS "${GUROBI_DIR}/lib")

file(GLOB MATH_SOURCES "*.cpp")
add_library(math STATIC ${MATH_SOURCES})

target_include_directories(math PRIVATE ${GUROBI_INCLUDE_DIRS})
target_link_libraries(math PUBLIC ${GUROBI_CPP_LIBRARY} ${GUROBI_LIBRARY})

target_compile_options(math PRIVATE -Wall -Wextra -Wpedantic)
