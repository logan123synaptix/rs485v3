cmake_minimum_required(VERSION 3.22)

add_library(synaptix_tcp STATIC
    tcp_server.c
)

target_include_directories(synaptix_tcp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../socket_server
)

target_link_libraries(synaptix_tcp PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
    lwipcore
)