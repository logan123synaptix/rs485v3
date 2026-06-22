cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

add_library(usb_net STATIC)

file(GLOB USB_NET_SOURCE "usb_netif.c")

target_sources(usb_net PRIVATE ${USB_NET_SOURCE})

target_include_directories(usb_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(usb_net PUBLIC 
    libs
    stm32cubemx
)