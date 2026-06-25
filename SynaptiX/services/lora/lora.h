#ifndef LORA_H
#define LORA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "cqueue.h"
#define LORA_BUFFER_SIZE 256
#define LORA_RES_SUCCESS 0x01
#define LORA_RES_FAIL 0x00
#define LORA_RES_TIMEOUT 0xFF

    typedef struct LoraParameters
    {
        uint8_t isConfiged;    // X3
        uint8_t pointType;     // X4
        uint16_t PAN_ID;       // X5 X6
        uint8_t Channel;       // X7
        uint8_t transferModel; // X8
        uint16_t userAddress;  // X9 X10
        uint16_t X11X12;       // X11 X12
        uint8_t uartBraudRate; // X13
        uint8_t uartDataBits;  // X14
        uint8_t uartStopBits;  // X15
        uint8_t uartParity;    // X16
        uint16_t X17X18;       // X17 X18
        uint8_t antennaSelect; // X19
        uint8_t macAddress[8]; // X20 - X27

        uint8_t prePointType;     // X28
        uint16_t prePAN_ID;       // X29 X30
        uint8_t preChannel;       // X31
        uint8_t preTransferModel; // X32
        uint16_t preUserAddress;  // X33 X34
        uint16_t X35X36;          // X35 X36
        uint8_t preUartBraudRate; // X37
        uint8_t preUartDataBits;  // X38
        uint8_t preUartStopBits;  // X39
        uint8_t preUartParity;    // X40
        uint16_t X41X42;          // X41 X42
        uint8_t preAntennaSelect; // X43

        uint8_t X44;              // X44
        uint8_t isSecurity;       // X45
        uint8_t securityCode[4];  // X46 - X49
        uint8_t loraTotalSetting; // X50

        uint16_t shortAddress;
    } LoraParameter_t;

    typedef struct LoRaMesh LoRaMesh_t;

    typedef void (*Lora_Callback_Func)(LoRaMesh_t *lora, uint8_t isSuccress, void *arg);

    typedef struct LoRaDriver
    {
        /* data */
        void (*write)(uint8_t *buff, uint32_t len);
        void (*read)(uint8_t *buff, uint32_t len);
        uint32_t (*available)();
    } LoRaDriver_t;

    typedef enum LoRaEventType
    {
        EVENT_NONE = 0,
        EVENT_LINK_MODULE,
        EVENT_READ_MODULE,
        EVENT_WRITE_MODULE,
        EVENT_RESET_MODULE,
        EVENT_ENTER_LOWPOWER,
        EVENT_GET_POSITION,
    } LoRaEventType_t;

    typedef struct LoRaEvent
    {
        /* data */
        uint32_t timeout;
        uint32_t tick;
        LoRaEventType_t event;
        Lora_Callback_Func cb;
        void *arg;
    } LoRaEvent_t;

    struct LoRaMesh
    {
        /* data */
        LoraParameter_t param;
        LoRaDriver_t *driver;
        LoRaEvent_t event;
        LoRaEvent_t event_tmp;
        LoRaEvent_t event_list[10];
        CQueue_t event_queue;
        uint8_t module_type;
        uint8_t fw_version;
        uint8_t buff[LORA_BUFFER_SIZE];
        uint32_t buff_len;
    };

    void lora_init(LoRaMesh_t *lora, LoRaDriver_t *driver);
    void lora_poll(LoRaMesh_t *lora, uint32_t timestamp);
    void lora_connect(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg);
    void lora_reset_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg);
    void lora_read_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg);
    void lora_write_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg);
    void lora_enter_lowpower(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg);

#ifdef __cplusplus
}
#endif

#endif