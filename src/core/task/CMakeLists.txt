set(TASK_SOURCES
    attention.cpp
    conv.cpp
    gemm.cpp
)

add_library(task_obj OBJECT ${TASK_SOURCES})
