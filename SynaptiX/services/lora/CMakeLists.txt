cmake_minimum_required(VERSION 3.22)

add_library(synaptix_lora STATIC
    lora.c
)

target_include_directories(synaptix_lora PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_lora PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
)