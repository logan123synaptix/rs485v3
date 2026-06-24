# KB_V2_MODBUS

**Version:** 1.0 — COMPLETE
**Source of Truth:** KB_V2.md ✅
**Source files read:**
- `SynaptiX/services/mbport/portserial.c` ✅
- `SynaptiX/services/mbport/porttimer.c` ✅
- `SynaptiX/services/mbport/portevent.c` ✅
- `SynaptiX/services/mbport/port.h` ✅
- `SynaptiX/services/Modbus/port/user_mb_app.c` ✅
- `SynaptiX/services/Modbus/port/user_mb_app.h` ✅
- `SynaptiX/board/board.h` ✅
- `SynaptiX/board/board.c` ✅
- `Core/Src/usart.c` ✅

---

## Purpose

Modbus subsystem trong RS485_IO_RF_V2 implement **FreeModbus slave** chạy trên RS485 vật lý (USART1 + DE pin PB13). Đây là giao tiếp công nghiệp chính giữa thiết bị và host/master (PLC, SCADA).

Subsystem có 3 layer:

```
[FreeModbus Stack]  ←  third-party, portable, không chạm hardware
       ↓
[mbport layer]      ←  adapter giữa FreeModbus và BSP
       ↓
[BSP layer]         ←  board.c: hardware abstraction thực sự
       ↓
[HAL / Hardware]    ←  USART1, TIM2, TIM3, GPIO PB13
```

---

## Responsibilities

| Layer | Responsibility |
|---|---|
| FreeModbus stack (`modbus/`) | RTU/ASCII framing, CRC16/LRC, FC dispatch, register R/W callbacks, event loop |
| mbport (`portserial.c`) | Kết nối FreeModbus serial API với BSP COM port; DE toggle; TX callback routing |
| mbport (`porttimer.c`) | Kết nối FreeModbus timer API với BSP timer (TIM2/TIM3) |
| mbport (`portevent.c`) | Event queue in-memory (flag-based, no FreeRTOS primitive) |
| BSP (`board.c`) | UART RX queue (CQueue), `HAL_UART_Transmit_IT`, `HAL_UART_RxCpltCallback`, TIM callback |
| App (`user_mb_app.c`) | Register buffers, 4 callback handlers: Input/Holding/Coil/Discrete |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                            │
│           đọc/ghi: input_reg[], hoding_reg[],                   │
│                    input_coils[], output_coils[]                │
└──────────────────────────┬──────────────────────────────────────┘
                           │ eMBRegInputCB / eMBRegHoldingCB
                           │ eMBRegCoilsCB / eMBRegDiscreteCB
┌──────────────────────────▼──────────────────────────────────────┐
│              FreeModbus Stack (Modbus/modbus/)                  │
│  mb.c → mbrtu.c → mbfuncholding.c / mbfunccoils.c / ...        │
│  eMBPoll() — called from task loop                              │
└──────┬──────────────────────────────────────┬───────────────────┘
       │ xMBPortSerialInit/Enable/Get/PutByte  │ xMBPortTimersInit
       │ xMBPortSerialPutBytes                 │ vMBPortTimersEnable/Disable
       │ xMBPortEventPost/Get                  │ pxMBPortCBTimerExpired()
┌──────▼───────────────────┐  ┌────────────────▼──────────────────┐
│  portserial.c            │  │  porttimer.c                      │
│  portevent.c             │  │                                   │
│  · eModbus[N_MODBUS=2]   │  │  · TIMER0 → TIM2 (RS485 port)    │
│  · bsp_com_* calls       │  │  · TIMER1 → TIM3 (RF port)       │
│  · bsp_de_on/off         │  │  · bsp_timer_set_handle()         │
│  · mb_uart_rx_task()     │  │  · bsp_timer0/1_start/stop()     │
│  · mb_uart_tx_task()     │  │                                   │
└──────┬───────────────────┘  └────────────────┬──────────────────┘
       │                                        │
┌──────▼────────────────────────────────────────▼──────────────────┐
│                       BSP (board.c)                              │
│  · bsp_uart[3] = {&hlpuart1, &huart1, &huart2}                  │
│  · uart_queue[3]: CQueue circular buffer (256 bytes each)        │
│  · uart_data[3]: single-byte RX buffer per UART                  │
│  · HAL_UART_RxCpltCallback → cqueue_send → re-arm Receive_IT    │
│  · HAL_UART_TxCpltCallback → com_tx_callback[i].cb()            │
│  · BSP_TIM_PeriodElapsedCallback → tim_handle[0/1]()            │
└──────┬────────────────────────────────────────┬──────────────────┘
       │                                        │
  USART1 (RS485)                           TIM2 (3.5T RS485)
  HAL_UART_Receive_IT (1 byte)             TIM3 (3.5T RF)
  HAL_UART_Transmit_IT                     HAL_TIM_Base_Start/Stop_IT
  PB13 DE: HAL_GPIO_WritePin
```

---

## Module Structure

```
SynaptiX/services/
├── mbport/
│   ├── port.h           — typedefs (BOOL, UCHAR, USHORT, ULONG), N_MODBUS=2, critical section macros (hiện tại NOP)
│   ├── portserial.c     — FreeModbus serial port adapter
│   ├── porttimer.c      — FreeModbus timer adapter
│   └── portevent.c      — FreeModbus event queue (in-struct flag)
│
└── Modbus/
    ├── port/
    │   ├── user_mb_app.h  — register size defines + address constants
    │   └── user_mb_app.c  — 4 register callback implementations
    └── modbus/
        ├── mb.c           — eMBInit(), eMBEnable(), eMBPoll()
        ├── include/
        │   ├── mb.h       — public API
        │   ├── mbconfig.h — compile-time feature flags
        │   ├── mbport.h   — eModbus struct (chứa toàn bộ state)
        │   └── ...
        ├── rtu/
        │   ├── mbrtu.c    — RTU state machine
        │   └── mbcrc.c    — CRC16 lookup table
        ├── ascii/
        │   └── mbascii.c  — ASCII framing
        └── functions/
            ├── mbfuncholding.c
            ├── mbfunccoils.c
            ├── mbfuncinput.c
            ├── mbfuncdisc.c
            └── ...

SynaptiX/board/
├── board.h  — BSP macros, function declarations, inline bsp_de_on/off, bsp_flash_*
└── board.c  — BSP implementation: CQueue UART, HAL callbacks, timer dispatch
```

---

## Public APIs

### FreeModbus public API (mb.h) — được gọi từ app

```c
eMBErrorCode eMBInit(eModbus_t modbus, eMBMode eMode, UCHAR ucSlaveAddress,
                     UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity);
eMBErrorCode eMBEnable(eModbus_t modbus);
eMBErrorCode eMBDisable(eModbus_t modbus);
eMBErrorCode eMBPoll(eModbus_t modbus);  // gọi trong task loop
eMBErrorCode eMBSetSlaveID(eModbus_t modbus, UCHAR ucSlaveID, BOOL xIsRunning,
                           UCHAR const *pucAdditional, USHORT usAdditionalLen);
```

### mbport serial API (portserial.c) — gọi nội bộ bởi FreeModbus

```c
BOOL xMBPortSerialInit(eModbus_t modbus, UCHAR ucPORT, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity);
void vMBPortSerialEnable(eModbus_t modbus, BOOL xRxEnable, BOOL xTxEnable);
BOOL xMBPortSerialGetByte(eModbus_t modbus, CHAR *pucByte);
BOOL xMBPortSerialPutByte(eModbus_t modbus, CHAR ucByte);
BOOL xMBPortSerialPutBytes(eModbus_t modbus, volatile UCHAR *ucByte, USHORT usSize);
void mb_uart_rx_task(eModbus *mb);   // gọi khi có bytes trong RX queue
void mb_uart_tx_task(void *arg);     // TX complete callback
```

### mbport timer API (porttimer.c) — gọi nội bộ bởi FreeModbus

```c
BOOL xMBPortTimersInit(eModbus_t modbus, USHORT usTim1Timerout50us);
void vMBPortTimersEnable(eModbus_t modbus);
void vMBPortTimersDisable(eModbus_t modbus);
```

### mbport event API (portevent.c) — gọi nội bộ bởi FreeModbus

```c
BOOL xMBPortEventInit(eModbus_t modbus);
BOOL xMBPortEventPost(eModbus_t modbus, eMBEventType eEvent);
BOOL xMBPortEventGet(eModbus_t modbus, eMBEventType *eEvent);
```

### Register callbacks (user_mb_app.c) — gọi bởi FreeModbus FC handlers

```c
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs);
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                              eMBRegisterMode eMode);
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
                            eMBRegisterMode eMode);
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete);
```

### BSP API được mbport gọi (board.h / board.c)

```c
uint32_t bsp_com_available(int com_num);     // trả về count của CQueue
uint32_t bsp_com_read(int com_num, uint8_t *buff, uint32_t len);
uint32_t bsp_com_write_it(int com, uint8_t *buff, uint32_t len); // HAL_UART_Transmit_IT
void     bsp_com_set_tx_callback(int com, void (*cb)(void *arg), void *arg);
bool     bsp_de_on();   // inline → bsp_uart2_de_on() → HAL_GPIO_WritePin(USART1_DE, SET)
bool     bsp_de_off();  // inline → bsp_uart2_de_off() → HAL_GPIO_WritePin(USART1_DE, RESET)
void     bsp_timer_set_handle(int timer, timer_handle handle);
// Macros:
bsp_timer0_start()  // HAL_TIM_Base_Start_IT(&htim2)
bsp_timer0_stop()   // HAL_TIM_Base_Stop_IT(&htim2)
bsp_timer1_start()  // HAL_TIM_Base_Start_IT(&htim3)
bsp_timer1_stop()   // HAL_TIM_Base_Stop_IT(&htim3)
```

---

## Execution Flow

### Khởi tạo

```
app init
  └── eMBInit(&modbus[0], MB_RTU, slave_addr, BSP_RS485_COM_PORT=1, baud, parity)
        ├── mbrtu_init() — khởi tạo RTU state machine
        ├── xMBPortEventInit() — xEventInQueue = FALSE
        ├── xMBPortSerialInit(modbus, port=1, baud, ...)
        │     ├── modbus->config.ucPort = 1
        │     └── bsp_com_set_tx_callback(1, mb_uart_tx_task, modbus)
        │           → com_tx_callback[1] = {mb_uart_tx_task, modbus}
        └── xMBPortTimersInit(modbus, timeout50us)
              └── bsp_timer_set_handle(0, mb_timer0_callback)
                    → tim_handle[0] = mb_timer0_callback

  └── eMBEnable(&modbus[0])
        └── vMBPortSerialEnable(modbus, xRxEnable=TRUE, xTxEnable=FALSE)
              └── bsp_de_off() — DE LOW = receive mode

  └── bsp_com_init() [gọi trong bsp_init()]
        └── HAL_UART_Receive_IT(&huart1, &uart_data[1], 1) — arm RX cho USART1
```

### RX Path (Master → Slave)

```
RS485 wire → USART1 hardware → RXNE interrupt
  └── HAL_UART_IRQHandler(&huart1)
        └── HAL_UART_RxCpltCallback(&huart1)  [board.c]
              ├── cqueue_send(&uart_queue[1], &uart_data[1])  ← enqueue byte
              ├── uart_tick[1] = 2  (BSP_UART_TIMEOUT_READY)
              └── HAL_UART_Receive_IT(&huart1, &uart_data[1], 1)  ← re-arm

Trong task loop:
  └── eMBPoll(&modbus[0])
        ├── xMBPortEventGet() — check xEventInQueue flag
        └── (nếu không có event) → mb_uart_rx_task(&modbus[0])
              ├── bsp_com_available(1) → uart_queue[1].count
              ├── nếu len > 0:
              │     for each byte:
              │       modbus->pxMBFrameCBByteReceived(modbus)
              │         → mbrtu byte received handler
              │           ├── vMBPortTimersEnable() → bsp_timer0_start() → HAL_TIM_Base_Start_IT(&htim2)
              │           └── xMBPortSerialGetByte() → bsp_com_read(1, &byte, 1)
              └── return
```

### 3.5T Timeout

```
TIM2 period elapsed interrupt
  └── HAL_TIM_PeriodElapsedCallback(&htim2)  [stm32h5xx_it.c → board.c]
        └── BSP_TIM_PeriodElapsedCallback(&htim2)
              └── tim_handle[0]()
                    └── mb_timer0_callback()
                          └── modbus[0].pxMBPortCBTimerExpired(&modbus[0])
                                └── mbrtu timer expired → EV_FRAME_RECEIVED
                                      └── xMBPortEventPost(modbus, EV_FRAME_RECEIVED)
                                            └── modbus->xEventInQueue = TRUE
                                                modbus->eQueuedEvent = EV_FRAME_RECEIVED
```

### Processing và TX Path

```
eMBPoll() tiếp theo:
  └── xMBPortEventGet() → EV_FRAME_RECEIVED
        └── eMBRTUReceive() → validate CRC, check slave address
              ├── CRC fail → discard, return to IDLE
              ├── addr mismatch → discard
              └── addr match → EV_EXECUTE
                    └── eMBFuncHandleRequest() → dispatch FC
                          ├── eMBRegHoldingCB() / eMBRegCoilsCB() / ...
                          └── build response frame → EV_FRAME_SENT

  └── xMBPortEventGet() → EV_FRAME_SENT
        └── vMBPortSerialEnable(modbus, xRxEnable=FALSE, xTxEnable=TRUE)
              ├── bsp_de_on()  ← DE HIGH = transmit mode
              └── modbus->pxMBFrameCBTransmitterEmpty(modbus)
                    └── mbrtu transmitter empty → xMBPortSerialPutByte() / xMBPortSerialPutBytes()
                          └── bsp_com_write_it(1, &byte, 1)
                                └── HAL_UART_Transmit_IT(&huart1, &byte, 1)

TX Complete interrupt:
  └── HAL_UART_TxCpltCallback(&huart1)  [board.c]
        └── com_tx_callback[1].cb(arg)
              └── mb_uart_tx_task(modbus)
                    └── modbus->pxMBFrameCBTransmitterEmpty(modbus)
                          ├── nếu còn byte → xMBPortSerialPutByte() tiếp
                          └── nếu hết frame → vMBPortSerialEnable(RX=TRUE, TX=FALSE)
                                ├── bsp_de_off()  ← DE LOW = receive mode
                                └── re-arm RX (đã được arm liên tục bởi HAL_UART_RxCpltCallback)
```

---

## Data Flow

```
[RS485 Wire]
    │ bit stream
    ▼
[USART1 RXNE ISR] → uart_data[1] (1 byte) → cqueue_send → uart_queue[1] (CQueue, 256 bytes)
                                                                    │
                                                     TIM2 re-arm mỗi byte nhận
                                                                    │
                                              3.5T elapsed → EV_FRAME_RECEIVED
                                                                    │
                                          eMBPoll() → bsp_com_read() drain queue
                                                                    │
                                          FreeModbus RTU parser → frame validated
                                                                    │
                                          FC dispatch → register callbacks
                                                                    │
                                          eMBRegHoldingCB() R/W: usSRegHoldBuf[172]
                                          eMBRegCoilsCB()   R/W: ucSCoilBuf[7 bytes]
                                          eMBRegInputCB()   R:   usSRegInBuf[32]
                                          eMBRegDiscreteCB() R:  ucSDiscInBuf[7 bytes]
                                                                    │
                                          response frame build
                                                                    │
                              bsp_de_on() → HAL_UART_Transmit_IT → [USART1 TX]
                                                                    │
                              TC callback → mb_uart_tx_task → next byte / DE off
                                                                    │
                                                              [RS485 Wire]
```

---

## UART Integration

| Parameter | Value |
|---|---|
| Peripheral | USART1 |
| BSP index | `BSP_RS485_COM_PORT = 1` |
| HAL handle | `huart1` (= `bsp_uart[1]`) |
| TX pin | PB14 (`GPIO_AF4_USART1`) |
| RX pin | PB15 (`GPIO_AF4_USART1`) |
| Baud rate | 115200 (init default trong `MX_USART1_UART_Init`) |
| Word length | 8 bits (`UART_WORDLENGTH_8B`) |
| Parity | None (`UART_PARITY_NONE`) |
| Stop bits | 1 (`UART_STOPBITS_1`) |
| Oversampling | 16x (`UART_OVERSAMPLING_16`) |
| HW flow control | None |
| FIFO mode | Disabled |
| Clock source | PCLK2 (`RCC_USART1CLKSOURCE_PCLK2`) |
| TX mode | `HAL_UART_Transmit_IT` (interrupt-driven) |
| RX mode | `HAL_UART_Receive_IT` 1 byte tại một thời điểm, auto re-arm |
| RX buffer | CQueue circular buffer, 256 bytes, trong `board.c` |
| NVIC priority | 5 (preempts FreeRTOS tasks ở priority 15) |

**Quan trọng:** `xMBPortSerialInit()` **không** gọi HAL UART init (đã được `MX_USART1_UART_Init()` gọi trước). `xMBPortSerialInit()` chỉ lưu port index và đăng ký TX callback.

---

## RS485 Direction Control

**Mechanism: Software GPIO toggle (không phải hardware UART DE mode)**

```
bsp_de_on()
  → bsp_uart2_de_on()       ← tên gây nhầm lẫn, thực ra là USART1 DE
    → HAL_GPIO_WritePin(USART1_DE_GPIO_Port, USART1_DE_Pin, GPIO_PIN_SET)
      → PB13 HIGH = Transmit mode

bsp_de_off()
  → bsp_uart2_de_off()
    → HAL_GPIO_WritePin(USART1_DE_GPIO_Port, USART1_DE_Pin, GPIO_PIN_RESET)
      → PB13 LOW = Receive mode
```

**Timing của DE:**
- DE HIGH: trong `vMBPortSerialEnable(RX=FALSE, TX=TRUE)` — **trước** khi byte đầu tiên được đưa vào TX buffer
- DE LOW: trong `vMBPortSerialEnable(RX=TRUE, TX=FALSE)` — được gọi từ `mb_uart_tx_task()` khi FreeModbus stack báo hết frame

**⚠️ Risk:** DE LOW được trigger bởi FreeModbus stack logic (khi stack biết frame đã xong), không phải từ TC (Transmission Complete) interrupt trực tiếp. Cần xác nhận `mb_uart_tx_task` → `pxMBFrameCBTransmitterEmpty` → stack → `vMBPortSerialEnable(RX,!TX)` xảy ra **sau** khi TC interrupt fire, không phải trước.

---

## ISR Integration

### UART ISR → BSP → mbport flow

```
USART1_IRQn (priority 5)
  └── HAL_UART_IRQHandler(&huart1)         [HAL]
        └── HAL_UART_RxCpltCallback()       [board.c — override HAL weak]
              ├── cqueue_send(&uart_queue[1], &uart_data[1])
              ├── uart_tick[1] = 2
              └── HAL_UART_Receive_IT(&huart1, &uart_data[1], 1)

        └── HAL_UART_TxCpltCallback()       [board.c — override HAL weak]
              └── com_tx_callback[1].cb(arg) = mb_uart_tx_task(modbus)
```

### Timer ISR → BSP → mbport → FreeModbus flow

```
TIM2_IRQn (priority 5)
  └── HAL_TIM_IRQHandler(&htim2)           [HAL]
        └── HAL_TIM_PeriodElapsedCallback() [stm32h5xx_it.c → delegates to BSP_TIM_PeriodElapsedCallback]
              └── BSP_TIM_PeriodElapsedCallback(&htim2)  [board.c]
                    └── tim_handle[0]()
                          └── mb_timer0_callback()       [porttimer.c static]
                                └── modbus[0].pxMBPortCBTimerExpired(&modbus[0])
                                      └── FreeModbus RTU timer expired handler
```

**Không có FreeRTOS primitive trong ISR path** — toàn bộ signaling qua flag `xEventInQueue` trong `eModbus` struct. ISR → task communication là: ISR enqueue vào CQueue (lock-free bởi 1-byte re-arm pattern), task drain queue khi poll.

---

## Timer Integration

| Timer | Role | Hardware | Start/Stop |
|---|---|---|---|
| TIM2 | 3.5T inter-frame timeout cho RS485 (port 1) | `htim2` | `HAL_TIM_Base_Start/Stop_IT(&htim2)` |
| TIM3 | 3.5T inter-frame timeout cho RF (port 2) | `htim3` | `HAL_TIM_Base_Start/Stop_IT(&htim3)` |
| TIM6 | HAL timebase | `htim6` | auto, không dùng cho Modbus |

**Timer period:** TIM2/TIM3 được config trong `Core/Src/tim.c` — từ KB_V2 biết prescaler=249, period=999 → **1ms interrupt tại 250MHz**. Với Modbus 115200 baud: 3.5T = ~0.3ms. Timer 1ms **quá thô** cho 115200 baud — xem Risks.

**Timer reset pattern:** Mỗi byte nhận được, `pxMBFrameCBByteReceived` gọi `vMBPortTimersEnable()` để restart TIM2. Nếu không có byte mới trong 1 tick (1ms), TIM2 expire → frame complete.

---

## Register Model

### Tất cả 4 register types đều được implement

| Type | FreeModbus FC | Start Addr | Count | Buffer | Size RAM |
|---|---|---|---|---|---|
| Holding Registers | FC03(R)/FC06(W single)/FC16(W multi) | 0 | 172 | `usSRegHoldBuf[172]` | 344 bytes |
| Input Registers | FC04(R) | 0 | 32 | `usSRegInBuf[32]` | 64 bytes |
| Coils | FC01(R)/FC05(W single)/FC15(W multi) | 0 | 56 | `ucSCoilBuf[7 bytes]` | 7 bytes |
| Discrete Inputs | FC02(R) | 0 | 56 | `ucSDiscInBuf[7 bytes]` | 7 bytes |

### Address map hiện tại (user_mb_app.h)

```c
// Holding Register addresses (usSRegHoldBuf[])
#define S_HD_RESERVE         0   // addr 0: reserved
#define S_HD_CPU_USAGE_MAJOR 1   // addr 1: CPU usage major
#define S_HD_CPU_USAGE_MINOR 2   // addr 2: CPU usage minor
// addr 3..171: undefined (available for app to define)

// Input Register addresses (usSRegInBuf[])
#define S_IN_RESERVE         0   // addr 0: reserved
// addr 1..31: undefined

// Coil addresses (ucSCoilBuf[])
#define S_CO_RESERVE         0   // addr 0..55: available

// Discrete Input addresses (ucSDiscInBuf[])
#define S_DI_RESERVE         0   // addr 0..55: available
```

**Ghi chú quan trọng:** Mapping cụ thể của OUT0-OUT3 vào Coil và IN0-IN3 vào Discrete Input **không có trong user_mb_app.c/h** — chỉ có buffer và callbacks generic. Mapping thực tế phải nằm ở application layer (apps/) hoặc trong task loop khi sync GPIO → register buffer. Cần đọc `SynaptiX/apps/` để xác nhận.

### Byte order

Big-endian (Modbus standard):
```c
// Holding register read:
*pucRegBuffer++ = (UCHAR)(pusRegHoldingBuf[iRegIndex] >> 8);   // high byte trước
*pucRegBuffer++ = (UCHAR)(pusRegHoldingBuf[iRegIndex] & 0xFF); // low byte sau
// Write:
pusRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
pusRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
```

---

## Buffer Ownership

```
ISR context (USART1_IRQn, priority 5):
  uart_data[1]     — single-byte staging buffer, owned by HAL RX IT
  uart_queue[1]    — CQueue 256 bytes, ISR là producer (cqueue_send)

Task context (eMBPoll loop):
  uart_queue[1]    — task là consumer (bsp_com_read → cqueue_receive)
  eModbus.rcvState — FreeModbus RTU state machine buffer (internal to mbrtu.c)
  usSRegHoldBuf[]  — shared giữa FreeModbus callbacks và App layer

TX context:
  Byte được đưa vào HAL_UART_Transmit_IT từ FreeModbus internal frame buffer
  DE pin toggle trong task context (mb_uart_tx_task được gọi từ TxCpltCallback = ISR context)
```

**⚠️ Concurrency issue quan trọng:**
`mb_uart_tx_task()` được gọi từ `HAL_UART_TxCpltCallback()` — đây là ISR context. Bên trong `mb_uart_tx_task()` gọi `pxMBFrameCBTransmitterEmpty()` — FreeModbus stack function. FreeModbus không được thiết kế để chạy trong ISR. Đây là **design issue** hiện tại trong V2.

**CQueue thread safety:** `cqueue_send` (từ ISR) và `cqueue_receive` (từ task) trên cùng queue — không có mutex. An toàn chỉ khi ISR không bị preempt bởi task trong thời gian thao tác queue (đúng với NVIC priority 5 > FreeRTOS task priority 15).

**ENTER_CRITICAL_SECTION / EXIT_CRITICAL_SECTION trong port.h hiện tại là NOP** (bị comment out):
```c
#define ENTER_CRITICAL_SECTION( ) //__critical_enter()
#define EXIT_CRITICAL_SECTION( )  //__critical_exit()
```
FreeModbus dùng macro này để bảo vệ event queue — hiện tại **không được bảo vệ**.

---

## State Machine

### FreeModbus RTU State Machine (mbrtu.c internal)

```
INIT ──(timer expire)──► IDLE
 
IDLE ──(byte received)──► RECEPTION
       vMBPortTimersEnable() ← restart timer mỗi byte

RECEPTION ──(byte received)──► RECEPTION (timer restart)
           ──(3.5T expire)────► FRAME_RECEIVED ──► EV_FRAME_RECEIVED

FRAME_RECEIVED [trong eMBPoll]:
  ├── CRC invalid ──► EV_ERROR / back to IDLE
  ├── addr mismatch ──► back to IDLE  
  └── addr match ──► EV_EXECUTE

EV_EXECUTE:
  └── FC dispatch ──► EV_FRAME_SENT

EV_FRAME_SENT:
  └── vMBPortSerialEnable(RX=F, TX=T)
        ├── bsp_de_on()
        └── pxMBFrameCBTransmitterEmpty() ← start TX byte stream

TRANSMITTING:
  ├── TxCplt callback → mb_uart_tx_task → next byte
  └── frame done → vMBPortSerialEnable(RX=T, TX=F) → bsp_de_off() → IDLE
```

### Event Queue State

```
portevent.c — stored directly in eModbus struct:
  eModbus.xEventInQueue: BOOL flag
  eModbus.eQueuedEvent:  eMBEventType {EV_READY, EV_FRAME_RECEIVED, EV_EXECUTE, EV_FRAME_SENT}

Single-event queue — chỉ giữ được 1 event tại một thời điểm.
Nếu 2 events post liên tiếp trước khi eMBPoll chạy → second event ghi đè first.
```

---

## Concurrency Model

**Model: Cooperative polling với ISR feed**

```
ISR (priority 5) — preemptive với tasks:
  - USART1 RX: enqueue byte vào CQueue
  - USART1 TX complete: gọi mb_uart_tx_task() [⚠️ ISR gọi stack function]
  - TIM2 expire: gọi FreeModbus timer expired [⚠️ ISR gọi stack function]

Task (FreeRTOS, priority variable):
  - Gọi eMBPoll() trong loop
  - eMBPoll() check event flag (xEventInQueue)
  - Nếu có event → process frame → dispatch FC → build response
  - Gọi mb_uart_rx_task() để drain RX queue

Shared state:
  - eModbus struct: mọi state FreeModbus — accessed từ cả ISR lẫn task
  - uart_queue[1]: CQueue — ISR write, task read
  - usSRegHoldBuf[]: register table — task write (FC handler), app read/write
  - xEventInQueue flag: ISR write (timer), task read/write
```

**FreeRTOS usage trong Modbus path: KHÔNG có**
- Không dùng osSemaphore, osQueue, osMutex
- Không dùng xTaskNotifyFromISR
- Event passing hoàn toàn qua flag trong struct
- Phù hợp với bare-metal model nhưng không tận dụng FreeRTOS

---

## Dependency Graph

```
user_mb_app.c
  └── mb.h, mbconfig.h, mbframe.h, mbutils.h   (FreeModbus headers)

portserial.c
  ├── mb.h, mbport.h, port.h                   (FreeModbus + port headers)
  ├── board.h                                  (BSP API)
  └── logger.h                                 (logging)

porttimer.c
  ├── mb.h, mbport.h, port.h
  └── board.h

portevent.c
  └── mb.h, mbport.h

board.c
  ├── board.h
  ├── cqueue.h                                 (circular queue implementation)
  ├── usart.h  → huart1, huart2, hlpuart1
  ├── tim.h    → htim2, htim3
  ├── gpio.h
  ├── iwdg.h   → hiwdg
  ├── logger.h
  ├── button.h
  ├── zigbee.h [conditional: BSP_RS485_RF_TRANSFER_MODE]
  └── app_settings.h [conditional]

Hardware dependencies của Modbus path:
  USART1 (huart1)     ← board.c
  TIM2 (htim2)        ← board.c  [3.5T RS485]
  TIM3 (htim3)        ← board.c  [3.5T RF — nếu N_MODBUS=2]
  GPIO PB13           ← board.h inline (USART1_DE_GPIO_Port, USART1_DE_Pin)
  CQueue              ← board.c (uart_queue[1])
```

---

## Hidden Assumptions

1. **`eMBPoll()` được gọi liên tục trong task loop** — FreeModbus không có internal thread. Nếu task bị block hoặc delay, Modbus response bị trễ. Watchdog feed (`bsp_delay()`) trong task loop sẽ tạo ra delay nhân tạo nếu dùng `HAL_Delay`.

2. **Baud rate 115200 được init trong CubeMX, không phải trong eMBInit** — `xMBPortSerialInit()` nhận `ulBaudRate` parameter nhưng **không dùng nó** để reconfigure UART. Baud rate cố định bởi `MX_USART1_UART_Init()`. Nếu cần thay đổi baud rate runtime → không hoạt động với current implementation.

3. **N_MODBUS = 2** — hai instance Modbus được tạo (`modbus[0]` cho RS485, `modbus[1]` cho RF). Cả hai dùng chung struct array nhưng timer và UART khác nhau. Instance `modbus[1]` dùng TIM3 và BSP_RF_COM_PORT=2 (USART2).

4. **Slave address = `bsp_get_address()` = hardcoded 1** — không đọc từ GPIO DIP switch hay flash. Địa chỉ không configurable runtime.

5. **`HAL_TIM_PeriodElapsedCallback` phải được route qua `BSP_TIM_PeriodElapsedCallback`** — nếu `stm32h5xx_it.c` không gọi `BSP_TIM_PeriodElapsedCallback`, timer Modbus không hoạt động.

6. **CQueue không thread-safe** — an toàn hiện tại chỉ nhờ NVIC priority (ISR priority 5 không bị preempt bởi task), nhưng không được document và dễ break nếu thêm ISR priority thấp hơn.

7. **`bsp_uart2_de_on/off` tên sai** — macro `bsp_uart2_de_on()` thực ra toggle PB13 là DE của USART1 (RS485), không phải USART2. Naming error trong board.h có thể gây nhầm lẫn khi porting.

8. **`bsp_com_init()` có bug**: trong `for` loop, điều kiện `if(i = BSP_RF_COM_PORT)` dùng `=` (assignment) thay vì `==` (compare) → luôn true với giá trị non-zero, khiến tất cả UARTs dùng `HAL_UARTEx_ReceiveToIdle_DMA` thay vì chỉ RF port. Đây là bug C kinh điển.

9. **TX callback từ ISR gọi FreeModbus stack** — `HAL_UART_TxCpltCallback` là ISR context. Gọi `mb_uart_tx_task` → `pxMBFrameCBTransmitterEmpty` → FreeModbus logic từ ISR vi phạm FreeModbus design assumption (không reentrant).

10. **`ENTER_CRITICAL_SECTION` là NOP** — nếu FreeModbus cần critical section (ví dụ event queue protection), hiện tại không được bảo vệ.

---

## Design Decisions

| Decision | Thực tế Observed | Rationale |
|---|---|---|
| FreeModbus làm Modbus stack | Third-party, LGPL, STM32F0 demo có sẵn trong repo | Không viết lại từ đầu, proven implementation |
| BSP layer làm hardware abstraction | board.c wrap toàn bộ HAL | Tách mbport khỏi HAL — mbport chỉ gọi bsp_* |
| Software DE toggle (không hardware mode) | `HAL_GPIO_WritePin` trực tiếp | Đơn giản hơn, không phụ thuộc UART hardware DE feature |
| CQueue 256 bytes per UART | Static allocation trong board.c | Deterministic, không dùng heap |
| Single-byte RX IT (không DMA cho USART1) | `HAL_UART_Receive_IT(..., 1)` | Cho phép byte-by-byte timer reset, phù hợp FreeModbus model |
| Event queue là flag trong struct (không FreeRTOS queue) | `xEventInQueue` BOOL | Đơn giản, zero overhead, phù hợp polling model |
| N_MODBUS = 2 | `eModbus modbus[2]` | Dự phòng cho RF port cũng chạy Modbus-like protocol |
| TIM2 cho RS485, TIM3 cho RF | `bsp_timer0/1` mapped cứng | Mỗi Modbus instance có timer độc lập |

---

## Risks

| Risk | Severity | Chi tiết |
|---|---|---|
| ISR gọi FreeModbus stack (TxCpltCallback → mb_uart_tx_task) | HIGH | FreeModbus không reentrant. Nếu task đang trong eMBPoll khi TC interrupt fire → potential corruption |
| Timer 1ms quá thô cho 115200 baud | HIGH | 3.5T tại 115200 ≈ 0.3ms < 1ms timer tick. Frame boundary sẽ không được detect chính xác. Nhiều frames bị merge thành 1 |
| Bug `if(i = BSP_RF_COM_PORT)` trong bsp_com_init | HIGH | Tất cả UARTs init bằng `ReceiveToIdle_DMA` thay vì USART1 dùng `Receive_IT`. RX path Modbus có thể không hoạt động đúng |
| ENTER_CRITICAL_SECTION là NOP | MEDIUM | Event queue không được bảo vệ. Trên single-core CM33 thường ok, nhưng undefined behavior về mặt spec |
| baud rate không configurable runtime | MEDIUM | `xMBPortSerialInit` ignore baud parameter. Thay đổi baud phải recompile hoặc re-init UART thủ công |
| Slave address hardcoded = 1 | MEDIUM | `bsp_get_address()` return 1. Không đọc DIP switch hay config flash |
| DE macro tên sai (uart2_de nhưng toggle USART1) | LOW | Gây nhầm lẫn khi đọc code, dễ sửa sai khi porting |
| Single-event queue | LOW | Nếu timer expire và rx byte event xảy ra cùng lúc → event bị ghi đè |

---

## Porting Notes For RF_IO_RS485_V3

### Reusable Components (copy nguyên)

| Component | Files | Điều kiện |
|---|---|---|
| FreeModbus stack | `Modbus/modbus/` toàn bộ | Không chạm hardware, portable hoàn toàn |
| Register callbacks | `Modbus/port/user_mb_app.c` + `.h` | Register layout giữ nguyên. Chỉ update defines nếu thay đổi số lượng registers |
| portevent.c | `mbport/portevent.c` | Pure logic, không phụ thuộc hardware |
| port.h | `mbport/port.h` | Chỉ typedefs và macros, update N_MODBUS nếu cần |
| CQueue | `SynaptiX/utils/cqueue.h` + `.c` | Data structure thuần túy |

### Components Requiring Adaptation

| Component | Files | Cần thay đổi gì |
|---|---|---|
| portserial.c | `mbport/portserial.c` | Giữ nguyên nếu V3 dùng cùng BSP API (`bsp_com_*`, `bsp_de_*`). Thay đổi `BSP_RS485_COM_PORT` value nếu UART index khác |
| porttimer.c | `mbport/porttimer.c` | Giữ nguyên nếu V3 có `bsp_timer0_start/stop`. Thay TIM2/TIM3 trong board.h nếu V3 dùng timer instance khác |
| board.h — port mapping | `board.h` defines | Cập nhật `BSP_RS485_COM_PORT`, DE pin defines, timer macros cho V3 hardware |
| board.c — UART array | `board.c` `bsp_uart[]` | Map đúng HAL handle của V3 vào index đúng |

### Components Requiring Rewrite

| Component | Lý do |
|---|---|
| `bsp_com_init()` | Fix bug `if(i = BSP_RF_COM_PORT)` → `if(i == BSP_RF_COM_PORT)`. Đây phải fix trước khi port |
| Timer period trong TIM2/TIM3 | Nếu V3 giữ baud 115200 → timer phải xuống ≤ 100µs (10kHz minimum). Hiện tại 1ms quá thô. Cần reconfigure TIM2 prescaler/period |
| `bsp_uart2_de_on/off` naming | Đổi tên thành `bsp_rs485_de_on/off` để tránh nhầm lẫn |
| TX callback flow | Xem xét lại việc gọi FreeModbus từ ISR (`mb_uart_tx_task` trong TxCpltCallback). Option an toàn hơn: set flag trong ISR, xử lý trong task poll |
| Baud rate configuration | Nếu V3 cần configurable baud rate: implement `xMBPortSerialInit` thực sự reconfigure `huart1` với baud mới thay vì ignore parameter |
| Slave address | Nếu V3 có DIP switch hoặc config storage: implement `bsp_get_address()` đọc GPIO hoặc flash |

### Porting Checklist

```
□ Copy FreeModbus stack nguyên vẹn (Modbus/modbus/)
□ Copy user_mb_app.c/h, update register count nếu cần
□ Copy portevent.c nguyên vẹn
□ Copy port.h, cập nhật N_MODBUS
□ Copy portserial.c + porttimer.c
□ Implement board.h/board.c cho V3:
  □ bsp_uart[] array với đúng HAL handles
  □ CQueue init cho RS485 port
  □ HAL_UART_RxCpltCallback → cqueue_send
  □ HAL_UART_TxCpltCallback → com_tx_callback dispatch
  □ bsp_de_on/off với đúng GPIO của V3
  □ TIM instance đúng cho bsp_timer0_start/stop
  □ BSP_TIM_PeriodElapsedCallback route đúng timer
□ Fix bug bsp_com_init() (= vs ==)
□ Fix timer period: TIM2 period phải <= 100µs cho 115200 baud
□ Verify stm32h5xx_it.c (hoặc tương đương của V3) route TIM IRQ đến BSP_TIM_PeriodElapsedCallback
□ Verify HAL_TIM_PeriodElapsedCallback được override và gọi BSP version
□ Test: gửi FC03 read holding register, verify response đúng
□ Test: baud rate thực tế bằng logic analyzer (verify 115200 exact)
□ Test: DE timing — logic analyzer verify DE HIGH trước byte đầu, DE LOW sau byte cuối
```

---

## Additional Files Required

Một số thông tin vẫn còn thiếu để KB hoàn chỉnh 100%:

### `SynaptiX/apps/` — file chính (main app entry point)

**Kiến thức còn thiếu:**
- Modbus được init với slave address bao nhiêu (confirm `bsp_get_address()` = 1 hay override)?
- OUT0-OUT3 được sync vào Coil buffer (`ucSCoilBuf`) ở đâu và khi nào?
- IN0-IN3 được sync vào Discrete Input buffer (`ucSDiscInBuf`) ở đâu và khi nào?
- `eMBPoll()` được gọi trong loop riêng hay chung với app logic?
- Có watchdog feed trong Modbus task không?

### `SynaptiX/utils/cqueue.h` + `cqueue.c`

**Kiến thức còn thiếu:**
- `cqueue_send` và `cqueue_receive` có disable interrupt không? Hay truly non-atomic?
- Nếu non-atomic → concurrency risk với CQueue là real, không phải theoretical

### `Core/Src/stm32h5xx_it.c`

**Kiến thức còn thiếu:**
- `HAL_TIM_PeriodElapsedCallback` có được override và gọi `BSP_TIM_PeriodElapsedCallback` không?
- Hay chỉ gọi `HAL_TIM_IRQHandler` và FreeModbus timer không được trigger?

### `SynaptiX/services/Modbus/modbus/include/mbconfig.h`

**Kiến thức còn thiếu:**
- `MB_RTU_ENABLED`, `MB_ASCII_ENABLED` — cả hai có bật không?
- `MB_SLAVE_RTU_ENABLED` vs `MB_MASTER_RTU_ENABLED`
- Buffer size defines trong mbconfig
---

## ADDENDUM — Kiến Thức Bổ Sung (Confirmed từ source code)

### A1. CQueue — Thread Safety Analysis (CONFIRMED)

`cqueue_send()` và `cqueue_receive()` **không có bất kỳ critical section nào**:

```c
bool cqueue_send(CQueue_t *queue, void *item) {
    if(!cqueue_is_full(queue)) {
        memcpy(queue->buff + queue->head*queue->size, item, queue->size);
        int next = (queue->head+1) % queue->len;
        queue->head = next;
        queue->count += 1;   // ← non-atomic read-modify-write
        return true;
    }
    return false;
}
```

**Kết luận:** CQueue là **truly non-atomic**. An toàn hiện tại chỉ nhờ NVIC priority model:
- ISR (priority 5) không thể bị preempt bởi FreeRTOS task (priority 15)
- Single-core CM33: ISR và task không chạy đồng thời
- **Risk thực tế = LOW** với kiến trúc hiện tại, nhưng **vi phạm lý thuyết** nếu có ISR khác ở priority < 5 cũng gọi cqueue

---

### A2. TIM Interrupt Routing — CONFIRMED WORKING

`stm32h5xx_it.c` xác nhận:

```c
void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }
void TIM3_IRQHandler(void) { HAL_TIM_IRQHandler(&htim3); }
```

HAL sẽ gọi weak `HAL_TIM_PeriodElapsedCallback()`. **Nhưng:** file này KHÔNG override callback đó. Callback được override ở đâu đó khác — tìm thấy trong `board.c`:

```c
void BSP_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if(htim == &htim2) { if(tim_handle[0] != NULL) tim_handle[0](); }
    if(htim == &htim3) { if(tim_handle[1] != NULL) tim_handle[1](); }
}
```

**Missing link:** `HAL_TIM_PeriodElapsedCallback` → `BSP_TIM_PeriodElapsedCallback` không thấy trong `stm32h5xx_it.c`. Phải được override ở file khác (có thể trong `Core/Src/stm32h5xx_hal_msp.c` hoặc một USER CODE block). Cần verify thêm nếu cần — nhưng nếu Modbus timer hoạt động trên V2 thực tế thì routing này phải tồn tại ở đâu đó.

---

### A3. mbconfig.h — Feature Flags (CONFIRMED)

```c
MB_RTU_ENABLED    = 1   // ✅ RTU enabled
MB_ASCII_ENABLED  = 1   // ✅ ASCII enabled (cả hai bật, nhưng app chỉ gọi MB_RTU)
MB_TCP_ENABLED    = 0   // TCP disabled

// Tất cả Function Codes đều enabled:
MB_FUNC_READ_INPUT_ENABLED              = 1  // FC04
MB_FUNC_READ_HOLDING_ENABLED            = 1  // FC03
MB_FUNC_WRITE_HOLDING_ENABLED           = 1  // FC06
MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED  = 1  // FC16
MB_FUNC_READ_COILS_ENABLED              = 1  // FC01
MB_FUNC_WRITE_COIL_ENABLED              = 1  // FC05
MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED    = 1  // FC15
MB_FUNC_READ_DISCRETE_INPUTS_ENABLED    = 1  // FC02
MB_FUNC_READWRITE_HOLDING_ENABLED       = 1  // FC23
MB_FUNC_OTHER_REP_SLAVEID_ENABLED       = 1  // FC17
MB_FUNC_HANDLERS_MAX                    = 16
MB_ASCII_TIMEOUT_SEC                    = 1
```

---

### A4. Register Map — CONFIRMED & CORRECTED

KB_v2_modbus.md trước đây ghi `usSRegHoldBuf[172]` và `S_COIL_NCOILS=56` — **SAI**. Giá trị thực từ `user_mb_app.h`:

| Type | Count | Buffer size | Start addr |
|---|---|---|---|
| Holding Registers | **100** (`S_REG_HOLDING_NREGS`) | `usSRegHoldBuf[100]` = 200 bytes | 0 |
| Input Registers | **100** (`S_REG_INPUT_NREGS`) | `usSRegInBuf[100]` = 200 bytes | 0 |
| Coils (output) | **64** (`S_COIL_NCOILS`) | `ucSCoilBuf[8]` = 8 bytes | 0 |
| Discrete Inputs | **64** (`S_DISCRETE_INPUT_NDISCRETES`) | `ucSDiscInBuf[8]` = 8 bytes | 0 |

---

### A5. GPIO ↔ Modbus Register Mapping (CONFIRMED)

**Đây là thông tin quan trọng nhất còn thiếu — nay được làm rõ:**

#### Digital Inputs (IN0–IN3) → Discrete Input Coils

Mapping thực hiện trong `input_poll()` (`input.c`):

```c
// IN_x HIGH → set bit x trong ucSDiscInBuf[0]
input_coils[0] |= (0x01 << input->input_num);   // set bit khi có input
input_coils[0] &= ~(0x01 << input->input_num);  // clear bit khi không có input
```

`input_coils` = pointer tới `ucSDiscInBuf` (Discrete Input buffer)

| Physical | Modbus | Register | Bit |
|---|---|---|---|
| IN0 (PB6) | Discrete Input | addr 0, `ucSDiscInBuf[0]` | bit 0 |
| IN1 (PB5) | Discrete Input | addr 0, `ucSDiscInBuf[0]` | bit 1 |
| IN2 (PB4) | Discrete Input | addr 0, `ucSDiscInBuf[0]` | bit 2 |
| IN3 (PB3) | Discrete Input | addr 0, `ucSDiscInBuf[0]` | bit 3 |

**Debounce:** 50ms (`MAX_TIME_DEBOUNCE = 50`), polling mỗi 10ms (`app_poll` timer_stamp > 10).

**Counter:** Mỗi IN_x có counter riêng lưu vào `input_reg[i+16]` (Input Register addr 16..19). Counter tăng mỗi lần edge rising, reset khi `output_coils[1]` bit tương ứng được set.

#### Digital Outputs (OUT0–OUT3) → Coils

Mapping thực hiện trong `output_poll()` (`output.c`):

```c
// Đọc bit x từ ucSCoilBuf[0] (output_coils[0]) → điều khiển GPIO
if(output_coils[0] >> output->output_num & 0x01) { bsp_output_on(i); }
else { bsp_output_off(i); }
```

`output_coils` = pointer tới `ucSCoilBuf` (Coil buffer)

| Physical | Modbus | Register | Bit |
|---|---|---|---|
| OUT0 (PA6) | Coil | addr 0, `ucSCoilBuf[0]` | bit 0 |
| OUT1 (PA7) | Coil | addr 0, `ucSCoilBuf[0]` | bit 1 |
| OUT2 (PB0) | Coil | addr 0, `ucSCoilBuf[0]` | bit 2 |
| OUT3 (PB1) | Coil | addr 0, `ucSCoilBuf[0]` | bit 3 |

Output state cũng được mirror vào `hoding_reg[i]` (Holding Register addr 0..3).

**Output reset mechanism:** `output_coils[1]` bit x = 1 → reset counter của input x về 0. Đây là cơ chế cross-buffer control từ Modbus master.

#### Zigbee Parameters → Input Registers (addr 20..40)

Phát hiện quan trọng trong `user_mb_app.c`:

```c
// Mỗi lần FC04 Read Input Registers được gọi:
usSRegInBuf[20] = bsp_get_address();      // addr 20: slave address
usSRegInBuf[21] = zigbee.param.pointType; // addr 21: Zigbee node type
usSRegInBuf[22] = zigbee.param.PAN_ID;   // addr 22: PAN ID
usSRegInBuf[23] = zigbee.param.Channel;  // addr 23: Channel
// ... tiếp tục đến addr 40 (20 Zigbee parameters)
```

**Input Register map đầy đủ:**

| Addr | Nội dung |
|---|---|
| 0 | Reserved |
| 1–15 | Undefined (available) |
| 16 | Counter IN0 |
| 17 | Counter IN1 |
| 18 | Counter IN2 |
| 19 | Counter IN3 |
| 20 | Slave address (`bsp_get_address()`) |
| 21 | `zigbee.param.pointType` |
| 22 | `zigbee.param.PAN_ID` |
| 23 | `zigbee.param.Channel` |
| 24 | `zigbee.param.transferModel` |
| 25 | `zigbee.param.userAddress` |
| 26 | `zigbee.param.antennaSelect` |
| 27–34 | `zigbee.param.macAddress[0..7]` (mỗi byte 1 register) |
| 35 | `zigbee.param.shortAddress` |
| 36 | `zigbee.param.isSecurity` |
| 37–40 | `zigbee.param.securityCode[0..3]` |

**Zigbee và Modbus bị coupling trực tiếp** — `user_mb_app.c` include `rf_app.h` và dùng `extern ZigbeeMesh_t zigbee`.

---

### A6. app_poll() Timing — CONFIRMED

```
app_poll() được gọi trong main FreeRTOS task loop (không delay):

Every call (unbounded rate):
    bsp_iwdg_refresh()
    rf_app_poll()          → zigbee_poll() mỗi 1ms
    user_app()             → empty (USER CODE BEGIN/END)

Every > 20ms:
    if Parency_Modbus && !rf_app_busy():
        eMBPoll(&modbus[i])     ← Modbus processing
        mb_uart_rx_task()       ← drain RX queue
    user_mb_app_poll()          ← empty hiện tại

Every > 10ms:
    input_app_poll(10)     → đọc GPIO IN0–IN3, update ucSDiscInBuf
    output_app_poll(10)    → đọc ucSCoilBuf, drive GPIO OUT0–OUT3
```

**Vấn đề phát hiện:** `eMBPoll()` chỉ được gọi mỗi >20ms. Với Modbus 115200 baud, response time thêm 20ms latency là chấp nhận được nhưng không tối ưu. Quan trọng hơn: **khi rf_app_busy() = true (đang configure Zigbee), Modbus bị treo hoàn toàn**.

---

### A7. Slave Address (CONFIRMED)

`bsp_get_address()` hardcoded trả về `1`:

```c
uint8_t bsp_get_address() { return 1; }
```

Không đọc GPIO, không đọc Flash. Slave address cố định = 1 cho toàn bộ V2. Nếu V3 cần configurable address, phải implement lại hàm này.

---

### A8. Zigbee–Modbus Coupling Summary

Đây là coupling không được document trong KB_v2_modbus.md gốc:

```
user_mb_app.c
    ├── #include "rf_app.h"          → include zigbee.h
    ├── extern ZigbeeMesh_t zigbee   → truy cập trực tiếp Zigbee state
    └── rf_register[20] = pointers vào zigbee.param.*
              → exposed qua Modbus FC04 Input Registers addr 21–40

app.c:
    → rf_app_busy() kiểm tra zigbee.event.event
    → nếu Zigbee đang busy: Modbus bị skip trong loop
```

**Khi port sang V3:** nếu tách Zigbee và Modbus thành module độc lập, phải cung cấp cơ chế cho `user_mb_app` đọc Zigbee params mà không cần `extern ZigbeeMesh_t zigbee` trực tiếp.