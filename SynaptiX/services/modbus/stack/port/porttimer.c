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
#include "bsp_timer.h"

extern eModbus modbus[N_MODBUS];

TimerHandle_t mb_timer[N_MODBUS];

/* ----------------------- static functions ---------------------------------*/
static void mb_timer_callback(void *arg){
	for(int i = 0;i<N_MODBUS;i++){
		if(arg == (void*) modbus[i].timer_handle)
		{
			modbus[i].pxMBPortCBTimerExpired(&modbus[i]);
			return;
		}
	}
}

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit(eModbus_t modbus, USHORT usTim1Timerout50us )
{
	modbus->timer_handle = (void*) &mb_timer[modbus->config.ucPort];
	bsp_timer_set_callback((TimerHandle_t*)modbus->timer_handle, mb_timer_callback, modbus->timer_handle);
    return TRUE;
}


inline void
vMBPortTimersEnable( eModbus_t modbus )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
//	modbus->timer_start(modbus->timer_handle);
	bsp_timer_start(modbus->timer_handle);
}

inline void
vMBPortTimersDisable( eModbus_t modbus )
{
    /* Disable any pending timers. */
//	modbus->timer_stop(modbus->timer_handle);
	bsp_timer_stop(modbus->timer_handle);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */

