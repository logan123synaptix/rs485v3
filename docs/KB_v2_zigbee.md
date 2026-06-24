# KB_V2_ZIGBEE
## Zigbee Subsystem — Implementation Knowledge Base
**Source Repository:** `logan123synaptix/rs485v2` (RS485_IO_RF_V2)  
**Base Reference:** `docs/KB_v2.md` (Source of Truth — không lặp lại ở đây)  
**Scope:** Implementation knowledge bổ sung cho việc port sang RF_IO_RS485_V3

---

## 1. Purpose

Zigbee subsystem trong RS485_IO_RF_V2 đóng vai trò **RF communication driver** để kết nối board với một Zigbee RF module bên ngoài. Subsystem này:

- Quản lý configuration của Zigbee RF module (đọc/ghi tham số PAN ID, Channel, Node Type, v.v.)
- Cung cấp abstraction layer để app layer gửi/nhận qua interface `ZigbeeDriver_t`
- Trong chế độ `Parency_Transfer`: hoạt động như **transparent bridge** — dữ liệu RS485 được forward thẳng qua RF và ngược lại
- Trong chế độ `Parency_Modbus`: Zigbee module được configure trước, sau đó Modbus RTU chạy trên cùng UART physical với Zigbee

---

## 2. Responsibilities

| Responsibility | File |
|---|---|
| Zigbee module configuration protocol (read/write/reset/connect) | `zigbee.c / zigbee.h` |
| Event queue management cho async command flow | `zigbee.c` |
| Timeout-based response collection | `zigbee.c` |
| Auto-restart khi idle quá 10 giây | `zigbee.c` |
| UART I/O abstraction (write/read/available) | `ZigbeeDriver_t` (set up tại `rf_app.c`) |
| Physical UART binding (USART2 / `BSP_RF_COM_PORT`) | `board.c` |
| RF↔RS485 transparent bridge (khi `Parency_Transfer`) | `board.c` → `HAL_UARTEx_RxEventCallback` |
| Compile-time enable/disable | `app_config.h`: `#define ZIGBEE_ENABLE 1` |

---

## 3. Architecture

```
┌───────────────────────────────────────────────────────┐
│                    app_poll() loop                     │
│   (single FreeRTOS task: mainApp / user_app task)     │
├───────────────────────────────────────────────────────┤
│                     rf_app_poll()                      │
│              ↳ zigbee_poll(&zigbee, 1ms)              │
├───────────────────────────────────────────────────────┤
│              ZigbeeMesh_t (state machine)              │
│   event_queue[10] → current event → send cmd → wait  │
│              timeout → read response → callback       │
├───────────────────────────────────────────────────────┤
│                   ZigbeeDriver_t                       │
│    write()   ←──── rf_write() ────→ bsp_com_write()  │
│    read()    ←──── rf_read()  ────→ bsp_com_read()   │
│    available() ←─ rf_get_available() → bsp_com_available() │
├───────────────────────────────────────────────────────┤
│               BSP COM Layer (board.c)                  │
│      BSP_RF_COM_PORT = 2 → USART2 (huart2)           │
│      CQueue_t uart_queue[2] (256 bytes, byte-level)   │
├───────────────────────────────────────────────────────┤
│     HAL_UARTEx_ReceiveToIdle_DMA / _IT (USART2)      │
│     DMA CH1 (GPDMA) — buffer 256 bytes                │
├───────────────────────────────────────────────────────┤
│              Zigbee RF Module (External)               │
│         Physical: USART2 (PA2/TX, PA3/RX)            │
└───────────────────────────────────────────────────────┘
```

---

## 4. Module Structure

```
SynaptiX/services/zigbee/
    zigbee.h        — Public API, struct definitions, constants
    zigbee.c        — State machine, protocol encoding/decoding

SynaptiX/apps/
    rf_app.h        — rf_app_init(), rf_app_poll(), rf_app_busy()
    rf_app.c        — Instantiate ZigbeeMesh_t + ZigbeeDriver_t, callbacks
    app_config.h    — #define ZIGBEE_ENABLE 1 / LORA_ENABLE 0
    app.c           — Gọi rf_app_init() và rf_app_poll() trong main loop

SynaptiX/board/
    board.h         — BSP_RF_COM_PORT, bsp_com_* API
    board.c         — UART ring buffer, ISR callbacks, DMA RxEvent handler

SynaptiX/utils/
    cqueue.h / cqueue.c — Circular queue (dùng cho event queue và UART RX buffer)
```

---

## 5. Zigbee Module Type

| Attribute | Value |
|---|---|
| **Loại module** | Proprietary RF module (không phải stack Zigbee chuẩn IEEE 802.15.4 của Silicon Labs/TI) |
| **Giao tiếp với MCU** | **UART** (USART2, PA2/PA3) — **KHÔNG phải SPI4** |
| **Tốc độ UART** | Cấu hình qua `app_setting.rf.baudrate` (load từ Flash) |
| **Protocol** | **Binary proprietary** — không phải AT command |
| **Network roles** | Coordinator (1), Router (2), EndDevice (3) — cấu hình trong `ZigbeeParameter_t.pointType` |
| **Network join** | Không có explicit join flow trong firmware này — module tự join theo PAN_ID + Channel được write vào |
| **Addressing** | PAN_ID (16-bit), userAddress (16-bit), shortAddress (16-bit), macAddress (8 bytes) |
| **Security** | Optional: `isSecurity` flag + `securityCode[4]` |

> **Quan trọng:** KB_v2.md ghi Zigbee dùng SPI4. **Sai.** Code thực tế hoàn toàn dùng UART (USART2). SPI4 chỉ dùng cho LoRa. Đây là hidden assumption nguy hiểm nhất khi port.

---

## 6. Protocol Type

Giao thức là **binary framing** với cấu trúc:

### 6.1 Command Frame (MCU → Module)

```
Byte 0:   0xFC        — Start byte
Byte 1:   LENGTH      — Số byte data (không tính start + length + checksum)
Byte 2:   INS         — Instruction code
Byte 3..N: Data       — Payload (tuỳ command)
Byte N+1: XY          — Checksum = sum(byte[0]..byte[N])
```

| Command | INS Code | Frame Length | Mô tả |
|---|---|---|---|
| Link/Connect | 0x04 | 9 bytes | Handshake, lấy FW version |
| Restart/Reset | 0x06 | 9 bytes | Reset module |
| Read Parameters | 0x0E | 9 bytes | Đọc toàn bộ config |
| Write Parameters | 0x07 | 42 bytes | Ghi toàn bộ config |
| Request Signal | 0x0C | 9 bytes | Query RSSI/signal |
| Enter Low Power | 0x12 | 9 bytes | Vào chế độ tiết kiệm điện |
| Query EndDevice Position | 0x0A | 7 bytes | Chỉ Coordinator mới gọi được |

### 6.2 Response Frame (Module → MCU)

```
Byte 0:   0xFA        — Start byte (khác 0xFC của command)
Byte 1:   DATA_LEN    — Số byte payload
Byte 2:   0x0A        — Response type byte (fixed trong hầu hết responses)
Byte 3..N: Payload
Byte N+1: XY          — Checksum = sum(byte[0]..byte[N])
```

### 6.3 Checksum Algorithm

```c
// XY = sum của tất cả byte từ byte[0] đến byte[inputLen-2]
uint8_t zigbeeGetXY(uint8_t *inputData, uint16_t inputLen) {
    uint8_t result = 0;
    for (i = 0; i < inputLen - 1; i++) result += inputData[i];
    return result;
}
```

### 6.4 Response Validation Pattern

Mỗi response có 4 validation conditions:
1. `inputData[0] == 0xFA` (start byte)
2. `inputData[1] == EXPECTED_LEN` (length cụ thể theo command)
3. `inputData[2] == 0x0A` (response type)
4. `inputData[inputLen-1] == XY` (checksum valid)

Tất cả 4 phải pass → `ZIGBEE_RES_SUCCESS`. Bất kỳ fail → `ZIGBEE_RES_FAIL`.

### 6.5 Read Response Payload (48 bytes parameter block)

Tổng frame Read Response = 4 (header) + 48 (payload) + 1 (checksum) = 53 bytes

```
readHeaderLen = 4 → payload bắt đầu từ byte [4]
[4]       pointType
[5][6]    PAN_ID (big-endian)
[7]       Channel
[8]       transferModel
[9][10]   userAddress (big-endian)
[11][12]  X7X8
[13]      uartBaudRate
[14]      uartDataBits
[15]      uartStopBits
[16]      uartParity
[17][18]  X13X14
[19]      antennaSelect
[20..27]  macAddress[8]
[28]      prePointType
[29][30]  prePAN_ID
[31]      preChannel
[32]      preTransferModel
[33][34]  preUserAddress
[35][36]  X31X32
[37]      preUartBaudRate
[38]      preUartDataBits
[39]      preUartStopBits
[40]      preUartParity
[41][42]  X37X38
[43]      preAntennaSelect
[44][45]  shortAddress
[46]      X42
[47]      isSecurity
[48..51]  securityCode[4]
[52]      XY (checksum)
```

---

## 7. Public APIs

```c
// zigbee.h

void zigbee_init(ZigbeeMesh_t *zigbee, ZigbeeDriver_t *driver);
// Khởi tạo struct, zero param, init event_queue (static, capacity=10)

void zigbee_poll(ZigbeeMesh_t *zigbee, uint32_t timestamp);
// Phải được gọi mỗi 1ms trong task loop. Là engine của toàn bộ state machine.

void zigbee_connect(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_LINK_MODULE, timeout=300ms

void zigbee_reset_module(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_RESET_MODULE, timeout=300ms

void zigbee_read_module(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_READ_MODULE, timeout=1000ms

void zigbee_write_module(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_WRITE_MODULE, timeout=1000ms

void zigbee_request_signal(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_GET_POSITION, timeout=300ms

void zigbee_query_ed_pos(ZigbeeMesh_t *zigbee, ZB_Callback_Func cb, void *arg);
// Enqueue ZB_EVENT_QUERY_POS, timeout=2000ms — CHỈ Coordinator (pointType==1)
```

**Callback signature:**
```c
typedef void (*ZB_Callback_Func)(ZigbeeMesh_t *zigbee, uint8_t isSuccess, void *arg);
// isSuccess: ZIGBEE_RES_SUCCESS(0x01), ZIGBEE_RES_FAIL(0x00), ZIGBEE_RES_TIMEOUT(0xFF)
```

**Driver interface:**
```c
typedef struct ZigbeeDriver {
    void     (*write)(uint8_t *buff, uint32_t len);
    void     (*read)(uint8_t *buff, uint32_t len);
    uint32_t (*available)();
} ZigbeeDriver_t;
```

---

## 8. Initialization Flow

```
app_init()
    └── rf_app_init()
            ├── indicator_init()
            ├── zigbee_driver.write   = rf_write        // → bsp_com_write(BSP_RF_COM_PORT)
            │   zigbee_driver.read    = rf_read         // → bsp_com_read(BSP_RF_COM_PORT)
            │   zigbee_driver.available = rf_get_available // → bsp_com_available(BSP_RF_COM_PORT)
            ├── zigbee_init(&zigbee, &zigbee_driver)
            │       ├── zigbee->driver = driver
            │       ├── memset param = 0
            │       ├── memset event = 0
            │       └── cqueue_init_static(event_queue, event_list[10], ...)
            ├── zigbee_connect(&zigbee, zigbee_connect_cb, NULL)    // enqueue
            ├── zigbee_read_module(&zigbee, zigbee_read_cb, NULL)   // enqueue
            └── zigbee_reset_module(&zigbee, zigbee_reset_cb, NULL) // enqueue

// → 3 events được xếp vào queue ngay khi init
// → Thứ tự thực thi: CONNECT → READ → RESET (FIFO queue)
```

**Lưu ý init sequence:** `zigbee_write_module()` trong `rf_app_init()` bị comment out — tức là write config không được gọi tự động, chỉ được trigger từ shell command hoặc user_app.

---

## 9. TX Flow (MCU → RF Module)

```
App gọi zigbee_connect() / zigbee_read_module() / v.v.
    ↓
cqueue_send(&event_queue, &event_tmp)    // enqueue event
    ↓
[Trong zigbee_poll(), khi event.event == ZB_EVENT_NONE]
    ↓
cqueue_receive(&event_queue, &event)     // dequeue
    ↓
switch(event.event):
    ZB_EVENT_LINK_MODULE   → zigbee_send_connect_cmd()
    ZB_EVENT_READ_MODULE   → zigbee_send_read_cmd()
    ZB_EVENT_WRITE_MODULE  → zigbee_send_write_cmd() [gọi zigbeeGetWriteIns() trước]
    ZB_EVENT_RESET_MODULE  → zigbee_send_restart_cmd()
    ZB_EVENT_GET_POSITION  → zigbee_send_request_signal_cmd()
    ZB_EVENT_QUERY_POS     → zigbee_query_end_device_pos()
    ZB_EVENT_ENTER_LOWPOWER → zigbee_send_enter_lowpower_cmd()
    ↓
driver->write(frame, len)
    ↓
bsp_com_write(BSP_RF_COM_PORT=2, buff, len)
    ↓
HAL_UART_Transmit(&huart2, buff, len, HAL_MAX_DELAY)  // BLOCKING
```

**Lưu ý:** TX là **blocking** với `HAL_MAX_DELAY`. Không có DMA TX cho RF port trong V2.

---

## 10. RX Flow (RF Module → MCU)

### 10.1 ISR → Buffer Path

```
USART2 RX interrupt (DMA hoặc IT)
    ↓
HAL_UARTEx_RxEventCallback(huart2, Size)   [board.c]
    ↓
[if Parency_Transfer mode AND zigbee.event == ZB_EVENT_NONE]
    → HAL_UART_Transmit(&huart2, data, Size)  // forward thẳng sang RS485 TX
    
[always]
    → for each byte: cqueue_send(&uart_queue[BSP_RF_COM_PORT], byte)
    → HAL_UARTEx_ReceiveToIdle_IT(bsp_uart[BSP_RF_COM_PORT], dma_buf, 256)  // re-arm
```

### 10.2 Poll → Process Path

```
[Trong zigbee_poll(), khi event.tick >= event.timeout]
    ↓
zigbee->buff_len = driver->available()   // = uart_queue[2].count
    ↓
driver->read(zigbee->buff, buff_len)     // copy từ ring buffer vào zigbee->buff
    ↓
[if buff_len == 0] → cb(zigbee, ZIGBEE_RES_TIMEOUT, arg)
    ↓
switch(event.event):
    ZB_EVENT_LINK_MODULE  → zigbee_connect_process()
    ZB_EVENT_READ_MODULE  → zigbeeReadProcess()
    ZB_EVENT_WRITE_MODULE → zigbee_write_process()
    ZB_EVENT_RESET_MODULE → zigbee_reset_process()
    ZB_EVENT_GET_POSITION → zigbee_request_signal_process()
    ZB_EVENT_QUERY_POS    → zigbee_query_ed_process() [trả SUCCESS luôn, không validate]
    ↓
[if res != FAIL] → cb(zigbee, res, arg)
    ↓
event.event = ZB_EVENT_NONE   // giải phóng để dequeue event tiếp theo
```

---

## 11. UART Integration

| Parameter | Value |
|---|---|
| **UART peripheral** | USART2 (`huart2`) |
| **Pins** | PA2 (TX), PA3 (RX) |
| **DMA** | GPDMA CH0 (RX), CH1 (TX) — từ CubeMX config |
| **BSP index** | `BSP_RF_COM_PORT = 2` |
| **UART array** | `bsp_uart[2] = &huart2` |
| **RX mode** | `HAL_UARTEx_ReceiveToIdle_DMA()` → initial; sau đó re-arm bằng `HAL_UARTEx_ReceiveToIdle_IT()` |
| **TX mode** | `HAL_UART_Transmit()` — **blocking** |
| **RX buffer** | `uart_data_dma[BSP_RF_COM_PORT][256]` — DMA receive buffer |
| **RX ring buffer** | `uart_queue[BSP_RF_COM_PORT]` — CQueue_t, 256 bytes |
| **Baudrate** | Từ `app_setting.rf.baudrate` (Flash-persisted) |

**Bug tiềm ẩn trong `bsp_com_init()`:**
```c
// Bug: điều kiện if dùng assignment (=) thay vì comparison (==)
if(i = BSP_RF_COM_PORT) {   // ← LUÔN TRUE cho i=2, nhưng set i=2 với mọi i
    HAL_UARTEx_ReceiveToIdle_DMA(...)
} else
    HAL_UART_Receive_IT(...)
// Kết quả: chỉ USART2 nhận DMA. USART1 và LPUART1 nhận IT mode (vòng lặp break ngay)
```

---

## 12. ISR Integration

| ISR/Callback | Trigger | Hành động |
|---|---|---|
| `HAL_UARTEx_RxEventCallback` | USART2 Idle line / DMA half-complete | Copy data vào `uart_queue[2]`; nếu Parency_Transfer → forward sang USART1 (RS485) |
| `HAL_UART_RxCpltCallback` | USART1/LPUART1 IT mode (byte-by-byte) | Copy 1 byte vào `uart_queue[i]`, re-arm IT |
| `HAL_UART_TxCpltCallback` | Bất kỳ UART TX complete | Gọi `com_tx_callback[i].cb` nếu có |

**Zigbee subsystem KHÔNG có ISR riêng.** Toàn bộ data path qua ring buffer → poll.

---

## 13. Buffer Management

| Buffer | Kích thước | Owner | Lifetime |
|---|---|---|---|
| `uart_data_dma[2][256]` | 256 bytes | `board.c` (static) | Toàn bộ runtime — DMA target |
| `uart_queue[2]` (CQueue) | 256 bytes | `board.c` (static) | Toàn bộ runtime — FIFO ring buffer |
| `zigbee->buff[1024]` | 1024 bytes (256×4) | `ZigbeeMesh_t` (static trong `rf_app.c`) | Toàn bộ runtime |
| `writeIns[42]` | 42 bytes | `zigbee.c` (static global) | Toàn bộ runtime — **shared, not thread-safe** |

**Ownership model:**
- ISR/DMA write vào `uart_data_dma[]`
- `HAL_UARTEx_RxEventCallback` copy từ DMA buffer vào `uart_queue[]`
- `zigbee_poll()` đọc từ `uart_queue[]` vào `zigbee->buff[]`
- Không có dynamic allocation. Không có mutex.

---

## 14. State Machine

```
State được lưu trong: zigbee->event.event (ZigbeeEventType_t)

States:
    ZB_EVENT_NONE         — Idle. Dequeue event tiếp hoặc auto-restart nếu idle 10s
    ZB_EVENT_LINK_MODULE  — Đang chờ response của connect command
    ZB_EVENT_READ_MODULE  — Đang chờ response của read parameters
    ZB_EVENT_WRITE_MODULE — Đang chờ response của write parameters
    ZB_EVENT_RESET_MODULE — Đang chờ response của reset
    ZB_EVENT_ENTER_LOWPOWER — Đang chờ response của low power
    ZB_EVENT_GET_POSITION   — Đang chờ signal response
    ZB_EVENT_QUERY_POS      — Đang chờ end device position

Transitions:
    NONE → [event in queue]     → send command → ACTIVE_EVENT (tick=0)
    ACTIVE_EVENT → tick++ each poll
    ACTIVE_EVENT → [tick >= timeout] → read response → validate → callback → NONE

Timeout values:
    LINK_MODULE:  300ms
    RESET_MODULE: 300ms
    GET_POSITION: 300ms
    READ_MODULE:  1000ms
    WRITE_MODULE: 1000ms
    QUERY_POS:    2000ms

Auto-restart:
    NONE state + available()==0 → time_reset += 1ms → if >= 10000ms → send restart
    NONE state + available() > 0 → time_reset = 0 (reset counter)
```

**Quan trọng:** State machine là **one-event-at-a-time**. Trong khi đang xử lý 1 event, tất cả `cqueue_send()` tiếp theo chỉ queue lại — không bị drop nhưng không được xử lý cho đến khi event hiện tại done.

---

## 15. Concurrency Model

**Không có mutex, không có semaphore, không có FreeRTOS IPC** trong Zigbee subsystem.

| Component | Thread/Context |
|---|---|
| `zigbee_poll()` | mainApp FreeRTOS task |
| `zigbee_connect()` / enqueue APIs | mainApp FreeRTOS task (cùng context) |
| `HAL_UARTEx_RxEventCallback` | IRQ context (ISR priority 5) |
| `bsp_com_write()` / TX | mainApp task (blocking HAL_UART_Transmit) |

**Race condition tiềm ẩn:** `cqueue_send()` trong ISR và `cqueue_receive()` trong task chạy trên cùng `uart_queue[]` không có critical section. Trong hệ thống single-core Cortex-M33, byte-level operations đủ atomic cho phần lớn trường hợp nhưng không đảm bảo về mặt lý thuyết.

**Không có RTOS task riêng cho Zigbee.** Toàn bộ chạy trong `mainApp` task duy nhất.

---

## 16. Dependency Graph

```
zigbee.c / zigbee.h
    ← cqueue.h / cqueue.c         (event queue)
    ← logger.h / logger.c         (log_info, log_debug, log_print_hex)
    ← string.h                    (memset)

rf_app.c / rf_app.h
    ← zigbee.h
    ← board.h                     (bsp_com_*, bsp_get_tick)
    ← logger.h
    ← app_config.h                (ZIGBEE_ENABLE compile flag)
    ← indicator.h                 (indicator_set_net_status)

board.c / board.h
    ← cqueue.h
    ← zigbee.h                    (extern ZigbeeMesh_t zigbee — kiểm tra event state trong ISR)
    ← app_settings.h              (app_setting.app_mode — kiểm tra Parency_Transfer trong ISR)
    ← HAL UART / GPDMA

app.c
    ← rf_app.h
    ← app_settings.h
    ← board.h
```

**Circular dependency quan trọng:**  
`board.c` include `zigbee.h` và dùng `extern ZigbeeMesh_t zigbee` → board layer biết về zigbee application object. Đây là vi phạm layer separation và là **hidden coupling** nguy hiểm nhất khi port.

---

## 17. Hidden Assumptions

| # | Assumption | Vị trí | Rủi ro khi port |
|---|---|---|---|
| 1 | Zigbee dùng UART (USART2), không phải SPI4 | `rf_app.c`, `board.c` | KB_v2.md ghi nhầm SPI4 — phải dùng UART trên V3 |
| 2 | `zigbee_poll()` được gọi mỗi 1ms chính xác | `rf_app.c`: `if(tick - rf_time_stamp >= 1)` | Nếu task bị preempt hoặc delay, timeout counting sẽ drift |
| 3 | Chỉ có 1 event được xử lý tại 1 thời điểm | `zigbee_poll()` logic | Enqueue nhiều command → xử lý tuần tự, không parallel |
| 4 | `writeIns[42]` là static global | `zigbee.c` | Không re-entrant. Nếu V3 có multi-instance Zigbee → data corruption |
| 5 | `bsp_com_read()` không biết khi nào frame kết thúc | `rf_app.c` `rf_read()` | Zigbee_poll đọc "tất cả available bytes" sau timeout — không có frame delimiter khác |
| 6 | `board.c` phụ thuộc `zigbee.event.event` tại ISR level | `HAL_UARTEx_RxEventCallback` | Parency_Transfer mode kiểm tra `zigbee.event == ZB_EVENT_NONE` trong ISR — race condition nếu task đang ghi |
| 7 | `bsp_com_init()` bug: `if(i = BSP_RF_COM_PORT)` | `board.c` | Assignment thay vì comparison — chỉ tình cờ đúng cho BSP_RF_COM_PORT=2 |
| 8 | Auto-restart timer dùng tích lũy `+= timestamp` | `zigbee_poll()` | Nếu timestamp > 1 (poll bị delay), counter tăng nhanh hơn, restart sớm hơn |
| 9 | `zigbee_query_ed_pos()` process không validate response | `zigbee_query_ed_process()` return SUCCESS unconditionally | Mọi response (hoặc không có response) đều trả SUCCESS |
| 10 | Re-arm RX dùng `ReceiveToIdle_IT` (không phải DMA) sau lần đầu | `HAL_UARTEx_RxEventCallback` | Lần đầu dùng DMA, sau đó dùng IT — throughput khác nhau |

---

## 18. Design Decisions

| Decision | Rationale |
|---|---|
| Polling model (không interrupt-driven) | Đơn giản hóa state machine, tránh ISR complexity cho protocol parsing |
| Timeout-based response collection | Module không signal "response complete" — MCU chỉ chờ đủ thời gian rồi đọc |
| Event queue depth = 10 | Đủ cho init sequence (connect + read + reset = 3 events) |
| `ZIGBEE_BUFFER_SIZE = 256*4 = 1024` | Oversized so với response thực tế (max ~53 bytes) — buffer safety margin |
| Driver injection pattern (ZigbeeDriver_t) | Cho phép mock driver trong unit test, hoặc swap UART ↔ SPI nếu cần |
| Timestamp accumulation thay vì absolute tick | Zigbee module không cần `bsp_get_tick()` trực tiếp → portable hơn |
| No network join flow | Module được assume đã pre-configured hoặc configure qua write command trước |

---

## 19. Risks

| Risk | Severity | Mô tả |
|---|---|---|
| **Wrong interface assumption (SPI vs UART)** | CRITICAL | KB_v2.md ghi SPI4, thực tế là UART. Port phải dùng UART. |
| **board.c coupled to zigbee.h** | HIGH | ISR trong board.c đọc `zigbee.event.event` — vi phạm layer, khó test, race condition |
| **No frame synchronization** | HIGH | Nếu DMA nhận partial frame, `zigbee_poll()` sẽ đọc incomplete data và fail validation → timeout |
| **Blocking TX in task context** | MEDIUM | `HAL_UART_Transmit(HAL_MAX_DELAY)` block task — nếu module không pull UART, task treo |
| **writeIns static global** | MEDIUM | Non-reentrant. V3 cần xem xét nếu có concurrent write |
| **No retry mechanism** | MEDIUM | Nếu response timeout, callback nhận `ZIGBEE_RES_TIMEOUT` — không có auto-retry. App phải retry. |
| **Auto-restart at 10s idle** | LOW | Restart có thể gián đoạn data session nếu module đang hoạt động bình thường |
| **CQueue not thread-safe** | LOW | Single-core M33, hiếm khi gây vấn đề nhưng không đúng về mặt lý thuyết |

---

## 20. Porting Notes for RF_IO_RS485_V3

### Interface Requirements

V3 phải cung cấp `ZigbeeDriver_t` với 3 function pointers:

```c
ZigbeeDriver_t driver = {
    .write     = /* function gửi bytes ra UART RF port */,
    .read      = /* function đọc bytes từ ring buffer RF UART */,
    .available = /* function trả số bytes available trong buffer */
};
```

Không phụ thuộc vào SPI4, không phụ thuộc vào HAL trực tiếp.

### Tickstamp

`zigbee_poll(zigbee, timestamp)` nhận timestamp tính bằng ms. V3 phải:
- Gọi `zigbee_poll()` đều đặn với `timestamp = elapsed_ms_since_last_call`
- Hoặc gọi mỗi 1ms với `timestamp = 1`

### Decoupling board.c khỏi zigbee.h

Trong V3, `HAL_UARTEx_RxEventCallback` (hoặc tương đương) **KHÔNG ĐƯỢC** đọc trực tiếp `zigbee.event.event`. Thay thế:

```c
// V3: dùng flag hoặc callback
static volatile bool zigbee_cmd_active = false;
// Set bởi zigbee_poll() khi có event active
// Đọc bởi ISR để quyết định có forward hay không
```

### Parency_Transfer Mode

Trong V2, transparent bridge được implement tại ISR level (`HAL_UARTEx_RxEventCallback`). V3 cần quyết định: giữ logic này ở ISR (low latency) hay đưa vào task (safer). Nếu giữ ISR:
- Phải có cơ chế signal "zigbee in configuration mode" mà ISR đọc được safely
- `volatile bool` hoặc atomic flag là đủ trên single-core

---

## 21. Reusable Components (Port trực tiếp)

| Component | File | Ghi chú |
|---|---|---|
| `ZigbeeMesh_t`, `ZigbeeParameter_t`, `ZigbeeEvent_t`, `ZigbeeDriver_t` | `zigbee.h` | Struct definitions — copy nguyên |
| `ZigbeeEventType_t` enum | `zigbee.h` | Copy nguyên |
| All `#define` constants (INS codes, role types, baud codes) | `zigbee.h` | Copy nguyên |
| `zigbee_init()` | `zigbee.c` | Copy nguyên |
| `zigbee_poll()` | `zigbee.c` | Copy nguyên — chỉ cần driver abstraction đúng |
| All public API functions (`connect`, `reset`, `read`, `write`, `request_signal`, `query_ed_pos`) | `zigbee.c` | Copy nguyên |
| All `static` command frame arrays (`linkIns`, `readIns`, v.v.) | `zigbee.c` | Copy nguyên |
| `zigbeeGetWriteIns()` | `zigbee.c` | Copy nguyên |
| `zigbeeGetXY()` checksum | `zigbee.c` | Copy nguyên |
| All response process functions | `zigbee.c` | Copy nguyên |
| `CQueue_t` | `utils/cqueue.h / cqueue.c` | Portable — không phụ thuộc HAL |

---

## 22. Components Requiring Adaptation

| Component | File | Thay đổi cần thiết |
|---|---|---|
| `ZigbeeDriver_t` binding | `rf_app.c` | Bind lại với UART layer của V3 |
| Callbacks (`zigbee_connect_cb`, `zigbee_read_cb`, `zigbee_reset_cb`) | `rf_app.c` | Adapt sang indicator/LED API của V3 |
| `rf_app_init()` init sequence | `rf_app.c` | Adapt sang boot sequence của V3; xem xét lại thứ tự connect→read→reset |
| `rf_app_poll()` timing | `rf_app.c` | Adapt sang timing mechanism của V3 (FreeRTOS delay, SysTick, v.v.) |
| `logger` calls trong `zigbee.c` | `zigbee.c` | Nếu V3 dùng logger khác, cần swap. Nếu giữ logger V2 thì copy `logger.h/.c` |

---

## 23. Components Requiring Rewrite

| Component | File | Lý do |
|---|---|---|
| `HAL_UARTEx_RxEventCallback` Parency_Transfer logic | `board.c` | Phải decoupled khỏi `extern ZigbeeMesh_t zigbee` |
| `bsp_com_init()` RF port DMA setup | `board.c` | Bug `if(i = ...)` phải fix; V3 có thể dùng peripheral khác |
| `bsp_com_write()` cho RF port | `board.c` | V3 có thể cần non-blocking TX (DMA TX thay vì blocking) |
| `app_config.h` ZIGBEE_ENABLE | V3 config | Tạo lại phù hợp với V3 build system |
| Parency_Transfer data path | `board.c` / `rs4852RF.c` | V3 cần xác định lại bridge logic và decoupling |

---

*Tài liệu này bổ sung KB_v2.md — không thay thế. Đọc KB_v2.md trước để hiểu platform, clock, peripheral map tổng thể.*