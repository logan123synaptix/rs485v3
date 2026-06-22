#pragma once

// See https://mongoose.ws/documentation/#build-options
#define MG_ENABLE_FREERTOS 1
#define MG_ARCH MG_ARCH_NEWLIB
#define MG_OTA MG_OTA_NONE
#define MG_TLS MG_TLS_NONE
#define MG_IO_SIZE 256

#define MG_ENABLE_LWIP 1
#define MG_ENABLE_SOCKET 1
#define MG_ENABLE_CUSTOM_MILLIS 0
#define MG_ENABLE_CUSTOM_RANDOM 0
#define MG_ENABLE_PACKED_FS 1
#define MG_ENABLE_DRIVER_STM32H 0
#define MG_ENABLE_POSIX_FS 0
