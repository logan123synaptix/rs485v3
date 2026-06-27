/**
 * @file user_mb_app.h
 * @brief Modbus slave register definitions for RF_IO_RS485_V3.
 *
 * Register map:
 *   Discrete Inputs  (FC02): 64 — DI status, zigbee link status
 *   Coils            (FC01): 64 — DO control
 *   Input Registers  (FC04): 100 — sensor/telemetry data
 *   Holding Registers(FC03): 100 — configuration registers
 */

#ifndef _USER_MB_APP_
#define _USER_MB_APP_

#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbutils.h"

/* ── Register map ─────────────────────────────────────────────────────────── */

#define S_DISCRETE_INPUT_START      0
#define S_DISCRETE_INPUT_NDISCRETES 64
#if S_DISCRETE_INPUT_NDISCRETES > 0
extern UCHAR *input_coils;
#endif

#define S_COIL_START                0
#define S_COIL_NCOILS               64
#if S_COIL_NCOILS > 0
extern UCHAR *output_coils;
#endif

#define S_REG_INPUT_START           0
#define S_REG_INPUT_NREGS           100
#if S_REG_INPUT_NREGS > 0
extern USHORT *input_reg;
#endif

#define S_REG_HOLDING_START         0
#define S_REG_HOLDING_NREGS         100
#if S_REG_HOLDING_NREGS > 0
extern USHORT *hoding_reg;
#endif

/* ── Holding register addresses ───────────────────────────────────────────── */
#define S_HD_RESERVE                0
#define S_HD_CPU_USAGE_MAJOR        1
#define S_HD_CPU_USAGE_MINOR        2

/* ── Input register addresses ─────────────────────────────────────────────── */
#define S_IN_RESERVE                0

/* ── Coil addresses ───────────────────────────────────────────────────────── */
#define S_CO_RESERVE                0

/* ── Discrete input addresses ─────────────────────────────────────────────── */
#define S_DI_RESERVE                0

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Poll function — call periodically from modbus_task to update
 *        register mirrors with live device data (DI/DO, zigbee params).
 *        TODO Phase 3: wire zigbee_service_get_params() here.
 */
void user_mb_app_poll(void);

#endif /* _USER_MB_APP_ */