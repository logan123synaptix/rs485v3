/**
 * @file bsp_uart.c
 * @brief UART / COM abstraction implementation for RF_IO_RS485_V3
 *
 * Implements the CQueue-backed COM layer. All HAL weak callbacks are
 * overridden in bsp_callbacks.c — this file only provides the data
 * structures and helper functions called from those callbacks.
 */

#include "bsp_uart.h"
#include "cqueue.h"
#include <string.h>

/* ── Extern HAL handles (defined in Core/Src/usart.c) ─────────────────── */
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* ── Handle array — order MUST match BSP_*_COM_PORT constants ──────────── */
UART_HandleTypeDef *bsp_uart[BSP_UART_NUM] = {
    &hlpuart1,   /* [0] BSP_DEBUG_COM_PORT  — LPUART1 (Shell) */
    &huart1,     /* [1] BSP_RS485_COM_PORT  — USART1  (Modbus) */
    &huart2,     /* [2] BSP_RF_COM_PORT     — USART2  (Zigbee) */
};

/* ── RX queues & shadow buffers ─────────────────────────────────────────── */
static uint8_t  s_rx_buf[BSP_UART_NUM][MAX_UART_BUFF_SIZE];
static CQueue_t s_rx_queue[BSP_UART_NUM];

/* Single-byte IT receive buffers for LPUART1 and USART1 */
uint8_t uart_data[BSP_UART_NUM];

/* Burst-receive buffer for USART2 (ReceiveToIdle_IT) */
uint8_t uart_data_dma[MAX_UART_BUFF_SIZE];

/* ── TX callbacks ─────────────────────────────────────────────────────── */
ComTxCallback_t com_tx_callback[BSP_UART_NUM];

/* ── Timer callbacks ─────────────────────────────────────────────────── */
static bsp_timer_cb_t s_timer_cb[1]; /* index 0 = TIM2 / TIMER0 */

/* ── bsp_com_init ─────────────────────────────────────────────────────── */
void bsp_com_init(void)
{
    for (int i = 0; i < BSP_UART_NUM; i++) {
        cqueue_init_static(&s_rx_queue[i], s_rx_buf[i],
                           MAX_UART_BUFF_SIZE, sizeof(uint8_t));
        com_tx_callback[i].cb  = NULL;
        com_tx_callback[i].arg = NULL;
    }

    /* Arm 1-byte IT mode for Shell (LPUART1) and Modbus (USART1) */
    HAL_UART_Receive_IT(bsp_uart[BSP_DEBUG_COM_PORT],
                        &uart_data[BSP_DEBUG_COM_PORT], 1);
    HAL_UART_Receive_IT(bsp_uart[BSP_RS485_COM_PORT],
                        &uart_data[BSP_RS485_COM_PORT], 1);

    /* Arm burst IT mode for Zigbee (USART2) — no GPDMA needed */
    HAL_UARTEx_ReceiveToIdle_IT(bsp_uart[BSP_RF_COM_PORT],
                                uart_data_dma, MAX_UART_BUFF_SIZE);
}

/* ── Called from HAL_UART_RxCpltCallback (bsp_callbacks.c) ────────────── */
void bsp_uart_rx_cplt_cb(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < BSP_UART_NUM; i++) {
        if (bsp_uart[i] == huart) {
            /* Only for 1-byte IT ports (LPUART1 and USART1) */
            if (i == BSP_RF_COM_PORT) break; /* USART2 uses RxEvent path */
            cqueue_send(&s_rx_queue[i], &uart_data[i]);
            /* Re-arm */
            HAL_UART_Receive_IT(bsp_uart[i], &uart_data[i], 1);
            break;
        }
    }
}

/* ── Called from HAL_UARTEx_RxEventCallback (bsp_callbacks.c) ─────────── */
void bsp_uart_rx_event_cb(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart == bsp_uart[BSP_RF_COM_PORT]) {
        for (uint16_t k = 0; k < size; k++) {
            cqueue_send(&s_rx_queue[BSP_RF_COM_PORT], &uart_data_dma[k]);
        }
        /* Re-arm */
        HAL_UARTEx_ReceiveToIdle_IT(bsp_uart[BSP_RF_COM_PORT],
                                    uart_data_dma, MAX_UART_BUFF_SIZE);
    }
}

/* ── Called from HAL_UART_TxCpltCallback (bsp_callbacks.c) ────────────── */
void bsp_uart_tx_cplt_cb(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < BSP_UART_NUM; i++) {
        if (bsp_uart[i] == huart) {
            if (com_tx_callback[i].cb != NULL) {
                com_tx_callback[i].cb(com_tx_callback[i].arg);
            }
            break;
        }
    }
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void bsp_com_set_tx_callback(int com, void (*callback)(void *arg), void *arg)
{
    if (com < 0 || com >= BSP_UART_NUM) return;
    com_tx_callback[com].cb  = callback;
    com_tx_callback[com].arg = arg;
}

uint32_t bsp_com_write(int com, uint8_t *buff, uint32_t len)
{
    if (com < 0 || com >= BSP_UART_NUM) return 0;
    if (HAL_UART_Transmit(bsp_uart[com], buff, (uint16_t)len, 1000) == HAL_OK) {
        return len;
    }
    return 0;
}

uint32_t bsp_com_write_it(int com, uint8_t *buff, uint32_t len)
{
    if (com < 0 || com >= BSP_UART_NUM) return 0;
    if (HAL_UART_Transmit_IT(bsp_uart[com], buff, (uint16_t)len) == HAL_OK) {
        return len;
    }
    return 0;
}

uint32_t bsp_com_read(int com, uint8_t *buff, uint32_t len)
{
    if (com < 0 || com >= BSP_UART_NUM) return 0;
    uint32_t count = 0;
    while (count < len && cqueue_receive(&s_rx_queue[com], &buff[count])) {
        count++;
    }
    return count;
}

uint32_t bsp_com_available(int com)
{
    if (com < 0 || com >= BSP_UART_NUM) return 0;
    return (uint32_t)s_rx_queue[com].count;
}

bool bsp_com_ready(int com)
{
    if (com < 0 || com >= BSP_UART_NUM) return false;
    return (bsp_uart[com]->gState == HAL_UART_STATE_READY);
}

void bsp_com_flush(int com)
{
    if (com < 0 || com >= BSP_UART_NUM) return;
    uint8_t dummy;
    while (cqueue_receive(&s_rx_queue[com], &dummy)) {}
}

/* ── Timer API ─────────────────────────────────────────────────────────── */

void bsp_timer_set_handle(int timer_idx, bsp_timer_cb_t handle)
{
    if (timer_idx == 0) {
        s_timer_cb[0] = handle;
    }
}

void bsp_tim_period_elapsed_cb(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (s_timer_cb[0] != NULL) {
            s_timer_cb[0]();
        }
    }
}