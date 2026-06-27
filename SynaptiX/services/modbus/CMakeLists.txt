cmake_minimum_required(VERSION 3.22)

set(STACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stack/modbus)
set(MBPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbport)

add_library(synaptix_modbus STATIC
    # Service layer
    modbus_service.c

    # Port layer (V3-specific HAL adaptation)
    ${MBPORT_DIR}/portevent.c
    ${MBPORT_DIR}/portserial.c
    ${MBPORT_DIR}/porttimer.c

    # FreeModbus stack core
    ${STACK_DIR}/mb.c

    # RTU (only mode used)
    ${STACK_DIR}/rtu/mbrtu.c
    ${STACK_DIR}/rtu/mbcrc.c

    # Function codes
    ${STACK_DIR}/functions/mbfunccoils.c
    ${STACK_DIR}/functions/mbfuncdiag.c
    ${STACK_DIR}/functions/mbfuncdisc.c
    ${STACK_DIR}/functions/mbfuncholding.c
    ${STACK_DIR}/functions/mbfuncinput.c
    ${STACK_DIR}/functions/mbfuncother.c
    ${STACK_DIR}/functions/mbutils.c

    # ASCII (compiled in, disabled via mbconfig.h at runtime)
    ${STACK_DIR}/ascii/mbascii.c

    # TCP (compiled in, disabled via mbconfig.h at runtime)
    ${STACK_DIR}/tcp/mbtcp.c
)

target_include_directories(synaptix_modbus PUBLIC
    # Public header for consumers (modbus_service.h)
    ${CMAKE_CURRENT_SOURCE_DIR}

    # mbport headers (port.h)
    ${MBPORT_DIR}

    # FreeModbus public headers
    ${STACK_DIR}/include

    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/user/modbus
)

target_include_directories(synaptix_modbus PRIVATE
    # RTU/ASCII/TCP internal headers
    ${STACK_DIR}/rtu
    ${STACK_DIR}/ascii
    ${STACK_DIR}/tcp
)

target_link_libraries(synaptix_modbus PUBLIC
    stm32cubemx
    synaptix_bsp
)