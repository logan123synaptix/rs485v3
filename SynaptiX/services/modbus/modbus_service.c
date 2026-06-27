/**
 * @file modbus_service.c
 * @brief Modbus RTU slave service implementation for RF_IO_RS485_V3.
 *
 * Init sequence:
 *   1. bsp_com_init()        — arm USART1 RX interrupt
 *   2. eMBInit()             — configure FreeModbus RTU, slave addr=1, port=1, 115200-8N1
 *   3. eMBEnable()           — enable protocol stack
 *   4. xTaskCreate()         — spawn modbus_task
 *
 * Task loop (every 20ms):
 *   - eMBPoll()              — process Modbus state machine
 *   - vMBPortSerialPoll()    — drain USART1 RX queue into Modbus RX FSM
 *   - user_mb_app_poll()     — update application register mirror
 */

#include "modbus_service.h"

#include "mb.h"
#include "port.h"
#include "bsp_uart.h"
#include "user_mb_app.h"

#include "FreeRTOS.h"
#include "task.h"
#include "app_freertos.h"

/* ── External symbols from portserial.c ──────────────────────────────────── */
extern eModbus modbus[N_MODBUS];
extern void vMBPortSerialPoll(eModbus_t modbus);

/* ── Task config ──────────────────────────────────────────────────────────── */
#define MODBUS_TASK_STACK_WORDS     1024
#define MODBUS_TASK_PRIORITY        osPriorityNormal
#define MODBUS_POLL_INTERVAL_MS     20

/* ── Slave config (D1: hardcoded) ─────────────────────────────────────────── */
#define MODBUS_SLAVE_ADDRESS        1
#define MODBUS_BAUD_RATE            115200

/* ── Task ─────────────────────────────────────────────────────────────────── */

void modbus_task(void *arg)
{
    (void)arg;
    for (;;) {
        eMBPoll(&modbus[0]);
        vMBPortSerialPoll(&modbus[0]);
        user_mb_app_poll();
        vTaskDelay(pdMS_TO_TICKS(MODBUS_POLL_INTERVAL_MS));
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void modbus_service_init(void)
{
    bsp_com_init();

    eMBInit(&modbus[0],
            MB_RTU,
            MODBUS_SLAVE_ADDRESS,
            BSP_RS485_COM_PORT,
            MODBUS_BAUD_RATE,
            MB_PAR_NONE);

    eMBEnable(&modbus[0]);

    xTaskCreate(modbus_task,
                "modbusTask",
                MODBUS_TASK_STACK_WORDS,
                NULL,
                MODBUS_TASK_PRIORITY,
                NULL);
}