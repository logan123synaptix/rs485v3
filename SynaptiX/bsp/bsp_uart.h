/**
 * @file bsp_uart.h
 * @brief UART / COM abstraction for RF_IO_RS485_V3
 *
 * Provides a CQueue-backed COM layer that mirrors the V2 board.h API
 * so that Modbus mbport, Shell and Zigbee can share the same BSP.
 *
 * COM port mapping:
 *   BSP_DEBUG_COM_PORT  = 0  → hlpuart1 (LPUART1, Shell / Logger)
 *   BSP_RS485_COM_PORT  = 1  → huart1   (USART1,  Modbus RTU)
 *   BSP_RF_COM_PORT     = 2  → huart2   (USART2,  Zigbee)
 */

#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h5xx_hal.h"
#include "cqueue.h"
#include <stdint.h>
#include <stdbool.h>

/* ── COM port indices ─────────────────────────────────────────────────── */
#define BSP_UART_NUM            3
#define BSP_DEBUG_COM_PORT      0   /* LPUART1 */
#define BSP_RS485_COM_PORT      1   /* USART1  */
#define BSP_RF_COM_PORT         2   /* USART2  */

#define MAX_UART_BUFF_SIZE      256  /* bytes, must be power-of-2 friendly */

/* ── TX callback type ─────────────────────────────────────────────────── */
typedef struct {
    void (*cb)(void *arg);
    void *arg;
} ComTxCallback_t;

/* ── Public API ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise BSP UART layer.
 *        Arms Receive_IT for LPUART1 and USART1 (1-byte mode).
 *        Arms ReceiveToIdle_IT for USART2 (Zigbee, burst mode).
 *        Must be called before any bsp_com_* function.
 */
void bsp_com_init(void);

/**
 * @brief Register a TX-complete callback for a COM port.
 *        The callback is invoked from HAL_UART_TxCpltCallback (ISR context).
 *        mbport portserial.c uses this to implement mb_uart_tx_task.
 */
void bsp_com_set_tx_callback(int com, void (*callback)(void *arg), void *arg);

/**
 * @brief Blocking write (polls until HAL_UART_Transmit completes).
 * @return Number of bytes written.
 */
uint32_t bsp_com_write(int com, uint8_t *buff, uint32_t len);

/**
 * @brief Non-blocking write via HAL_UART_Transmit_IT.
 *        TX-complete callback fires after all bytes sent.
 * @return Number of bytes written (len on success, 0 on error).
 */
uint32_t bsp_com_write_it(int com, uint8_t *buff, uint32_t len);

/**
 * @brief Read up to 'len' bytes from the RX CQueue.
 * @return Number of bytes actually read.
 */
uint32_t bsp_com_read(int com, uint8_t *buff, uint32_t len);

/**
 * @brief Return number of bytes available in the RX CQueue.
 */
uint32_t bsp_com_available(int com);

/**
 * @brief Return true if the UART TX path is idle (not busy).
 */
bool bsp_com_ready(int com);

/**
 * @brief Flush (discard) all bytes in the RX CQueue for a COM port.
 */
void bsp_com_flush(int com);

/* ── Internal HAL callback handlers (called from bsp_callbacks.c) ─────── */
void bsp_uart_rx_cplt_cb(UART_HandleTypeDef *huart);
void bsp_uart_rx_event_cb(UART_HandleTypeDef *huart, uint16_t size);
void bsp_uart_tx_cplt_cb(UART_HandleTypeDef *huart);

/* ── Timer 0 macros (TIM2, used by Modbus 3.5T) ──────────────────────── */
extern TIM_HandleTypeDef htim2;

#define bsp_timer0_start()   HAL_TIM_Base_Start_IT(&htim2)
#define bsp_timer0_stop()    HAL_TIM_Base_Stop_IT(&htim2)

/**
 * @brief Timer dispatch called from HAL_TIM_PeriodElapsedCallback.
 *        Routes TIM2 tick to the registered Modbus timer handler.
 */
void bsp_tim_period_elapsed_cb(TIM_HandleTypeDef *htim);

/**
 * @brief Register a function to call on every TIM2 tick.
 *        porttimer.c uses this to set the Modbus 3.5T handler.
 */
typedef void (*bsp_timer_cb_t)(void);
void bsp_timer_set_handle(int timer_idx, bsp_timer_cb_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */