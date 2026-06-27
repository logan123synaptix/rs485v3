/*
 * FreeModbus Library: RF_IO_RS485_V3 Port
 * Ported from RS485_v2 mbport/portserial.c
 *
 * Changes from V2:
 *   - board.h → bsp_uart.h + bsp_gpio.h
 *   - logger.h log_info → LOGI (logger macro)
 *   - Removed RF/Zigbee Modbus branch (N_MODBUS=1, RS485 only)
 *   - Fixed V2 bug: assignment instead of comparison in if(i = BSP_RF_COM_PORT)
 */

#include "mb.h"
#include "mbport.h"
#include "port.h"

#include "bsp_uart.h"
#include "bsp_gpio.h"

#include <string.h>
#include <stdio.h>

/* ── Instance table ───────────────────────────────────────────────────────── */

eModbus modbus[N_MODBUS] = {
    [0] = {
        .rs485_de_select   = bsp_rs485_de_on,
        .rs485_de_deselect = bsp_rs485_de_off,
    },
};

/* ── Internal callbacks ───────────────────────────────────────────────────── */

static void mb_uart_rx_task(eModbus *mb)
{
    int len = bsp_com_available(mb->config.ucPort);
    if (len > 0) {
        for (int i = 0; i < len; i++) {
            if (mb->pxMBFrameCBByteReceived != NULL) {
                mb->pxMBFrameCBByteReceived(mb);
            }
        }
    }
}

static void mb_uart_tx_task(void *arg)
{
    eModbus *mb = (eModbus *)arg;
    if (mb->pxMBFrameCBTransmitterEmpty != NULL) {
        mb->pxMBFrameCBTransmitterEmpty(mb);
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

BOOL xMBPortSerialInit(eModbus_t modbus, UCHAR ucPORT, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity)
{
    modbus->config.ucPort = ucPORT;
    bsp_com_set_tx_callback(ucPORT, mb_uart_tx_task, modbus);
    return TRUE;
}

void vMBPortSerialEnable(eModbus_t modbus, BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable) {
        if (modbus->rs485_de_deselect != NULL) modbus->rs485_de_deselect();
    }

    if (xTxEnable) {
        if (modbus->rs485_de_select != NULL) modbus->rs485_de_select();
        if (modbus->pxMBFrameCBTransmitterEmpty != NULL) {
            modbus->pxMBFrameCBTransmitterEmpty(modbus);
        }
    }
}

BOOL xMBPortSerialGetByte(eModbus_t modbus, CHAR *pucByte)
{
    uint8_t res = bsp_com_read(modbus->config.ucPort, (uint8_t *)pucByte, 1);
    return (res == 1) ? TRUE : FALSE;
}

BOOL xMBPortSerialPutByte(eModbus_t modbus, CHAR ucByte)
{
    modbus->tx_done = FALSE;
    bsp_com_write_it(modbus->config.ucPort, (uint8_t *)&ucByte, 1);
    return TRUE;
}

BOOL xMBPortSerialPutBytes(eModbus_t modbus, volatile UCHAR *ucByte, USHORT usSize)
{
    bsp_com_write_it(modbus->config.ucPort, (uint8_t *)ucByte, usSize);
    return TRUE;
}

/* ── Called from modbus_service poll loop ─────────────────────────────────── */

void vMBPortSerialPoll(eModbus_t modbus)
{
    mb_uart_rx_task(modbus);
}