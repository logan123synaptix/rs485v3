/**
 * @file web_service.h
 * @brief Web service — wraps Mongoose HTTP server for RF_IO_RS485_V3.
 *
 * Provides a thin service layer over Mongoose (mongoose_init / mongoose_poll).
 * Transport: USB RNDIS (usb_net) → lwIP → Mongoose HTTP on port 80.
 *
 * Usage:
 *   web_service_init();   // call once from app init / defaultTask
 */

#ifndef WEB_SERVICE_H
#define WEB_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise Mongoose and spawn web_server_task.
 *        Must be called after usb_netif_init() and lwIP are ready.
 */
void web_service_init(void);

#ifdef __cplusplus
}
#endif

#endif /* WEB_SERVICE_H */