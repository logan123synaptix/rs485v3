/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

// #include "main.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "port.h"
#include "string.h"
#if MB_PLATFORM == STM32_FLATFORM
#include "main.h"
#endif

#include "bsp_uart.h"


eModbus modbus[N_MODBUS];
UARTHandle_t mb_uart[N_MODBUS];

UCHAR singlechar[N_MODBUS];
/* ----------------------- static functions ---------------------------------*/

static void mb_uart_rx_cb(void *arg, uint8_t *data, uint16_t len)
{
    UARTHandle_t *uart = (UARTHandle_t *)arg;
    int i = len;
    while (i > 0)
    {
        if (cqueue_send(&uart->rx_fifo, (void*) &data[len - i]) == 1)
        {
            i--;
        }
        else
        {
            break;
        }
    }

	for(int i = 0;i<N_MODBUS;i++){
		UARTHandle_t *mb_handle = (UARTHandle_t *) modbus[i].rs485_handle;
		if(uart == mb_handle){
			modbus[i].pxMBFrameCBByteReceived(&modbus[i]);
//			modbus[i].rs485_recv_it(modbus[i].handle,&singlechar[modbus[i].config.ucPort],1);
			return;
		}
	}

}

static void mb_uart_tx_cb(void *arg){
	UARTHandle_t *uart = (UARTHandle_t *)arg;
	for(int i = 0;i<N_MODBUS;i++){
		UARTHandle_t *mb_handle = (UARTHandle_t *) modbus[i].rs485_handle;
		if(uart== mb_handle){
			modbus[i].pxMBFrameCBTransmitterEmpty(&modbus[i]);
			return;
		}
	}
}

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable(eModbus_t modbus, BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	if(xRxEnable)
	{
		if(modbus->rs485_de_deselect != NULL) modbus->rs485_de_deselect();
		// User code begin
//		modbus->rs485_recv_it(modbus->handle,&singlechar[modbus->config.ucPort],1);
		// User code end
	}
	else
	{
		// User code begin
//		bsp_uart_rx_about(modbus->handle);
		// User code end
	}

	if(xTxEnable)
	{
		if(modbus->rs485_de_select != NULL) modbus->rs485_de_select();
		modbus->pxMBFrameCBTransmitterEmpty(modbus);
	}
	else
	{
		// User code begin
//		bsp_uart_rx_about(modbus->handle);
		// User code end
	}
}

BOOL
xMBPortSerialInit(eModbus_t modbus, UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{

	modbus->config.ucPort = ucPORT;
	modbus->rs485_handle = (void*) &mb_uart[ucPORT];
	bsp_uart_set_rx_callback(modbus->rs485_handle, mb_uart_rx_cb, modbus->rs485_handle);
	bsp_uart_set_tx_callback(modbus->rs485_handle, mb_uart_tx_cb, modbus->rs485_handle);
    return TRUE;
}

BOOL
xMBPortSerialPutByte(eModbus_t modbus, CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	modbus->tx_done = FALSE;
	bsp_uart_write(modbus->rs485_handle,(uint8_t*) &ucByte, 1);
    return TRUE;
}

BOOL
xMBPortSerialGetByte(eModbus_t modbus, CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
//	*pucByte = (uint8_t)(singlechar[modbus->config.ucPort]);
	bsp_uart_read(modbus->rs485_handle,(uint8_t*)pucByte, 1);
    return TRUE;
}
BOOL xMBPortSerialPutBytes(eModbus_t modbus,volatile UCHAR *ucByte, USHORT usSize)
{
//	modbus->rs485_send_it(modbus->rs485_handle,(UCHAR*)ucByte,usSize);
	bsp_uart_write(modbus->rs485_handle,(uint8_t*) ucByte, usSize);
	return TRUE;
}

static uint32_t lock_count = 0;

void __critical_enter(void)
{
#if MB_PLATFORM == STM32_FLATFORM
	 __disable_irq();
#endif
	lock_count++;
}

void __critical_exit(void)
{
	lock_count--;
	if (lock_count == 0){
#if MB_PLATFORM == STM32_FLATFORM
		 __enable_irq();
#endif
	}	
}
