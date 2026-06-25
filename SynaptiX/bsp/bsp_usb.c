/**
 * @file bsp_usb.c
 * @brief USB CDC BSP implementation for RF_IO_RS485_V3
 *
 * Thin wrapper around TinyUSB tud_cdc_n_* multi-interface API.
 * TinyUSB stack must be initialised (tud_init) before calling any function here.
 */

#include "bsp_usb.h"
#include "tusb.h"

/* ── Public API ───────────────────────────────────────────────────────────── */

bool bsp_usb_connected(uint8_t ch)
{
    return tud_cdc_n_connected(ch);
}

uint32_t bsp_usb_transmit(uint8_t ch, uint8_t *buf, uint32_t len)
{
    if (!tud_cdc_n_ready(ch)) {
        return 0;
    }
    uint32_t written = tud_cdc_n_write(ch, buf, len);
    tud_cdc_n_write_flush(ch);
    return written;
}

uint32_t bsp_usb_available(uint8_t ch)
{
    return tud_cdc_n_available(ch);
}

uint32_t bsp_usb_receiver(uint8_t ch, uint8_t *buf, uint32_t len)
{
    return tud_cdc_n_read(ch, buf, len);
}