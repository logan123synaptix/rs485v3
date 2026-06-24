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
#include "mbascii.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"
#include "../../port/port.h"

#if MB_ASCII_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */
#define MB_SER_PDU_SIZE_MIN     3       /*!< Minimum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_LRC     1       /*!< Size of LRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_WAIT_EOF           /*!< Wait for End of Frame. */
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_TX_START,             /*!< Starting transmission (':' sent). */
    STATE_TX_DATA,              /*!< Sending of data (Address, Data, LRC). */
    STATE_TX_END,               /*!< End of transmission. */
    STATE_TX_NOTIFY             /*!< Notify sender that the frame has been sent. */
} eMBSndState;



/* ----------------------- Static functions ---------------------------------*/
static UCHAR    prvucMBCHAR2BIN( UCHAR ucCharacter );

static UCHAR    prvucMBBIN2CHAR( UCHAR ucByte );

static UCHAR    prvucMBLRC( UCHAR * pucFrame, USHORT usLen );

/* ----------------------- Static variables ---------------------------------*/
//static volatile eMBSndState eSndState;
//static volatile eMBRcvState eRcvState;

/* We reuse the Modbus RTU buffer because only one buffer is needed and the
 * RTU buffer is bigger. */

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBASCIIInit(eModbus_t modbus, UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ( void )ucSlaveAddress;
    
    ENTER_CRITICAL_SECTION(  );
    modbus->ucMBLFCharacter = MB_ASCII_DEFAULT_LF;

    if( xMBPortSerialInit(modbus, ucPort, ulBaudRate, 7, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else if( xMBPortTimersInit(modbus, MB_ASCII_TIMEOUT_SEC * 20000UL ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }

    EXIT_CRITICAL_SECTION(  );

    if(eStatus == MB_ENOERR){
    	eMBSndState *eSndState = malloc(sizeof(eMBSndState));
    	eMBRcvState *eRcvState = malloc(sizeof(eMBRcvState));
    	modbus->RcvState = eRcvState;
    	modbus->SndState = eSndState;
    }

    return eStatus;
}

void
eMBASCIIStart( eModbus_t modbus )
{
    ENTER_CRITICAL_SECTION(  );
    vMBPortSerialEnable(modbus, TRUE, FALSE );
    eMBRcvState *state = modbus->RcvState;
    *state = STATE_RX_IDLE;
    EXIT_CRITICAL_SECTION(  );

    /* No special startup required for ASCII. */
    ( void )xMBPortEventPost(modbus, EV_READY );
}

void
eMBASCIIStop( eModbus_t modbus )
{
    ENTER_CRITICAL_SECTION(  );
    vMBPortSerialEnable(modbus, FALSE, FALSE );
    vMBPortTimersDisable(modbus  );
    EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBASCIIReceive(eModbus_t modbus, UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION(  );
    assert( modbus->usRcvBufferPos < MB_SER_PDU_SIZE_MAX );

    /* Length and CRC check */
    if( ( modbus->usRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( prvucMBLRC( ( UCHAR * ) modbus->ucASCIIBuf, modbus->usRcvBufferPos ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = modbus->ucASCIIBuf[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( modbus->usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_LRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & modbus->ucASCIIBuf[MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBASCIISend(eModbus_t modbus, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR           usLRC;

    ENTER_CRITICAL_SECTION(  );
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    eMBRcvState *state = modbus->RcvState;
    eMBSndState *SndState = modbus->SndState;
    if( *state == STATE_RX_IDLE )
    {
        /* First byte before the Modbus-PDU is the slave address. */
    	modbus->pucSndBufferCur = ( UCHAR * ) pucFrame - 1;
    	modbus->usSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
    	modbus->pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
    	modbus->usSndBufferCount += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = prvucMBLRC( ( UCHAR * ) modbus->pucSndBufferCur, modbus->usSndBufferCount );
        modbus->ucASCIIBuf[modbus->usSndBufferCount++] = usLRC;

        /* Activate the transmitter. */
        *SndState = STATE_TX_START;
        vMBPortSerialEnable(modbus, FALSE, TRUE );
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
xMBASCIIReceiveFSM( eModbus_t modbus )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;
    UCHAR           ucResult;
    eMBSndState *SndState = modbus->SndState;
    eMBRcvState *RcvState = modbus->RcvState;
    assert( *SndState == STATE_TX_IDLE );

    ( void )xMBPortSerialGetByte(modbus, ( CHAR * ) & ucByte );
    switch ( *RcvState )
    {
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
    case STATE_RX_RCV:
        /* Enable timer for character timeout. */
        vMBPortTimersEnable( modbus );
        if( ucByte == ':' )
        {
            /* Empty receive buffer. */
        	modbus->eBytePos = BYTE_HIGH_NIBBLE;
        	modbus->usRcvBufferPos = 0;
        }
        else if( ucByte == MB_ASCII_DEFAULT_CR )
        {
            *RcvState = STATE_RX_WAIT_EOF;
        }
        else
        {
            ucResult = prvucMBCHAR2BIN( ucByte );
            switch ( modbus->eBytePos )
            {
                /* High nibble of the byte comes first. We check for
                 * a buffer overflow here. */
            case BYTE_HIGH_NIBBLE:
                if( modbus->usRcvBufferPos < MB_SER_PDU_SIZE_MAX )
                {
                	modbus->ucASCIIBuf[modbus->usRcvBufferPos] = ( UCHAR )( ucResult << 4 );
                	modbus->eBytePos = BYTE_LOW_NIBBLE;
                    break;
                }
                else
                {
                    /* not handled in Modbus specification but seems
                     * a resonable implementation. */
                    *RcvState = STATE_RX_IDLE;
                    /* Disable previously activated timer because of error state. */
                    vMBPortTimersDisable( modbus );
                }
                break;

            case BYTE_LOW_NIBBLE:
            	modbus->ucASCIIBuf[modbus->usRcvBufferPos] |= ucResult;
            	modbus->usRcvBufferPos++;
            	modbus->eBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_RX_WAIT_EOF:
        if( ucByte == modbus->ucMBLFCharacter )
        {
            /* Disable character timeout timer because all characters are
             * received. */
            vMBPortTimersDisable( modbus );
            /* Receiver is again in idle state. */
            *RcvState = STATE_RX_IDLE;

            /* Notify the caller of eMBASCIIReceive that a new frame
             * was received. */
            xNeedPoll = xMBPortEventPost(modbus, EV_FRAME_RECEIVED );
        }
        else if( ucByte == ':' )
        {
            /* Empty receive buffer and back to receive state. */
        	modbus->eBytePos = BYTE_HIGH_NIBBLE;
            modbus->usRcvBufferPos = 0;
            *RcvState = STATE_RX_RCV;

            /* Enable timer for character timeout. */
            vMBPortTimersEnable( modbus );
        }
        else
        {
            /* Frame is not okay. Delete entire frame. */
            *RcvState = STATE_RX_IDLE;
        }
        break;

    case STATE_RX_IDLE:
        if( ucByte == ':' )
        {
            /* Enable timer for character timeout. */
            vMBPortTimersEnable( modbus );
            /* Reset the input buffers to store the frame. */
            modbus->usRcvBufferPos = 0;;
            modbus->eBytePos = BYTE_HIGH_NIBBLE;
            *RcvState = STATE_RX_RCV;
        }
        break;
    }

    return xNeedPoll;
}

BOOL
xMBASCIITransmitFSM( eModbus_t modbus )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;
    eMBRcvState *state = modbus->RcvState;
    eMBSndState *SndState = modbus->SndState;
    assert( *state == STATE_RX_IDLE );
    switch ( *SndState )
    {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
    case STATE_TX_START:
        ucByte = ':';
        xMBPortSerialPutByte(modbus, ( CHAR )ucByte );
        *SndState = STATE_TX_DATA;
        modbus->eBytePos = BYTE_HIGH_NIBBLE;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream with the high nibble sent first and the low nibble sent
         * last. If all data bytes are exhausted we send a '\r' character
         * to end the transmission. */
    case STATE_TX_DATA:
        if( modbus->usSndBufferCount > 0 )
        {
            switch ( modbus->eBytePos )
            {
            case BYTE_HIGH_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *modbus->pucSndBufferCur >> 4 ) );
                xMBPortSerialPutByte(modbus, ( CHAR ) ucByte );
                modbus->eBytePos = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *modbus->pucSndBufferCur & 0x0F ) );
                xMBPortSerialPutByte(modbus, ( CHAR )ucByte );
                modbus->pucSndBufferCur++;
                modbus->eBytePos = BYTE_HIGH_NIBBLE;
                modbus->usSndBufferCount--;
                break;
            }
        }
        else
        {
            xMBPortSerialPutByte(modbus, MB_ASCII_DEFAULT_CR );
            *SndState = STATE_TX_END;
        }
        break;

        /* Finish the frame by sending a LF character. */
    case STATE_TX_END:
        xMBPortSerialPutByte(modbus, ( CHAR )modbus->ucMBLFCharacter );
        /* We need another state to make sure that the CR character has
         * been sent. */
        *SndState = STATE_TX_NOTIFY;
        break;

        /* Notify the task which called eMBASCIISend that the frame has
         * been sent. */
    case STATE_TX_NOTIFY:
        *SndState = STATE_TX_IDLE;
        xNeedPoll = xMBPortEventPost(modbus, EV_FRAME_SENT );

        /* Disable transmitter. This prevents another transmit buffer
         * empty interrupt. */
        vMBPortSerialEnable(modbus, TRUE, FALSE );
        *SndState = STATE_TX_IDLE;
        break;

        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(modbus, TRUE, FALSE );
        break;
    }

    return xNeedPoll;
}

BOOL
xMBASCIITimerT1SExpired(eModbus_t modbus )
{
	eMBRcvState *state = modbus->RcvState;
    switch ( *state )
    {
        /* If we have a timeout we go back to the idle state and wait for
         * the next frame.
         */
    case STATE_RX_RCV:
    case STATE_RX_WAIT_EOF:
        *state = STATE_RX_IDLE;
        break;

    default:
        assert( ( *state == STATE_RX_RCV ) || ( *state == STATE_RX_WAIT_EOF ) );
        break;
    }
    vMBPortTimersDisable( modbus );

    /* no context switch required. */
    return FALSE;
}


static          UCHAR
prvucMBCHAR2BIN( UCHAR ucCharacter )
{
    if( ( ucCharacter >= '0' ) && ( ucCharacter <= '9' ) )
    {
        return ( UCHAR )( ucCharacter - '0' );
    }
    else if( ( ucCharacter >= 'A' ) && ( ucCharacter <= 'F' ) )
    {
        return ( UCHAR )( ucCharacter - 'A' + 0x0A );
    }
    else
    {
        return 0xFF;
    }
}

static          UCHAR
prvucMBBIN2CHAR( UCHAR ucByte )
{
    if( ucByte <= 0x09 )
    {
        return ( UCHAR )( '0' + ucByte );
    }
    else if( ( ucByte >= 0x0A ) && ( ucByte <= 0x0F ) )
    {
        return ( UCHAR )( ucByte - 0x0A + 'A' );
    }
    else
    {
        /* Programming error. */
        assert( 0 );
    }
    return '0';
}


static          UCHAR
prvucMBLRC( UCHAR * pucFrame, USHORT usLen )
{
    UCHAR           ucLRC = 0;  /* LRC char initialized */

    while( usLen-- )
    {
        ucLRC += *pucFrame++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    ucLRC = ( UCHAR ) ( -( ( CHAR ) ucLRC ) );
    return ucLRC;
}

#endif
