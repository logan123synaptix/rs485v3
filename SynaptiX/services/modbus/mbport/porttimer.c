/*
 * FreeModbus Library: RF_IO_RS485_V3 Port
 * Ported from RS485_v2 mbport/porttimer.c
 *
 * Changes from V2:
 *   - board.h → bsp_uart.h
 *   - N_MODBUS=1 — only RS485 instance, no RF branch
 */

#include "mb.h"
#include "mbport.h"
#include "port.h"
#include "bsp_uart.h"

extern eModbus modbus[N_MODBUS];

/* ── Timer callbacks ──────────────────────────────────────────────────────── */

static void mb_timer0_callback(void)
{
    if (modbus[0].pxMBPortCBTimerExpired != NULL) {
        modbus[0].pxMBPortCBTimerExpired(&modbus[0]);
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

BOOL xMBPortTimersInit(eModbus_t modbus, USHORT usTim1Timerout50us)
{
    bsp_timer_set_handle(0, mb_timer0_callback);
    return TRUE;
}

inline void vMBPortTimersEnable(eModbus_t modbus)
{
    if (modbus->config.ucPort == BSP_RS485_COM_PORT) {
        bsp_timer0_start();
    }
}

inline void vMBPortTimersDisable(eModbus_t modbus)
{
    if (modbus->config.ucPort == BSP_RS485_COM_PORT) {
        bsp_timer0_stop();
    }
}

void vMBPortTimersDelay(eModbus_t modbus, USHORT usTimeOutMS)
{
    (void)modbus;
    /* FreeRTOS delay — only call from task context */
    extern void vTaskDelay(uint32_t);
    vTaskDelay(usTimeOutMS);
}