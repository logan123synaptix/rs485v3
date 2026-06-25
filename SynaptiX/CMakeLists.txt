cmake_minimum_required(VERSION 3.22)

add_subdirectory(utils/cqueue)
add_subdirectory(bsp)
add_subdirectory(services/modbus)
add_subdirectory(app)
add_subdirectory(board)

# Phase 2 — uncomment when shell is ready
# add_subdirectory(services/shell)

# Phase 3 — uncomment when zigbee is ready
add_subdirectory(services/zigbee)

# Phase 4 - Lora
add_subdirectory(services/lora)