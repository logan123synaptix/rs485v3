/**
 * @file bsp_gpio.h
 * @brief GPIO pin aliases and DI/DO abstraction for RF_IO_RS485_V3
 *
 * V3 main.h does NOT define pin aliases (unlike V2).
 * All pin definitions live here.
 *
 * Pin mapping (from .ioc / gpio.c):
 *   DO1→PA6  DO2→PA7  DO3→PB0  DO4→PB1
 *   DI1→PB6  DI2→PB5  DI3→PB4  DI4→PB3
 *   DE →PB13 (RS485 driver enable, active HIGH)
 *   LED0→PC13  LED1→PC14  LED2→PC15
 */

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h5xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Output (DO) pins ─────────────────────────────────────────────────── */
#define BSP_DO1_PORT    GPIOA
#define BSP_DO1_PIN     GPIO_PIN_6
#define BSP_DO2_PORT    GPIOA
#define BSP_DO2_PIN     GPIO_PIN_7
#define BSP_DO3_PORT    GPIOB
#define BSP_DO3_PIN     GPIO_PIN_0
#define BSP_DO4_PORT    GPIOB
#define BSP_DO4_PIN     GPIO_PIN_1

/* ── Input (DI) pins ──────────────────────────────────────────────────── */
#define BSP_DI1_PORT    GPIOB
#define BSP_DI1_PIN     GPIO_PIN_6
#define BSP_DI2_PORT    GPIOB
#define BSP_DI2_PIN     GPIO_PIN_5
#define BSP_DI3_PORT    GPIOB
#define BSP_DI3_PIN     GPIO_PIN_4
#define BSP_DI4_PORT    GPIOB
#define BSP_DI4_PIN     GPIO_PIN_3

/* ── RS485 DE pin ─────────────────────────────────────────────────────── */
#define BSP_RS485_DE_PORT   GPIOB
#define BSP_RS485_DE_PIN    GPIO_PIN_13

/* ── LED pins (active HIGH on V3) ─────────────────────────────────────── */
#define BSP_LED0_PORT   GPIOC
#define BSP_LED0_PIN    GPIO_PIN_13
#define BSP_LED1_PORT   GPIOC
#define BSP_LED1_PIN    GPIO_PIN_14
#define BSP_LED2_PORT   GPIOC
#define BSP_LED2_PIN    GPIO_PIN_15

/* ── Count macros ─────────────────────────────────────────────────────── */
#define BSP_OUTPUT_NUM  4
#define BSP_INPUT_NUM   4
#define BSP_LED_NUM     3

/* ── RS485 DE control ─────────────────────────────────────────────────── */
static inline void bsp_rs485_de_on(void) {
    HAL_GPIO_WritePin(BSP_RS485_DE_PORT, BSP_RS485_DE_PIN, GPIO_PIN_SET);
}
static inline void bsp_rs485_de_off(void) {
    HAL_GPIO_WritePin(BSP_RS485_DE_PORT, BSP_RS485_DE_PIN, GPIO_PIN_RESET);
}

/* ── LED control ──────────────────────────────────────────────────────── */
#define bsp_led0_on()      HAL_GPIO_WritePin(BSP_LED0_PORT, BSP_LED0_PIN, GPIO_PIN_SET)
#define bsp_led0_off()     HAL_GPIO_WritePin(BSP_LED0_PORT, BSP_LED0_PIN, GPIO_PIN_RESET)
#define bsp_led0_toggle()  HAL_GPIO_TogglePin(BSP_LED0_PORT, BSP_LED0_PIN)

#define bsp_led1_on()      HAL_GPIO_WritePin(BSP_LED1_PORT, BSP_LED1_PIN, GPIO_PIN_SET)
#define bsp_led1_off()     HAL_GPIO_WritePin(BSP_LED1_PORT, BSP_LED1_PIN, GPIO_PIN_RESET)
#define bsp_led1_toggle()  HAL_GPIO_TogglePin(BSP_LED1_PORT, BSP_LED1_PIN)

#define bsp_led2_on()      HAL_GPIO_WritePin(BSP_LED2_PORT, BSP_LED2_PIN, GPIO_PIN_SET)
#define bsp_led2_off()     HAL_GPIO_WritePin(BSP_LED2_PORT, BSP_LED2_PIN, GPIO_PIN_RESET)
#define bsp_led2_toggle()  HAL_GPIO_TogglePin(BSP_LED2_PORT, BSP_LED2_PIN)

/* ── DI/DO API ────────────────────────────────────────────────────────── */

/**
 * @brief Read digital input state.
 * @param input_num  0-based index (0=DI1 … 3=DI4)
 * @return 1 if HIGH, 0 if LOW, -1 if invalid index
 */
int bsp_get_input(int input_num);

/**
 * @brief Set digital output HIGH.
 * @param output_num  0-based index (0=DO1 … 3=DO4)
 * @return 0 on success, -1 if invalid index
 */
int bsp_output_on(int output_num);

/**
 * @brief Set digital output LOW.
 * @param output_num  0-based index (0=DO1 … 3=DO4)
 * @return 0 on success, -1 if invalid index
 */
int bsp_output_off(int output_num);

/**
 * @brief Toggle digital output.
 * @param output_num  0-based index (0=DO1 … 3=DO4)
 * @return 0 on success, -1 if invalid index
 */
int bsp_output_toggle(int output_num);

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPIO_H */