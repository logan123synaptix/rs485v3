cmake_minimum_required(VERSION 3.22)

add_library(synaptix_cqueue STATIC
    cqueue.c
)

target_include_directories(synaptix_cqueue PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)