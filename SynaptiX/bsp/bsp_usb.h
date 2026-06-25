#ifndef BSP_USB_H
#define BSP_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"
#include "stdbool.h"

#define BSP_USB_SHELL_CH    1

bool bsp_usb_connected(uint8_t ch);
uint32_t bsp_usb_transmit(uint8_t ch, uint8_t *buf, uint32_t len);
uint32_t bsp_usb_available(uint8_t ch);
uint32_t bsp_usb_receiver(uint8_t ch, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif