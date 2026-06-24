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

#include "main.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "port.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern eModbus modbus_no1;
extern eModbus modbus_no2;

UART_HandleTypeDef *serial_port[2] = {&huart1,&huart2};
static uint8_t singlechar[2];
/* ----------------------- static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable(eModbus_t modbus, BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	if(xRxEnable)
	{
		HAL_UART_Receive_IT(serial_port[modbus->config.ucPort], &singlechar[modbus->config.ucPort], 1);
	}
	else
	{
		HAL_UART_AbortReceive_IT(serial_port[modbus->config.ucPort]);
	}

	if(xTxEnable)
	{
		modbus->pxMBFrameCBTransmitterEmpty(modbus);
	}
	else
	{
		HAL_UART_AbortTransmit_IT(serial_port[modbus->config.ucPort]);
	}
}

BOOL
xMBPortSerialInit(eModbus_t modbus, UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{

	modbus->config.ucPort = ucPORT;

    return TRUE;
}

BOOL
xMBPortSerialPutByte(eModbus_t modbus, CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	modbus->tx_done = FALSE;
	HAL_UART_Transmit_IT(serial_port[modbus->config.ucPort], (uint8_t *)&ucByte, 1);

	while(modbus->tx_done != TRUE);
    return TRUE;
}

BOOL
xMBPortSerialGetByte(eModbus_t modbus, CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
	*pucByte = (uint8_t)(singlechar[modbus->config.ucPort]);
    return TRUE;
}
BOOL xMBPortSerialPutBytes(eModbus_t modbus,volatile UCHAR *ucByte, USHORT usSize)
{
	HAL_UART_Transmit_IT(serial_port[modbus->config.ucPort], (uint8_t *)ucByte, usSize);
	return TRUE;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	eModbus_t modbus = &modbus_no1; // Calling Object

	if(huart->Instance == huart1.Instance)
	{
		modbus->pxMBFrameCBByteReceived(modbus);
		HAL_UART_Receive_IT(serial_port[modbus->config.ucPort], &singlechar[modbus->config.ucPort], 1);
		return;
	}

	modbus = &modbus_no2;
	if(huart->Instance == huart2.Instance)
	{
		modbus->pxMBFrameCBByteReceived(modbus);
		HAL_UART_Receive_IT(serial_port[modbus->config.ucPort], &singlechar[modbus->config.ucPort], 1);
		return;
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

	eModbus_t modbus = &modbus_no1;// Calling Object

	if(huart->Instance == huart1.Instance)
	{
		modbus->pxMBFrameCBTransmitterEmpty(modbus);
		return;
	}
	modbus = &modbus_no2;
	if(huart->Instance == huart2.Instance)
	{
		modbus->pxMBFrameCBTransmitterEmpty(modbus);
		return;
	}
}
static uint32_t lock_count = 0;

void __critical_enter(void)
{
	__disable_irq();
	lock_count++;
}

void __critical_exit(void)
{
	lock_count--;
	if (lock_count == 0)
		__enable_irq();
}
