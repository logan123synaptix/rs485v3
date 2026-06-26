cmake_minimum_required(VERSION 3.22)

add_library(synaptix_web STATIC
    web_service.c
)

target_include_directories(synaptix_web PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(synaptix_web PUBLIC
    stm32cubemx
    mongoose
)