#include "lora.h"
#include <string.h>
#include <stdio.h>

#include "logger.h"

static const char *TAG = "LoRa";

static uint8_t LoraINS_LinkModule[7] = {0xFC, 0x03, 0x01, 0x44, 0x54, 0x4B, 0xE3};
static uint8_t LoraINS_ReadModule[7] = {0xFC, 0x03, 0x02, 0x44, 0x54, 0x4B, 0xE4};
static uint8_t LoraINS_ResetModule[7] = {0xFC, 0x03, 0x04, 0x44, 0x54, 0x4B, 0xE6};

static uint8_t LoraINS_QueryPosition_Network[7] = {0xFC, 0x03, 0x05, 0x44, 0x54, 0x4B, 0xE7};

static uint8_t LoraINS_RemotReadModule[9] = {0xFC, 0x05, 0x06, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0xFF};
static uint8_t LoraINS_RemotRestartModule[9] = {0xFC, 0x05, 0x07, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0xFF};
static uint8_t LoraINS_RemotWriteModule[21] = {0xFC, 0x11, 0x08, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t LoraINS_EnterLowPower[7] = {0xFC, 0x03, 0x09, 0x44, 0x54, 0x4B, 0xEB};
static uint8_t LoraINS_WriteModule[47];

// static uint8_t LoraINS_LinkModule[7] = {0xFC, 0x06, 0x04, 0x44, 0x54, 0x4B, 0x52,0x46, 0x81};
// static uint8_t LoraINS_ReadModule[7] = {0xFC, 0x03, 0x02, 0x44, 0x54, 0x4B, 0xE4};
// static uint8_t LoraINS_ResetModule[7] = {0xFC, 0x03, 0x04, 0x44, 0x54, 0x4B, 0xE6};

// static uint8_t LoraINS_QueryPosition_Network[7] = {0xFC, 0x03, 0x05, 0x44, 0x54, 0x4B, 0xE7};

// static uint8_t LoraINS_RemotReadModule[9] = {0xFC, 0x05, 0x06, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0xFF};
// static uint8_t LoraINS_RemotRestartModule[9] = {0xFC, 0x05, 0x07, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0xFF};
// static uint8_t LoraINS_RemotWriteModule[21] = {0xFC, 0x11, 0x08, 0x44, 0x54, 0x4B, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// static uint8_t LoraINS_EnterLowPower[7] = {0xFC, 0x03, 0x09, 0x44, 0x54, 0x4B, 0xEB};
// static uint8_t LoraINS_WriteModule[47];

static uint8_t getXY(uint8_t *inputData, uint16_t inputLen)
{
    uint8_t result = 0;
    uint16_t i = 0;

    for (i = 0; i < inputLen - 1; i++)
    {
        result = result + inputData[i];
    }
    return result;
}

static void lora_get_write_ins(LoRaMesh_t *this)
{
    uint8_t tempHead = 6;
    uint8_t temp1 = 0, temp2 = 0;

    LoraINS_WriteModule[0] = 0xFC;
    LoraINS_WriteModule[1] = 43; // Length
    LoraINS_WriteModule[2] = 3;

    LoraINS_WriteModule[3] = 0x44; // fix value
    LoraINS_WriteModule[4] = 0x54; // fix value
    LoraINS_WriteModule[5] = 0x4B; // fix value

    LoraINS_WriteModule[tempHead + 0] = this->param.isConfiged;
    LoraINS_WriteModule[tempHead + 1] = this->param.pointType;

    temp1 = this->param.PAN_ID / 256;
    temp2 = this->param.PAN_ID % 256;
    LoraINS_WriteModule[tempHead + 2] = temp1;
    LoraINS_WriteModule[tempHead + 3] = temp2;

    LoraINS_WriteModule[tempHead + 4] = this->param.Channel;
    LoraINS_WriteModule[tempHead + 5] = this->param.transferModel;

    temp1 = this->param.userAddress / 256;
    temp2 = this->param.userAddress % 256;
    LoraINS_WriteModule[tempHead + 6] = temp1;
    LoraINS_WriteModule[tempHead + 7] = temp2;

    temp1 = this->param.X11X12 / 256;
    temp2 = this->param.X11X12 % 256;
    LoraINS_WriteModule[tempHead + 8] = temp1;
    LoraINS_WriteModule[tempHead + 9] = temp2;

    LoraINS_WriteModule[tempHead + 10] = this->param.uartBraudRate;
    LoraINS_WriteModule[tempHead + 11] = this->param.uartDataBits;
    LoraINS_WriteModule[tempHead + 12] = this->param.uartStopBits;
    LoraINS_WriteModule[tempHead + 13] = this->param.uartParity;

    temp1 = this->param.X17X18 / 256;
    temp2 = this->param.X17X18 % 256;
    LoraINS_WriteModule[tempHead + 14] = temp1;
    LoraINS_WriteModule[tempHead + 15] = temp2;

    LoraINS_WriteModule[tempHead + 16] = this->param.antennaSelect;

    //-------------------------------------------------
    LoraINS_WriteModule[tempHead + 17] = this->param.prePointType;

    temp1 = this->param.prePAN_ID / 256;
    temp2 = this->param.prePAN_ID % 256;
    LoraINS_WriteModule[tempHead + 18] = temp1;
    LoraINS_WriteModule[tempHead + 19] = temp2;

    LoraINS_WriteModule[tempHead + 20] = this->param.preChannel;
    LoraINS_WriteModule[tempHead + 21] = this->param.preTransferModel;

    temp1 = this->param.preUserAddress / 256;
    temp2 = this->param.preUserAddress % 256;
    LoraINS_WriteModule[tempHead + 22] = temp1;
    LoraINS_WriteModule[tempHead + 23] = temp2;

    temp1 = this->param.X35X36 / 256;
    temp2 = this->param.X35X36 % 256;
    LoraINS_WriteModule[tempHead + 24] = temp1;
    LoraINS_WriteModule[tempHead + 25] = temp2;

    LoraINS_WriteModule[tempHead + 26] = this->param.preUartBraudRate;
    LoraINS_WriteModule[tempHead + 27] = this->param.preUartDataBits;
    LoraINS_WriteModule[tempHead + 28] = this->param.preUartStopBits;
    LoraINS_WriteModule[tempHead + 29] = this->param.preUartParity;

    temp1 = this->param.X41X42 / 256;
    temp2 = this->param.X41X42 % 256;
    LoraINS_WriteModule[tempHead + 30] = temp1;
    LoraINS_WriteModule[tempHead + 31] = temp2;

    LoraINS_WriteModule[tempHead + 32] = this->param.preAntennaSelect;
    LoraINS_WriteModule[tempHead + 33] = this->param.X44;

    LoraINS_WriteModule[tempHead + 34] = this->param.isSecurity;
    LoraINS_WriteModule[tempHead + 35] = this->param.securityCode[0];
    LoraINS_WriteModule[tempHead + 36] = this->param.securityCode[1];
    LoraINS_WriteModule[tempHead + 37] = this->param.securityCode[2];
    LoraINS_WriteModule[tempHead + 38] = this->param.securityCode[3];

    LoraINS_WriteModule[tempHead + 39] = this->param.loraTotalSetting;

    LoraINS_WriteModule[tempHead + 40] = getXY(LoraINS_WriteModule, 47);
}

static uint8_t lora_read_modlue_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    uint8_t result = 0;

    uint8_t tempXY = 0;
    uint8_t tempRight = 0;
    uint8_t i = 0;
    uint8_t tempHead = 4;

    tempXY = getXY(inputData, inputLength);

    if (inputData[0] == 0xFA)
    {
        tempRight++;
    }
    if (inputData[1] == 0x31)
    {
        tempRight++;
    }
    if (inputData[2] == 0x0A)
    {
        tempRight++;
    }
    if (inputData[inputLength - 1] == tempXY)
    {
        tempRight++;
    }

    if (tempRight == 4)
    {
        this->param.isConfiged = inputData[tempHead + 0];
        this->param.pointType = inputData[tempHead + 1];
        this->param.PAN_ID = inputData[tempHead + 2] * 256 + inputData[tempHead + 3];
        this->param.Channel = inputData[tempHead + 4];
        this->param.transferModel = inputData[tempHead + 5];
        this->param.userAddress = inputData[tempHead + 6] * 256 + inputData[tempHead + 7];
        this->param.X11X12 = inputData[tempHead + 8] * 256 + inputData[tempHead + 9];
        this->param.uartBraudRate = inputData[tempHead + 10];
        this->param.uartDataBits = inputData[tempHead + 11];
        this->param.uartStopBits = inputData[tempHead + 12];
        this->param.uartParity = inputData[tempHead + 13];
        this->param.X17X18 = inputData[tempHead + 14] * 256 + inputData[tempHead + 15];
        this->param.antennaSelect = inputData[tempHead + 16];

        for (i = 0; i < 8; i++)
        {
            this->param.macAddress[i] = inputData[tempHead + 17 + i];
        }

        this->param.prePointType = inputData[tempHead + 25];
        this->param.prePAN_ID = inputData[tempHead + 26] * 256 + inputData[tempHead + 27];
        this->param.preChannel = inputData[tempHead + 28];
        this->param.preTransferModel = inputData[tempHead + 29];
        this->param.preUserAddress = inputData[tempHead + 30] * 256 + inputData[tempHead + 31];
        this->param.X35X36 = inputData[tempHead + 32] * 256 + inputData[tempHead + 33];
        this->param.preUartBraudRate = inputData[tempHead + 34];
        this->param.preUartDataBits = inputData[tempHead + 35];
        this->param.preUartStopBits = inputData[tempHead + 36];
        this->param.preUartParity = inputData[tempHead + 37];
        this->param.X41X42 = inputData[tempHead + 38] * 256 + inputData[tempHead + 39];
        this->param.preAntennaSelect = inputData[tempHead + 40];

        this->param.X44 = inputData[tempHead + 41];
        this->param.isSecurity = inputData[tempHead + 42];

        for (i = 0; i < 4; i++)
        {
            this->param.securityCode[i] = inputData[tempHead + 43 + i];
        }

        this->param.loraTotalSetting = inputData[tempHead + 47];

        this->param.shortAddress = this->param.macAddress[5] * 256 + this->param.macAddress[7];

        result = 1;
    }
    return result;
}

static uint8_t lora_write_modlue_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    uint8_t write_response[8] = {0xFA, 0x04, 0x0A, 0x03, 0x44, 0x54, 0x4B, 0xEE};
    if (memcmp(inputData, write_response, 8) == 0)
        return LORA_RES_SUCCESS;
    return LORA_RES_FAIL;
}
static uint8_t lora_reset_modlue_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    uint8_t restart_response[8] = {0xFA, 0x04, 0x0A, 0x04, 0x44, 0x54, 0x4B, 0xEF};
    if (memcmp(inputData, restart_response, 7) == 0)
        return LORA_RES_SUCCESS;
    return LORA_RES_FAIL;
}
static uint8_t lora_link_modlue_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    uint8_t result = 0;

    uint8_t tempXY = 0;
    uint8_t tempRight = 0;
    // uint8_t i = 0;
    uint8_t tempHead = 4;

    tempXY = getXY(inputData, inputLength);

    if (inputData[0] == 0xFA)
    {
        tempRight++;
    }
    if (inputData[1] == 0x06)
    {
        tempRight++;
    }
    if (inputData[2] == 0x0A)
    {
        tempRight++;
    }
    if (inputData[inputLength - 1] == tempXY)
    {
        tempRight++;
    }
    if (tempRight == 4)
    {
        result = 1;
        this->module_type = inputData[tempHead + 0];
        this->fw_version = inputData[tempHead + 1];
    }
    if (result)
        return LORA_RES_SUCCESS;
    return LORA_RES_FAIL;
}
static uint8_t lora_enter_lowpower_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    uint8_t lowpower_response[8] = {0xFA, 0x04, 0x0A, 0x09, 0x44, 0x54, 0x4B, 0xF4};
    if (memcmp(inputData, lowpower_response, 8) == 0)
        return LORA_RES_SUCCESS;
    return LORA_RES_FAIL;
}
static uint8_t lora_get_position_process(LoRaMesh_t *this, uint8_t *inputData, uint16_t inputLength)
{
    return LORA_RES_FAIL;
}

void lora_init(LoRaMesh_t *lora, LoRaDriver_t *driver)
{
    lora->driver = driver;
    memset(&lora->param, 0, sizeof(LoraParameter_t));
    memset(&lora->event, 0, sizeof(LoRaEvent_t));
    memset(&lora->event_tmp, 0, sizeof(LoRaEvent_t));
    cqueue_init_static(&lora->event_queue, lora->event_list,sizeof(lora->event_list),sizeof(LoRaEvent_t));
    log_info(TAG, "LoRa Init");
}

static void lora_send_connect_cmd(LoRaMesh_t *lora)
{
    lora->driver->write(LoraINS_LinkModule, 7);
}
static void lora_send_read_cmd(LoRaMesh_t *lora)
{
    lora->driver->write(LoraINS_ReadModule, 7);
}
static void lora_send_reset_cmd(LoRaMesh_t *lora)
{
    lora->driver->write(LoraINS_ResetModule, 7);
}
static void lora_send_lowpower_cmd(LoRaMesh_t *lora)
{
    lora->driver->write(LoraINS_EnterLowPower, 7);
}
static void lora_send_write_cmd(LoRaMesh_t *lora)
{
    lora_get_write_ins(lora);
    lora->driver->write(LoraINS_WriteModule, 47);
}

void lora_poll(LoRaMesh_t *lora, uint32_t timestamp)
{
    if (lora->event.event == EVENT_NONE && lora->event_queue.count > 0)
    {
        cqueue_receive(&lora->event_queue, &lora->event);
        // New event
        switch (lora->event.event)
        {
        case EVENT_LINK_MODULE:
            lora_send_connect_cmd(lora);
            break;
        case EVENT_READ_MODULE:
            lora_send_read_cmd(lora);
            break;
        case EVENT_WRITE_MODULE:
            lora_send_write_cmd(lora);
            break;
        case EVENT_RESET_MODULE:
            lora_send_reset_cmd(lora);
            break;
        case EVENT_ENTER_LOWPOWER:
            lora_send_lowpower_cmd(lora);
            break;
        default:
            lora->event.event = EVENT_NONE;
            break;
        }
    }
    if (lora->event.tick < lora->event.timeout && lora->event.event != EVENT_NONE)
    {
        lora->event.tick += timestamp;
    }
    else if (lora->event.tick == lora->event.timeout)
    {
        lora->event.tick = 0;
        lora->buff_len = lora->driver->available();
        lora->driver->read(lora->buff, lora->buff_len);
        if (lora->event.event != EVENT_NONE)
        {
            log_debug(TAG, "LoRa Event : %d, len : %u", lora->event.event, lora->buff_len);
            log_print_hex(LOGGER_DEBUG, TAG, lora->buff, lora->buff_len);
        }
        if(lora->buff_len == 0 && lora->event.cb != NULL)
        {
            lora->event.cb(lora, LORA_RES_TIMEOUT, lora->event.arg);
            lora->event.event = EVENT_NONE;
            return;
        }
        uint8_t res = LORA_RES_FAIL;
        switch (lora->event.event)
        {
        case EVENT_LINK_MODULE:
            res = lora_link_modlue_process(lora, lora->buff, lora->buff_len);
            break;
        case EVENT_READ_MODULE:
            res = lora_read_modlue_process(lora, lora->buff, lora->buff_len);
            break;
        case EVENT_WRITE_MODULE:
            res = lora_write_modlue_process(lora, lora->buff, lora->buff_len);
            break;
        case EVENT_RESET_MODULE:
            res = lora_reset_modlue_process(lora, lora->buff, lora->buff_len);
            break;
        case EVENT_ENTER_LOWPOWER:
            res = lora_enter_lowpower_process(lora, lora->buff, lora->buff_len);
            break;
        default:
            break;
        }
        if (lora->event.cb != NULL)
        {
            lora->event.cb(lora, res, lora->event.arg);
        }
        lora->event.event = EVENT_NONE;
    }
}

void lora_connect(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg)
{

    log_info(TAG, "Connect to LoRa Module");
    // lora->driver->write(LoraINS_LinkModule, 7);
    lora->event_tmp.event = EVENT_LINK_MODULE;
    lora->event_tmp.timeout = 300;
    lora->event_tmp.tick = 0;
    lora->event_tmp.cb = cb;
    lora->event_tmp.arg = arg;
    cqueue_send(&lora->event_queue, &lora->event_tmp);
}

void lora_reset_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg)
{
    log_info(TAG, "Reset LoRa Module");
    lora->event_tmp.event = EVENT_RESET_MODULE;
    lora->event_tmp.timeout = 300;
    lora->event_tmp.tick = 0;
    lora->event_tmp.cb = cb;
    lora->event_tmp.arg = arg;
    cqueue_send(&lora->event_queue, &lora->event_tmp);
}

void lora_read_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg)
{
    log_info(TAG, "Read LoRa Module");
    lora->event_tmp.event = EVENT_READ_MODULE;
    lora->event_tmp.timeout = 2000;
    lora->event_tmp.tick = 0;
    lora->event_tmp.cb = cb;
    lora->event_tmp.arg = arg;
    cqueue_send(&lora->event_queue, &lora->event_tmp);
}

void lora_write_module(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg)
{
    log_info(TAG, "Write LoRa Module");
    lora_get_write_ins(lora);
    lora->event_tmp.event = EVENT_WRITE_MODULE;
    lora->event_tmp.timeout = 2000;
    lora->event_tmp.tick = 0;
    lora->event_tmp.cb = cb;
    lora->event_tmp.arg = arg;
    cqueue_send(&lora->event_queue, &lora->event_tmp);
}

void lora_enter_lowpower(LoRaMesh_t *lora, Lora_Callback_Func cb,void *arg)
{
    log_info(TAG, "Enter LoRa Low Power Mode");
    lora->event_tmp.event = EVENT_ENTER_LOWPOWER;
    lora->event_tmp.timeout = 300;
    lora->event_tmp.tick = 0;
    lora->event_tmp.cb = cb;
    lora->event_tmp.arg = arg;
    cqueue_send(&lora->event_queue, &lora->event_tmp);
}