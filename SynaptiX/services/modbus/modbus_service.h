/**
 * @file modbus_service.h
 * @brief Modbus RTU slave service for RF_IO_RS485_V3.
 *
 * Wraps FreeModbus init/poll into a FreeRTOS task.
 * Transport: USART1 (BSP_RS485_COM_PORT), 115200-8N1, RS485 half-duplex.
 * Slave address: hardcoded = 1 (D1).
 */

#ifndef MODBUS_SERVICE_H
#define MODBUS_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise Modbus RTU slave and spawn modbusTask.
 *        Call once from defaultTask after bsp_com_init().
 */
void modbus_service_init(void);

/**
 * @brief FreeRTOS task — runs eMBPoll + RX poll loop.
 *        Spawned internally by modbus_service_init().
 *        Stack: 1024 words, Priority: osPriorityNormal.
 */
void modbus_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_SERVICE_H */