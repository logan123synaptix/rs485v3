cmake_minimum_required(VERSION 3.22)

add_subdirectory(utils/cqueue)
add_subdirectory(bsp)
add_subdirectory(services/modbus)
add_subdirectory(app)

# Phase 2 — uncomment when shell is ready
# add_subdirectory(services/shell)

# Phase 3 — uncomment when zigbee is ready
# add_subdirectory(services/zigbee)