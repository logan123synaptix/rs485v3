cmake_minimum_required(VERSION 3.22)

add_library(synaptix_bsp STATIC
    bsp_callbacks.c
    bsp_gpio.c
    bsp_uart.c
)

target_include_directories(synaptix_bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_bsp PUBLIC
    stm32cubemx
    synaptix_cqueue
)