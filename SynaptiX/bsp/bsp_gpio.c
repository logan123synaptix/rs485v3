/**
 * @file bsp_gpio.c
 * @brief GPIO DI/DO implementation for RF_IO_RS485_V3
 */

#include "bsp_gpio.h"

/* ── Pin lookup tables ──────────────────────────────────────────────────── */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
} GpioPin_t;

static const GpioPin_t s_do_pins[BSP_OUTPUT_NUM] = {
    {BSP_DO1_PORT, BSP_DO1_PIN},  /* DO1 / OUT0 */
    {BSP_DO2_PORT, BSP_DO2_PIN},  /* DO2 / OUT1 */
    {BSP_DO3_PORT, BSP_DO3_PIN},  /* DO3 / OUT2 */
    {BSP_DO4_PORT, BSP_DO4_PIN},  /* DO4 / OUT3 */
};

static const GpioPin_t s_di_pins[BSP_INPUT_NUM] = {
    {BSP_DI1_PORT, BSP_DI1_PIN},  /* DI1 / IN0 */
    {BSP_DI2_PORT, BSP_DI2_PIN},  /* DI2 / IN1 */
    {BSP_DI3_PORT, BSP_DI3_PIN},  /* DI3 / IN2 */
    {BSP_DI4_PORT, BSP_DI4_PIN},  /* DI4 / IN3 */
};

/* ── Implementation ─────────────────────────────────────────────────────── */

int bsp_get_input(int input_num)
{
    if (input_num < 0 || input_num >= BSP_INPUT_NUM) {
        return -1;
    }
    return (HAL_GPIO_ReadPin(s_di_pins[input_num].port,
                             s_di_pins[input_num].pin) == GPIO_PIN_SET) ? 1 : 0;
}

int bsp_output_on(int output_num)
{
    if (output_num < 0 || output_num >= BSP_OUTPUT_NUM) {
        return -1;
    }
    HAL_GPIO_WritePin(s_do_pins[output_num].port,
                      s_do_pins[output_num].pin,
                      GPIO_PIN_SET);
    return 0;
}

int bsp_output_off(int output_num)
{
    if (output_num < 0 || output_num >= BSP_OUTPUT_NUM) {
        return -1;
    }
    HAL_GPIO_WritePin(s_do_pins[output_num].port,
                      s_do_pins[output_num].pin,
                      GPIO_PIN_RESET);
    return 0;
}

int bsp_output_toggle(int output_num)
{
    if (output_num < 0 || output_num >= BSP_OUTPUT_NUM) {
        return -1;
    }
    HAL_GPIO_TogglePin(s_do_pins[output_num].port,
                       s_do_pins[output_num].pin);
    return 0;
}