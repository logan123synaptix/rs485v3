cmake_minimum_required(VERSION 3.22)

add_library(synaptix_socketclient STATIC
    socketclient.c
)

target_include_directories(synaptix_socketclient PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../socket_server
)

target_link_libraries(synaptix_socketclient PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
    lwipcore
)