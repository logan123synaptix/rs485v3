cmake_minimum_required(VERSION 3.22)

add_library(synaptix_app STATIC
    app.c
    user/modbus/user_mb_app.c
)

target_include_directories(synaptix_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/user/modbus
)

target_link_libraries(synaptix_app PUBLIC
    synaptix_bsp
    synaptix_modbus
)