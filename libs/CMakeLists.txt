cmake_minimum_required(VERSION VERSION 3.22)

enable_language(C CXX ASM)

add_library(libs INTERFACE)



set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stm32-mw-lwip)

set(LWIP_INCLUDE_DIRS ${LWIP_DIR}/src/include
                      )

add_subdirectory(stm32-mw-lwip)

add_library(lwip_port STATIC ${LWIP_DIR}/system/OS/sys_arch.c)

target_include_directories(lwip_port PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../lwip/Target
    ${LWIP_DIR}/system
    ${LWIP_INCLUDE_DIRS}
)

target_include_directories(lwipcore PUBLIC ${LWIP_PORT_INC})
target_include_directories(lwipallapps PUBLIC ${LWIP_PORT_INC})
target_include_directories(lwipmbedtls PUBLIC ${LWIP_PORT_INC})

target_link_libraries(lwip_port PUBLIC stm32cubemx)

target_link_libraries(lwipcore PUBLIC 
    stm32cubemx
    lwip_port
)

target_link_libraries(lwipallapps PUBLIC 
    stm32cubemx
    lwip_port
)

target_link_libraries(lwipmbedtls PUBLIC 
    stm32cubemx
    lwip_port
)

add_subdirectory(tinyusb/src)

add_library(tinyusb STATIC)

tinyusb_target_add(tinyusb)

file(GLOB USB_NET_LIB_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/lib/networking/*.c)

target_sources(tinyusb PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/../usb/Target/usb_descriptors.c
        ${USB_NET_LIB_SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/portable/st/stm32_fsdev/fsdev_common.c
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/portable/st/stm32_fsdev/hcd_stm32_fsdev.c
        )

target_include_directories(tinyusb PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/../usb/Target
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/common 
        ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/device)

target_link_libraries(tinyusb PUBLIC 
    stm32cubemx
    lwipcore
    lwipallapps
    lwipmbedtls)

add_subdirectory(logger)

target_link_libraries(libs INTERFACE 
    lwipcore
    lwipallapps
    lwipmbedtls
    tinyusb
    lwip_port
    logger
)

