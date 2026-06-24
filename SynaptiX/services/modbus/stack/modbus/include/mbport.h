/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif



/* ----------------------- Type definitions ---------------------------------*/


typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBBytePos;


typedef enum
{
    EV_READY,                   /*!< Startup finished. */
    EV_FRAME_RECEIVED,          /*!< Frame received. */
    EV_EXECUTE,                 /*!< Execute function. */
    EV_FRAME_SENT               /*!< Frame sent. */
} eMBEventType;

/*! \ingroup modbus
 * \brief Parity used for characters in serial mode.
 *
 * The parity which should be applied to the characters sent over the serial
 * link. Please note that this values are actually passed to the porting
 * layer and therefore not all parity modes might be available.
 */
typedef enum
{
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
} eMBParity;

typedef struct eMB_Config{
	UCHAR    ucMBAddress;
	UCHAR ucSlaveAddress;
	UCHAR ucPort;
	ULONG ulBaudRate;
	eMBParity eParity;
}eMB_Config_t;

typedef struct eModbus eModbus;
typedef eModbus* eModbus_t;

struct eModbus {
	void *handle;
	eMB_Config_t config;
	BOOL( *pxMBFrameCBByteReceived ) ( eModbus_t modbus );
	BOOL( *pxMBFrameCBTransmitterEmpty ) ( eModbus_t modbus );
	BOOL( *pxMBPortCBTimerExpired ) ( eModbus_t modbus );

	BOOL( *pxMBFrameCBReceiveFSMCur ) ( eModbus_t modbus );
	BOOL( *pxMBFrameCBTransmitFSMCur ) ( eModbus_t modbus );

	void *rs485_handle;
	BOOL(*rs485_de_select)();
	BOOL(*rs485_de_deselect)();
	USHORT (*rs485_send_it)(void *handle,UCHAR *buff,ULONG len);
	USHORT (*rs485_recv_it)(void *handle,UCHAR *buff,ULONG len);

	void (*timer_start)(void *handle);
	void (*timer_stop)(void *handle);

	void *timer_handle;
	volatile BOOL tx_done;
	volatile UCHAR  ucRTUBuf[256];

	volatile UCHAR *pucSndBufferCur;
	volatile USHORT usSndBufferCount;

	volatile USHORT usRcvBufferPos;

	volatile UCHAR *ucASCIIBuf;

	volatile eMBBytePos eBytePos;

	volatile UCHAR ucLRC;
	volatile UCHAR ucMBLFCharacter;

	void *RcvState;
	void *SndState;

	eMBEventType eQueuedEvent;
	BOOL     xEventInQueue;
};



/* ----------------------- Supporting functions -----------------------------*/
BOOL            xMBPortEventInit( eModbus_t modbus );

BOOL            xMBPortEventPost( eModbus_t modbus,eMBEventType eEvent );

BOOL            xMBPortEventGet(  /*@out@ */eModbus_t modbus, eMBEventType * eEvent );

/* ----------------------- Serial port functions ----------------------------*/

BOOL            xMBPortSerialInit(eModbus_t modbus, UCHAR ucPort, ULONG ulBaudRate,
                                   UCHAR ucDataBits, eMBParity eParity );

void            vMBPortClose( eModbus_t modbus );

void            xMBPortSerialClose( eModbus_t modbus );

void            vMBPortSerialEnable(eModbus_t modbus, BOOL xRxEnable, BOOL xTxEnable );

BOOL            xMBPortSerialGetByte(eModbus_t modbus, CHAR * pucByte );

BOOL            xMBPortSerialPutByte(eModbus_t modbus, CHAR ucByte );
BOOL 			xMBPortSerialPutBytes(eModbus_t modbus,volatile UCHAR *ucByte, USHORT usSize);
/* ----------------------- Timers functions ---------------------------------*/
BOOL            xMBPortTimersInit(eModbus_t modbus, USHORT usTimeOut50us );

void            xMBPortTimersClose( eModbus_t modbus );

void            vMBPortTimersEnable( eModbus_t modbus );

void            vMBPortTimersDisable( eModbus_t modbus );

void            vMBPortTimersDelay(eModbus_t modbus, USHORT usTimeOutMS );

/* ----------------------- Callback for the protocol stack ------------------*/

/*!
 * \brief Callback function for the porting layer when a new byte is
 *   available.
 *
 * Depending upon the mode this callback function is used by the RTU or
 * ASCII transmission layers. In any case a call to xMBPortSerialGetByte()
 * must immediately return a new character.
 *
 * \return <code>TRUE</code> if a event was posted to the queue because
 *   a new byte was received. The port implementation should wake up the
 *   tasks which are currently blocked on the eventqueue.
 */
//extern          BOOL( *pxMBFrameCBByteReceived ) ( void );
//
//extern          BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );
//
//extern          BOOL( *pxMBPortCBTimerExpired ) ( void );

/* ----------------------- TCP port functions -------------------------------*/
BOOL            xMBTCPPortInit(eModbus_t modbus, USHORT usTCPPort );

void            vMBTCPPortClose( eModbus_t modbus );

void            vMBTCPPortDisable( eModbus_t modbus );

BOOL            xMBTCPPortGetRequest(eModbus_t modbus, UCHAR **ppucMBTCPFrame, USHORT * usTCPLength );

BOOL            xMBTCPPortSendResponse(eModbus_t modbus, const UCHAR *pucMBTCPFrame, USHORT usTCPLength );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
