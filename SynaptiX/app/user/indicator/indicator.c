#include "indicator.h"
#include "board.h"
#include "string.h"

void led_on(int led_num)
{
    if(led_num == INDICATOR_STATUS) bsp_led_status_on();
    else if(led_num == INDICATOR_NET) bsp_led_net_on();
    else if (led_num == INDICATOR_POW) bsp_led_pow_on();
}
void led_off(int led_num)
{
    if(led_num == INDICATOR_STATUS) bsp_led_status_off();
    else if(led_num == INDICATOR_NET) bsp_led_net_on();
    else if (led_num == INDICATOR_POW) bsp_led_pow_off();
}
void led_toggle(int led_num)
{
    if(led_num == INDICATOR_STATUS) bsp_led_status_toggle();
    else if(led_num == INDICATOR_NET) bsp_led_net_toggle(); 
    else if (led_num == INDICATOR_POW) bsp_led_pow_toggle();
}
void indicator_init(Indicator_t *ind)
{
    if(ind == NULL) return;
    memset(ind,0,sizeof(Indicator_t));
    ind->on = led_on;
    ind->off = led_off;
    ind->toggle = led_toggle;
    ind->timer_net = 0;
    ind->timer_status = 0;
    ind->net_connect = 0;
    indicator_set_net_status(ind,EVENT_NET_DISCONNECTED);
}

void indicator_poll(Indicator_t *ind, uint32_t timestamp)
{
    ind->timer_status += timestamp;
    ind->on(INDICATOR_POW);
    if(ind->timer_status >= 200){
        ind->timer_status = 0;
        if(ind->toggle) ind->toggle(INDICATOR_STATUS);
    }
    ind->timer_net += timestamp;
    switch (ind->net_connect)
    {
    case EVENT_NET_CONNECTED:
        /* code */
        if(ind->timer_net >= 2000){
            ind->timer_net = 0;
            if(ind->on) ind->toggle(INDICATOR_NET);
        }
        break;
    case EVENT_NET_DISCONNECTED:
        /* code */
        if(ind->timer_net >= 1000){
            ind->timer_net = 0;
            if(ind->toggle) ind->toggle(INDICATOR_NET);
        }
        break;
    default:
        if(ind->off) ind->off(INDICATOR_NET);
        break;
    }
}