#ifndef INDICATOR_H
#define INDICATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"

#define INDICATOR_STATUS    0
#define INDICATOR_NET       1
#define INDICATOR_POW       2

#define EVENT_INDICATOR_STATUS_ON   0x01
#define EVENT_NET_CONNECTED         0x02
#define EVENT_NET_DISCONNECTED      0x03

typedef struct Indicator
{
    /*  data    */
    void (*on)      (int indicator_num);
    void (*off)     (int indicator_num);
    void (*toggle)  (int indicator_num);      
    uint32_t timer_status;
    uint32_t timer_net;
    uint8_t net_connect;

} Indicator_t;

void indicator_poll(Indicator_t *ind, uint32_t timestamp);

static inline void indicator_set_net_status(Indicator_t *ind, uint8_t status){
    if(ind == NULL) return;
    ind->net_connect = status;
}

static inline void indicator_on(Indicator_t indicator, int indicator_num){
    if(indicator.on) indicator.on(indicator_num);
}

static inline void indicator_off(Indicator_t indicator, int indicator_num){
    if(indicator.off) indicator.off(indicator_num);
}

static inline void indicator_toggle(Indicator_t indicator, int indicator_num){
    if(indicator.toggle) indicator.toggle(indicator_num);
}

#ifdef __cplusplus
}
#endif

#endif