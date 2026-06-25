#ifndef RF_APP_H
#define RF_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "zigbee.h"
#include "stdbool.h"

void rf_app_init();
void rf_app_poll();
bool rf_app_busy();

extern ZigbeeMesh_t zigbee;

#ifdef __cplusplus
}
#endif

#endif