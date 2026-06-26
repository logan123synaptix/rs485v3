/**
 * @file web_service.c
 * @brief Web service implementation for RF_IO_RS485_V3.
 *
 * Wraps Mongoose init/poll into a FreeRTOS task.
 * Transport: USB CDC 0 (RNDIS) → usb_net → lwIP → Mongoose HTTP port 80.
 */

#include "web_service.h"
#include "mongoose_glue.h"

#include "FreeRTOS.h"
#include "task.h"

/* ── Task config ──────────────────────────────────────────────────────────── */
#define WEB_TASK_STACK_WORDS    8192
#define WEB_TASK_PRIORITY       10

/* ── Task ─────────────────────────────────────────────────────────────────── */

static void web_server_task(void *arg)
{
    (void)arg;
    mongoose_init();
    for (;;) {
        mongoose_poll();
    }
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void web_service_init(void)
{
    xTaskCreate(web_server_task,
                "web_server_task",
                WEB_TASK_STACK_WORDS,
                NULL,
                WEB_TASK_PRIORITY,
                NULL);
}