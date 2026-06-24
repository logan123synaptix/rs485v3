/**
 * @file bsp_callbacks.c
 * @brief Single file overriding ALL HAL weak callbacks for RF_IO_RS485_V3
 *
 * Rule: ONE implementation per callback, mux to sub-systems here.
 * Never override these callbacks anywhere else in the project.
 */

#include "stm32h5xx_hal.h"
#include "bsp_uart.h"

/* ── UART RX (1-byte IT mode: LPUART1 and USART1) ──────────────────────── */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_rx_cplt_cb(huart);
}

/* ── UART RX Event (ReceiveToIdle IT mode: USART2 / Zigbee) ────────────── */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    bsp_uart_rx_event_cb(huart, Size);
}

/* ── UART TX complete ────────────────────────────────────────────────────── */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bsp_uart_tx_cplt_cb(huart);
}

/* ── UART errors — re-arm to keep receive alive after noise/overrun ─────── */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    extern UART_HandleTypeDef hlpuart1;
    extern UART_HandleTypeDef huart1;
    extern UART_HandleTypeDef huart2;
    extern uint8_t uart_data[BSP_UART_NUM];
    extern uint8_t uart_data_dma[MAX_UART_BUFF_SIZE];

    if (huart == &hlpuart1) {
        HAL_UART_Receive_IT(&hlpuart1, &uart_data[BSP_DEBUG_COM_PORT], 1);
    } else if (huart == &huart1) {
        HAL_UART_Receive_IT(&huart1, &uart_data[BSP_RS485_COM_PORT], 1);
    } else if (huart == &huart2) {
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, uart_data_dma, MAX_UART_BUFF_SIZE);
    }
}