#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_gpio.h"
#include "bsp_uart.h"
#include "tim.h"
#include "logger.h"
#include "stm32h5xx_hal.h"

#define BSP_RS485_RF_TRANSFER_MODE  1
#define BSP_LOG_LEVEL               LOGGER_DEBUG

#define BSP_ANALOG_ENABLE           0

#if BSP_ANALOG_ENABLE
#define BSP_NUM_ADC_CHANNEL         5
#endif

#define bsp_led_status_on()         bsp_led0_on()
#define bsp_led_status_off()        bsp_led0_off()
#define bsp_led_status_toggle()     bsp_led0_toggle()

#define bsp_led_net_on()            bsp_led1_on()
#define bsp_led_net_off()           bsp_led1_off()
#define bsp_led_net_toggle()        bsp_led1_toggle()

#define bsp_led_pow_on()            bsp_led2_on()
#define bsp_led_pow_off()           bsp_led2_off()
#define bsp_led_pow_toggle()        bsp_led2_toggle()

#define bsp_gettick()               HAL_GetTick()
#define bsp_restart()               HAL_NVIC_SystemReset()
// #define bsp_delay(x)                HAL_Delay(x)


#ifdef __cplusplus
}
#endif

#endif