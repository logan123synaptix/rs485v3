/**
 * @file user_mb_app.c
 * @brief Modbus register callbacks for RF_IO_RS485_V3
 *
 * Adapted from V2 user_mb_app.c.
 * Changes:
 *  - Removed direct `extern ZigbeeMesh_t zigbee` coupling (Bug fix — decoupled)
 *  - Zigbee params will be exposed via zigbee_service getter in Phase 3
 *  - bsp_get_address() stub returns hardcoded 1 (slave addr)
 */

#include "user_mb_app.h"

#if (MB_ASCII_ENABLED > 0 || MB_RTU_ENABLED > 0 || MB_TCP_ENABLED > 0)

/* ── Discrete Inputs (FC02 Read Discrete Inputs) ─────────────────────── */
#if S_DISCRETE_INPUT_NDISCRETES > 0
USHORT usSDiscInStart                                = S_DISCRETE_INPUT_START;
#if S_DISCRETE_INPUT_NDISCRETES % 8
UCHAR  ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES / 8 + 1];
#else
UCHAR  ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES / 8];
#endif
UCHAR *input_coils = ucSDiscInBuf;
#endif

/* ── Coils (FC01/FC05) ───────────────────────────────────────────────── */
#if S_COIL_NCOILS > 0
USHORT usSCoilStart                                  = S_COIL_START;
#if S_COIL_NCOILS % 8
UCHAR  ucSCoilBuf[S_COIL_NCOILS / 8 + 1];
#else
UCHAR  ucSCoilBuf[S_COIL_NCOILS / 8];
#endif
UCHAR *output_coils = ucSCoilBuf;
#endif

/* ── Input Registers (FC04) ──────────────────────────────────────────── */
#if S_REG_INPUT_NREGS > 0
USHORT usSRegInStart                                 = S_REG_INPUT_START;
USHORT usSRegInBuf[S_REG_INPUT_NREGS];
USHORT *input_reg = usSRegInBuf;
#endif

/* ── Holding Registers (FC03/FC06) ──────────────────────────────────── */
#if S_REG_HOLDING_NREGS > 0
USHORT usSRegHoldStart                               = S_REG_HOLDING_START;
USHORT usSRegHoldBuf[S_REG_HOLDING_NREGS];
USHORT *hoding_reg = usSRegHoldBuf;
#endif

/* ── Register Callbacks ─────────────────────────────────────────────── */

eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
#if S_REG_INPUT_NREGS > 0
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex;

    usAddress--;  /* FreeModbus adds 1 before calling */

    if ((usAddress >= S_REG_INPUT_START) &&
        (usAddress + usNRegs <= S_REG_INPUT_START + S_REG_INPUT_NREGS))
    {
        iRegIndex = usAddress - usSRegInStart;
        /* TODO Phase 3: fill usSRegInBuf[21..40] from zigbee_service_get_params() */
        while (usNRegs > 0) {
            *pucRegBuffer++ = (UCHAR)(usSRegInBuf[iRegIndex] >> 8);
            *pucRegBuffer++ = (UCHAR)(usSRegInBuf[iRegIndex] & 0xFF);
            iRegIndex++;
            usNRegs--;
        }
    } else {
        eStatus = MB_ENOREG;
    }
    return eStatus;
#else
    return MB_ENOREG;
#endif
}

eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode)
{
#if S_REG_HOLDING_NREGS > 0
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex;

    usAddress--;

    if ((usAddress >= S_REG_HOLDING_START) &&
        (usAddress + usNRegs <= S_REG_HOLDING_START + S_REG_HOLDING_NREGS))
    {
        iRegIndex = usAddress - usSRegHoldStart;
        switch (eMode) {
        case MB_REG_READ:
            while (usNRegs > 0) {
                *pucRegBuffer++ = (UCHAR)(usSRegHoldBuf[iRegIndex] >> 8);
                *pucRegBuffer++ = (UCHAR)(usSRegHoldBuf[iRegIndex] & 0xFF);
                iRegIndex++;
                usNRegs--;
            }
            break;
        case MB_REG_WRITE:
            while (usNRegs > 0) {
                usSRegHoldBuf[iRegIndex]  = (USHORT)(*pucRegBuffer++) << 8;
                usSRegHoldBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
            }
            break;
        }
    } else {
        eStatus = MB_ENOREG;
    }
    return eStatus;
#else
    return MB_ENOREG;
#endif
}

eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode)
{
#if S_COIL_NCOILS > 0
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex, iRegBitIndex, iNReg;

    iNReg = usNCoils / 8 + 1;
    usAddress--;

    if ((usAddress >= S_COIL_START) &&
        (usAddress + usNCoils <= S_COIL_START + S_COIL_NCOILS))
    {
        iRegIndex    = (USHORT)(usAddress - usSCoilStart) / 8;
        iRegBitIndex = (USHORT)(usAddress - usSCoilStart) % 8;

        switch (eMode) {
        case MB_REG_READ:
            while (iNReg > 0) {
                *pucRegBuffer++ = xMBUtilGetBits(&ucSCoilBuf[iRegIndex++],
                                                 iRegBitIndex, 8);
                iNReg--;
            }
            pucRegBuffer--;
            usNCoils = usNCoils % 8;
            *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
            *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
            break;
        case MB_REG_WRITE:
            while (iNReg > 1) {
                xMBUtilSetBits(&ucSCoilBuf[iRegIndex++], iRegBitIndex, 8,
                               *pucRegBuffer++);
                iNReg--;
            }
            usNCoils = usNCoils % 8;
            if (usNCoils != 0) {
                xMBUtilSetBits(&ucSCoilBuf[iRegIndex++], iRegBitIndex,
                               usNCoils, *pucRegBuffer++);
            }
            break;
        }
    } else {
        eStatus = MB_ENOREG;
    }
    return eStatus;
#else
    return MB_ENOREG;
#endif
}

eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress,
                               USHORT usNDiscrete)
{
#if S_DISCRETE_INPUT_NDISCRETES > 0
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex, iRegBitIndex, iNReg;

    iNReg = usNDiscrete / 8 + 1;
    usAddress--;

    if ((usAddress >= S_DISCRETE_INPUT_START) &&
        (usAddress + usNDiscrete <= S_DISCRETE_INPUT_START + S_DISCRETE_INPUT_NDISCRETES))
    {
        iRegIndex    = (USHORT)(usAddress - usSDiscInStart) / 8;
        iRegBitIndex = (USHORT)(usAddress - usSDiscInStart) % 8;

        while (iNReg > 0) {
            *pucRegBuffer++ = xMBUtilGetBits(&ucSDiscInBuf[iRegIndex++],
                                             iRegBitIndex, 8);
            iNReg--;
        }
        pucRegBuffer--;
        usNDiscrete = usNDiscrete % 8;
        *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);
    } else {
        eStatus = MB_ENOREG;
    }
    return eStatus;
#else
    return MB_ENOREG;
#endif
}

void user_mb_app_poll(void)
{
    /* Reserved for DIO sync in Phase 1 completion / dioTask */
}

#endif /* MB_ASCII_ENABLED || MB_RTU_ENABLED || MB_TCP_ENABLED */