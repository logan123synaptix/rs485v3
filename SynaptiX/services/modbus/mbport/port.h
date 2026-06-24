/**
 * @file port.h
 * @brief FreeModbus port layer — type definitions and platform glue for RF_IO_RS485_V3
 *
 * This file is the single required "port.h" that FreeModbus stack includes
 * (via mbport.h → port.h). It defines the portable types (BOOL, UCHAR, …)
 * and the critical section macros expected by the stack.
 *
 * No external config header is needed — all definitions are self-contained.
 */

#ifndef PORT_H
#define PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── Compiler hints ───────────────────────────────────────────────────────── */
#define INLINE                  inline
#define PR_BEGIN_EXTERN_C       extern "C" {
#define PR_END_EXTERN_C         }

/* ── Critical section ─────────────────────────────────────────────────────── */
/* FreeRTOS taskENTER/EXIT_CRITICAL handle nesting correctly on Cortex-M33.   */
#include "FreeRTOS.h"
#include "task.h"
#define ENTER_CRITICAL_SECTION()    taskENTER_CRITICAL()
#define EXIT_CRITICAL_SECTION()     taskEXIT_CRITICAL()

/* ── Portable types required by FreeModbus stack ──────────────────────────── */
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

/* ── Number of concurrent Modbus instances ────────────────────────────────── */
/* V3 has 1 RS485 slave only. Zigbee does NOT use Modbus.                     */
#ifndef N_MODBUS
#define N_MODBUS    1
#endif

/* ── eModbus instance type ────────────────────────────────────────────────── */
/* Used as the first argument to all mbport functions to identify the          */
/* Modbus instance. N_MODBUS=1 so only MB_RS485 is valid.                     */
typedef enum {
    MB_RS485 = 0
} eModbus_t;

/* Alias used in older V2 code that passes plain int */
typedef eModbus_t eModbus;

#ifdef __cplusplus
}
#endif

#endif /* PORT_H */