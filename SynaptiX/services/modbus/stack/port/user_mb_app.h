/*
 * user_mb_app.h
 *
 *  Created on: Dec 16, 2023
 *      Author: MinhNhan
 */

#ifndef _USER_MB_APP_
#define _USER_MB_APP_

#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbutils.h"

/* -----------------------Slave Defines -------------------------------------*/
#define S_DISCRETE_INPUT_START        0
#define S_DISCRETE_INPUT_NDISCRETES   56
#if S_DISCRETE_INPUT_NDISCRETES > 0
extern UCHAR *input_coils;
#endif
#define S_COIL_START                  0
#define S_COIL_NCOILS                 56
#if S_COIL_NCOILS > 0
extern UCHAR *output_coils;
#endif
#define S_REG_INPUT_START             0
#define S_REG_INPUT_NREGS             32
#if S_REG_INPUT_NREGS > 0
extern USHORT *input_reg;
#endif
#define S_REG_HOLDING_START           0
#define S_REG_HOLDING_NREGS           172
#if S_REG_HOLDING_NREGS > 0
extern USHORT *hoding_reg;
#endif
/* salve mode: holding register's all address */
#define          S_HD_RESERVE                     0
#define          S_HD_CPU_USAGE_MAJOR             1
#define          S_HD_CPU_USAGE_MINOR             2
/* salve mode: input register's all address */
#define          S_IN_RESERVE                     0
/* salve mode: coil's all address */
#define          S_CO_RESERVE                     0
/* salve mode: discrete's all address */
#define          S_DI_RESERVE                     0

#endif // _USER_MB_APP_
