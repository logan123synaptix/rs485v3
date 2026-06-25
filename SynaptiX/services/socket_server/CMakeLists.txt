cmake_minimum_required(VERSION 3.22)

add_library(synaptix_socketserver STATIC
    socketserver.c
)

target_include_directories(synaptix_socketserver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_socketserver PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
)