cmake_minimum_required(VERSION 3.22)

add_library(synaptix_zigbee STATIC
    zigbee.c
)

target_include_directories(synaptix_zigbee PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_zigbee PUBLIC
    stm32cubemx
    synaptix_cqueue
    logger
)