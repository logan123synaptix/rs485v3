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

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"

#include "mbport.h"

#include "../port/port.h"
#if MB_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Static variables ---------------------------------*/

//static UCHAR    ucMBAddress;
//static eMBMode  eMBCurrentMode;

typedef enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
}eMBState_t;

typedef struct eModbus_Handle{
	peMBFrameSend peMBFrameSendCur;
	pvMBFrameStart pvMBFrameStartCur;
	pvMBFrameStop pvMBFrameStopCur;
	peMBFrameReceive peMBFrameReceiveCur;
	pvMBFrameClose pvMBFrameCloseCur;
	eMBState_t eMBState;
	eMBMode  eMBCurrentMode;
}eModbus_Handle_t;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 */
//static peMBFrameSend peMBFrameSendCur;
//static pvMBFrameStart pvMBFrameStartCur;
//static pvMBFrameStop pvMBFrameStopCur;
//static peMBFrameReceive peMBFrameReceiveCur;
//static pvMBFrameClose pvMBFrameCloseCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 */
//BOOL( *pxMBFrameCBByteReceived ) ( void );
//BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );
//BOOL( *pxMBPortCBTimerExpired ) ( void );
//
//BOOL( *pxMBFrameCBReceiveFSMCur ) ( void );
//BOOL( *pxMBFrameCBTransmitFSMCur ) ( void );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBInit(eModbus_t modbus, eMBMode eMode, UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    eModbus_Handle_t* handle = (eModbus_Handle_t*)malloc(sizeof(eModbus_Handle_t));
    if(handle == NULL) return MB_EINVAL;

    modbus->config.ucPort = ucPort;
    modbus->config.ulBaudRate = ulBaudRate;
    modbus->config.eParity = eParity;
    handle->eMBState = STATE_NOT_INITIALIZED;
	modbus->handle = handle;
    /* check preconditions */
    if( ( ucSlaveAddress == MB_ADDRESS_BROADCAST ) ||
        ( ucSlaveAddress < MB_ADDRESS_MIN ) || ( ucSlaveAddress > MB_ADDRESS_MAX ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
    	modbus->config.ucMBAddress = ucSlaveAddress;

        switch ( eMode )
        {
#if MB_RTU_ENABLED > 0
        case MB_RTU:
        	handle->pvMBFrameStartCur = eMBRTUStart;
        	handle->pvMBFrameStopCur = eMBRTUStop;
        	handle->peMBFrameSendCur = eMBRTUSend;
        	handle->peMBFrameReceiveCur = eMBRTUReceive;
        	handle->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
        	modbus->pxMBFrameCBByteReceived = xMBRTUReceiveFSM;
        	modbus->pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM;
        	modbus->pxMBPortCBTimerExpired = xMBRTUTimerT35Expired;

            eStatus = eMBRTUInit(modbus, modbus->config.ucMBAddress, modbus->config.ucPort, modbus->config.ulBaudRate, modbus->config.eParity );
            break;
#endif
#if MB_ASCII_ENABLED > 0
        case MB_ASCII:
        	handle->pvMBFrameStartCur = eMBASCIIStart;
        	handle->pvMBFrameStopCur = eMBASCIIStop;
        	handle->peMBFrameSendCur = eMBASCIISend;
        	handle->peMBFrameReceiveCur = eMBASCIIReceive;
        	handle->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
        	modbus->pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
        	modbus->pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
        	modbus->pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

            eStatus = eMBASCIIInit(modbus, modbus->config.ucMBAddress, modbus->config.ucPort, modbus->config.ulBaudRate, modbus->config.eParity );
            break;
#endif
        default:
            eStatus = MB_EINVAL;
        }

        if( eStatus == MB_ENOERR )
        {
            if( !xMBPortEventInit(modbus) )
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
            	handle->eMBCurrentMode = eMode;
                handle->eMBState = STATE_DISABLED;
            }
        }
    }
    if(eStatus != MB_ENOERR)
    	free(handle);
    return eStatus;
}

#if MB_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit(eModbus_t modbus, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    eModbus_Handle_t* handle = (eModbus_Handle_t*)malloc(sizeof(eModbus_Handle_t));
    if(handle == NULL) return MB_EINVAL;
    modbus->handle = handle;
    if( ( eStatus = eMBTCPDoInit(modbus, ucTCPPort ) ) != MB_ENOERR )
    {
        handle->eMBState = STATE_DISABLED;
    }
    else if( !xMBPortEventInit( modbus ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
    	handle->pvMBFrameStartCur = eMBTCPStart;
    	handle->pvMBFrameStopCur = eMBTCPStop;
    	handle->peMBFrameReceiveCur = eMBTCPReceive;
    	handle->peMBFrameSendCur = eMBTCPSend;
    	handle->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
    	modbus->config.ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
    	handle->eMBCurrentMode = MB_TCP;
        handle->eMBState = STATE_DISABLED;
    }
    if(eStatus != MB_ENOERR)
    	free(handle);
    return eStatus;
}
#endif

eMBErrorCode
eMBRegisterCB(eModbus_t modbus, UCHAR ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int             i;
    eMBErrorCode    eStatus;

    if( ( 0 < ucFunctionCode ) && ( ucFunctionCode <= 127 ) )
    {
        ENTER_CRITICAL_SECTION(  );
        if( pxHandler != NULL )
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( ( xFuncHandlers[i].pxHandler == NULL ) ||
                    ( xFuncHandlers[i].pxHandler == pxHandler ) )
                {
                    xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    xFuncHandlers[i].pxHandler = pxHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    xFuncHandlers[i].ucFunctionCode = 0;
                    xFuncHandlers[i].pxHandler = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}


eMBErrorCode
eMBClose( eModbus_t modbus )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    eModbus_Handle_t *handle = (eModbus_Handle_t*) modbus->handle;
    if( handle->eMBState == STATE_DISABLED )
    {
        if( handle->pvMBFrameCloseCur != NULL )
        {
            handle->pvMBFrameCloseCur( modbus );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBEnable( eModbus_t modbus )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    eModbus_Handle_t *handle = (eModbus_Handle_t*) modbus->handle;
    if( handle->eMBState == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
    	handle->pvMBFrameStartCur( modbus );
    	handle->eMBState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBDisable( eModbus_t modbus )
{
    eMBErrorCode    eStatus;
    eModbus_Handle_t *handle = (eModbus_Handle_t*) modbus->handle;
    if( handle->eMBState == STATE_ENABLED )
    {
    	handle->pvMBFrameStopCur( modbus );
    	handle->eMBState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( handle->eMBState == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBPoll( eModbus_t modbus )
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static USHORT   usLength;
    static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;
    eModbus_Handle_t *handle = (eModbus_Handle_t*) modbus->handle;
    /* Check if the protocol stack is ready. */
    if( handle->eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( xMBPortEventGet(modbus, &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = handle->peMBFrameReceiveCur(modbus, &ucRcvAddress, &ucMBFrame, &usLength );
            if( eStatus == MB_ENOERR )
            {
                /* Check if the frame is for us. If not ignore the frame. */
                if( ( ucRcvAddress == modbus->config.ucMBAddress ) || ( ucRcvAddress == MB_ADDRESS_BROADCAST ) )
                {
                    ( void )xMBPortEventPost(modbus, EV_EXECUTE );
                }
            }
            break;

        case EV_EXECUTE:
            eException = MB_EX_ILLEGAL_FUNCTION;
            ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                /* No more function handlers registered. Abort. */
                if( xFuncHandlers[i].ucFunctionCode == 0 )
                {
                    break;
                }
                else if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    eException = xFuncHandlers[i].pxHandler( ucMBFrame, &usLength );
                    break;
                }
            }

            /* If the request was not sent to the broadcast address we
             * return a reply. */
            if( ucRcvAddress != MB_ADDRESS_BROADCAST )
            {
                if( eException != MB_EX_NONE )
                {
                    /* An exception occured. Build an error frame. */
                    usLength = 0;
                    ucMBFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );
                    ucMBFrame[usLength++] = eException;
                }
                if( ( handle->eMBCurrentMode == MB_ASCII ) && MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS )
                {
                    vMBPortTimersDelay(modbus, MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );
                }                
                eStatus = handle->peMBFrameSendCur(modbus, modbus->config.ucMBAddress, ucMBFrame, usLength );
            }
            break;

        case EV_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}
