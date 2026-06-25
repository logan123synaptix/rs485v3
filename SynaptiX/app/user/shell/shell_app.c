/**
 * @file shell_app.c
 * @brief Shell application — wires cli_shell to USB CDC channel 1.
 *
 * Transport:  USB CDC interface 1 (BSP_USB_SHELL_CH) via bsp_usb.
 * Task:       shell_receive_task — polls USB RX and feeds chars to cli_shell.
 * Priority:   configMAX_PRIORITIES - 1 (same as V2).
 * Stack:      512 words.
 */

#include "shell_app.h"
#include "cli_shell.h"
#include "bsp_usb.h"
#include "board.h"

#include "FreeRTOS.h"
#include "task.h"

/* ── Internal state ───────────────────────────────────────────────────────── */

static ShellContext_t s_shell;

/* ── Transport callbacks ──────────────────────────────────────────────────── */

static int shell_send_char(void *arg, char c)
{
    (void)arg;
    bsp_usb_transmit(BSP_USB_SHELL_CH, (uint8_t *)&c, 1);
    return 0;
}

static int shell_send_str(void *arg, char *s)
{
    (void)arg;
    if (s == NULL) return 0;
    uint32_t len = 0;
    while (s[len]) len++;
    bsp_usb_transmit(BSP_USB_SHELL_CH, (uint8_t *)s, len);
    return 0;
}

/* ── RX task ──────────────────────────────────────────────────────────────── */

static void shell_receive_task(void *arg)
{
    (void)arg;
    char c;

    /* Wait for USB host to connect before printing prompt */
    while (!bsp_usb_connected(BSP_USB_SHELL_CH)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    cli_shell_boot(&s_shell);

    while (1) {
        uint32_t avail = bsp_usb_available(BSP_USB_SHELL_CH);
        for (uint32_t i = 0; i < avail; i++) {
            if (bsp_usb_receiver(BSP_USB_SHELL_CH, (uint8_t *)&c, 1) == 1) {
                cli_shell_receive_char(&s_shell, c);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void shell_app_init(void)
{
    s_shell.impl.arg       = NULL;
    s_shell.impl.send_char = shell_send_char;
    s_shell.impl.send_str  = shell_send_str;

    xTaskCreate(shell_receive_task,
                "shellTask",
                512,
                NULL,
                configMAX_PRIORITIES - 1,
                NULL);
}