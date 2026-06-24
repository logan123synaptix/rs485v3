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

/* ----------------------- Platform includes --------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "port.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

extern eModbus modbus_no1;
extern eModbus modbus_no2;

TIM_HandleTypeDef *timer[2]= {&htim1,&htim2};

/* ----------------------- static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit(eModbus_t modbus, USHORT usTim1Timerout50us )
{
    return TRUE;
}


inline void
vMBPortTimersEnable( eModbus_t modbus )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	HAL_TIM_Base_Start_IT(timer[modbus->config.ucPort]);
}

inline void
vMBPortTimersDisable( eModbus_t modbus )
{
    /* Disable any pending timers. */
	HAL_TIM_Base_Stop_IT(timer[modbus->config.ucPort]);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if(htim->Instance == htim1.Instance)
	{
		eModbus_t modbus = &modbus_no1;
		modbus->pxMBPortCBTimerExpired(modbus);
		return;
	}
	if(htim->Instance == htim2.Instance){
		eModbus_t modbus = &modbus_no2;
		modbus->pxMBPortCBTimerExpired(modbus);
		return;
	}

}
