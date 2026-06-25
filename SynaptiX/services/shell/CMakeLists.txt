cmake_minimum_required(VERSION 3.22)

add_library(synaptix_shell STATIC
    cli_shell.c
)

target_include_directories(synaptix_shell PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_shell PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
    synaptix_bsp
)