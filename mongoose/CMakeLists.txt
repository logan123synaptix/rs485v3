cmake_minimum_required(VERSION 3.22)

enable_language(C CXX ASM)

add_library(mongoose STATIC)

file(GLOB MONGOOSE_SRC "*.c")

target_sources(mongoose PRIVATE ${MONGOOSE_SRC})

target_include_directories(mongoose PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mongoose PRIVATE lwipcore stm32cubemx)

