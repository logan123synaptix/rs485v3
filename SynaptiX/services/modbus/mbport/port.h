/**
 * @file port.h
 * @brief FreeModbus port layer — type definitions and platform glue for RF_IO_RS485_V3
 *
 * NOTE: eModbus / eModbus_t are defined in stack/modbus/include/mbport.h
 *       as a struct+pointer. Do NOT redefine them here.
 */

#ifndef PORT_H
#define PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define INLINE                  inline
#define PR_BEGIN_EXTERN_C       extern "C" {
#define PR_END_EXTERN_C         }

#include "FreeRTOS.h"
#include "task.h"
#define ENTER_CRITICAL_SECTION()    taskENTER_CRITICAL()
#define EXIT_CRITICAL_SECTION()     taskEXIT_CRITICAL()

typedef uint8_t     BOOL;
typedef uint8_t     UCHAR;
typedef char        CHAR;
typedef uint16_t    USHORT;
typedef int16_t     SHORT;
typedef uint32_t    ULONG;
typedef int32_t     LONG;

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef N_MODBUS
#define N_MODBUS    1
#endif

/* eModbus / eModbus_t defined in stack/modbus/include/mbport.h — not here */

#ifdef __cplusplus
}
#endif

#endif /* PORT_H */