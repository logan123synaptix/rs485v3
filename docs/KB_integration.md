# RF_IO_RS485_V3_INTEGRATION_PLAN

> **Ngày tạo:** 2026-06-24  
> **Phiên bản KB:** KB_v3.md · KB_v2.md · KB_v2_modbus.md · KB_v2_zigbee.md · KB_v2_shell.md  
> **Source code verified:** RF_IO_RS485_V3 @ `logan123synaptix/rs485v3` · RS485_RF_IO_V2 @ `logan123synaptix/rs485v2`  
> **Trạng thái:** Migration Blueprint — CHƯA có code, CHƯA có patch

---

## ⚠️ MÂUTHUẪN GIỮA KB VÀ SOURCE CODE (Phải xác nhận trước khi code)

| # | KB nói | Source code thực tế | Mức độ |
|---|---|---|---|
| C1 | KB_v3.md: `SYSCLK = 32MHz (HSE = 25MHz ngoài)` | `main.c`: `SYSCLKSource = HSI`, `HSIDiv = DIV2` → **HSI 64MHz / 2 = 32MHz** — không dùng HSE | HIGH |
| C2 | KB_v3.md: `SystemClock_Config() [HSE 25MHz → SYSCLK 32MHz]` | Không có HSE trong clock config. Dùng internal HSI. PH0/PH1 có trong KB nhưng cần verify trên .ioc thực tế | HIGH |
| C3 | KB_v3.md: KB-4 execution flow ghi `MX_CORTEX_M33_NS_Init()` và `MX_PWR_Init()` | `main.c` không gọi hai hàm này trong init sequence | MEDIUM |
| C4 | KB_v2.md (Dependency Graph section 8): `zigbee → dùng SPI4` | KB_v2_zigbee.md Section 5 và source đều xác nhận Zigbee dùng **UART (USART2)**, không phải SPI4 | HIGH |
| C5 | KB_v3.md KB-5: `libs/` chứa modbus/, dio/, shell/, zigbee/, lora/ | `libs/CMakeLists.txt` thực tế: chỉ có `logger`, `stm32-mw-lwip`, `tinyusb` — **không có modbus/shell/zigbee** | CRITICAL |

**Kết luận C5:** Đây là xác nhận quan trọng nhất — V3 **chưa có** bất kỳ service nào (Modbus, Shell, Zigbee) được implement. `libs/` hiện tại chỉ là networking stack. Toàn bộ services phải được tạo mới.

---

## Architecture Gap Analysis

### V2 Architecture Summary

**Platform:** STM32H523CCU6, Cortex-M33, **250 MHz** (HSE 8MHz → PLL), 256KB Flash, 272KB RAM  
**SDK:** STM32Cube FW_H5 V1.5.1 · **Build:** GCC Makefile

**Layer structure:**
```
[SynaptiX Apps]           — apps/: app.c, rf_app.c, shell_app.c, input.c, output.c
       ↓
[SynaptiX Services]       — services/: Modbus stack, mbport, shell engine, zigbee driver, lora driver
       ↓
[Board Abstraction Layer] — board/: board.c/h (CQueue UART, GPIO, Timer dispatch)
       ↓
[HAL + CMSIS]             — Drivers/STM32H5xx_HAL_Driver/
       ↓
[Hardware Peripherals]    — USART1(RS485), USART2(Zigbee DMA), SPI4(LoRa), USB(3×CDC ACM), TIM2/3/6, GPDMA
```

**USB Stack:** AL94 I-CUBE-USBD-COMPOSITE v1.0.3 — 3× CDC ACM  
**RTOS:** FreeRTOS 10.6.2, CMSIS-RTOS2, heap_4, **128KB heap**  
**Task model:** Single `mainApp` task cho toàn bộ logic (Modbus poll + Zigbee poll + Shell + GPIO)  
**Modbus timer:** TIM2 (RS485 3.5T) + TIM3 (RF 3.5T), period = **1ms** (quá thô cho 115200 baud)  
**Shell I/O:** USB CDC channel 0 (TX/RX) + LPUART1 (logger output only)  
**Shell architecture:** `shell_receive_task` (FreeRTOS task, priority MAX-1) polling USB CDC  
**Zigbee I/O:** USART2 + GPDMA, blocking TX (`HAL_MAX_DELAY`), timeout-based RX  
**Watchdog:** IWDG enabled (~2s timeout)

### V3 Architecture Summary

**Platform:** STM32H523CCUx, Cortex-M33, **32 MHz** (HSI/2 — *confirmed từ source*), 256KB Flash, 272KB RAM  
**SDK:** STM32Cube FW_H5 V1.6.0 · **Build:** CMake 3.22+ + Ninja

**Layer structure:**
```
[Web Browser / REST Client]
       ↓ HTTP/REST
[Mongoose Web Server]         — mongoose/: HTTP server + REST API + Web UI
       ↓
[USB Networking]              — usb_net/: ECM + RNDIS → IP 192.168.7.1
       ↓
[TinyUSB Stack]               — libs/tinyusb/: CDC ACM + CDC ECM + RNDIS
       ↓
[lwIP TCP/IP Stack]           — libs/stm32-mw-lwip/
       ↓
[Application Services]        — libs/ (HIỆN TẠI TRỐNG — phải tạo)
       ↓
[FreeRTOS X-CUBE-FREERTOS 1.5.0] — Heap4, CMSIS-RTOS2, 131077 bytes heap
       ↓
[STM32H5 HAL V1.6.0]
       ↓
[Hardware]  LPUART1(Shell), USART1(RS485), USART2(Zigbee), USB(TinyUSB), TIM6(HAL timebase)
```

**USB Stack:** TinyUSB (composite: CDC ACM + CDC ECM + RNDIS)  
**Network:** lwIP + USB ECM/RNDIS → IP 192.168.7.1  
**Web server:** Mongoose (event-driven, single-file)  
**RTOS:** FreeRTOS (X-CUBE-FREERTOS 1.5.0), CMSIS-RTOS2, heap_4, **131077 bytes heap**  
**Task model hiện tại:** `defaultTask` (init) + `web_server_task` (Mongoose poll) — services chưa có  
**Logger:** `libs/logger/` đã tồn tại (logger.c/h riêng)  
**Watchdog:** IWDG **disabled** (comment out trong hal_conf.h)  
**DMA (GPDMA):** Không thấy trong init sequence V3 — chưa được cấu hình  
**SPI4:** **Disabled** (comment out trong hal_conf.h) — không có LoRa SPI  
**TIM2/TIM3:** Không có trong V3 init — chưa được cấu hình cho Modbus timer

### Key Architectural Differences

| Dimension | V2 | V3 | Impact khi Port |
|---|---|---|---|
| CPU Clock | 250 MHz | 32 MHz (HSI/2) | **CRITICAL:** Baud rate prescaler, TIM period, tất cả timing tính toán phải recalculate với PCLK của V3 |
| HAL SDK | FW_H5 V1.5.1 | FW_H5 V1.6.0 | LOW: API tương thích, nhưng cần verify nếu có breaking change |
| Build system | GCC Makefile | CMake + Ninja | HIGH: Tất cả source mới phải có CMakeLists.txt và được link vào `libs` target |
| USB Stack | AL94 CDC ACM ×3 | TinyUSB (CDC ACM + ECM + RNDIS) | HIGH: USB API hoàn toàn khác. Shell I/O phải bind lại |
| Network | Không có (USB chỉ CDC ACM) | lwIP + USB ECM/RNDIS → IP 192.168.7.1 | MEDIUM: Zigbee/Modbus status expose qua REST API mới |
| Web server | Không có | Mongoose REST API + Web UI | MEDIUM: Cần thêm REST endpoints cho Modbus, DIO, Zigbee |
| FreeRTOS heap | 128 KB | 131 KB | LOW: Gần bằng nhau, nhưng V3 thêm lwIP + Mongoose |
| Task model | Single mainApp task | Multi-task (defaultTask + web_server_task + services TBD) | HIGH: Phải thiết kế task architecture cho Modbus, Shell, Zigbee |
| Shell I/O | USB CDC ACM ch.0 (V2 BSP) | LPUART1 (KB_v3 KB-2) + USB CDC ACM (TinyUSB) | HIGH: Input source thay đổi |
| Zigbee UART | USART2 + GPDMA (DMA CH0/CH1) | USART2 (không có DMA init) | HIGH: DMA chưa được cấu hình trong V3 |
| Modbus timer | TIM2 + TIM3 (1ms period, sai) | Không có TIM2/TIM3 | CRITICAL: Phải thêm và cấu hình timer mới |
| Board layer | `SynaptiX/board/board.c` (BSP đầy đủ) | Không tồn tại | HIGH: Phải tạo BSP layer mới cho V3 |
| IWDG | Enabled (~2s) | Disabled | MEDIUM: Quyết định có enable không |
| SPI4 / LoRa | Có (SPI4 enabled) | Disabled | LOW: LoRa không trong scope port |
| GPIO labels | IN0-IN3 (PB6-PB3), OUT0-OUT3 (PA6,PA7,PB0,PB1) | DI1-4 (PB3-PB6), DO1-4 (PA6,PA7,PB0,PB1) | LOW: Cùng pin, khác label. Mapping logic giống nhau |
| DE pin | PB13 (USART1_DE) | PB13 (RS485 DE) | NONE: Giống nhau |
| Logger | `services/logger.c` trong V2 | `libs/logger/logger.c` trong V3 | LOW: Đã có sẵn trong V3 |

### Migration Challenges

1. **Clock 250MHz → 32MHz:** Tất cả timer prescaler phải tính lại. Modbus 3.5T timer đặc biệt nghiêm trọng.
2. **Không có BSP layer trong V3:** V2 phụ thuộc hoàn toàn vào `board.c` (CQueue, HAL callbacks, COM abstraction). V3 cần BSP tương đương.
3. **TIM2/TIM3 không tồn tại trong V3:** Modbus cần 2 hardware timer cho 3.5T inter-frame timeout.
4. **USB API thay đổi hoàn toàn:** Shell TX/RX phải bind lại với TinyUSB API thay vì AL94.
5. **CMake build system:** Mọi file mới phải đăng ký trong CMakeLists.txt đúng.
6. **lwIP + Mongoose overhead:** Với 32MHz và 131KB heap chia sẻ với lwIP, Mongoose, TinyUSB — memory budget cho services khá hạn chế.
7. **Zigbee GPDMA chưa cấu hình:** V3 chưa có DMA setup cho USART2. Phải quyết định: giữ DMA hay dùng IT mode.
8. **Zigbee–Modbus coupling trong V2:** `user_mb_app.c` đọc `extern ZigbeeMesh_t zigbee` trực tiếp. Phải decoupled trong V3.

---

## Modbus Migration Plan

### Reusable Components

Các thành phần sau đây **portable hoàn toàn**, không phụ thuộc hardware, có thể copy nguyên từ V2 sang V3:

| Component | Source V2 | Lý do reusable |
|---|---|---|
| FreeModbus stack core | `SynaptiX/services/Modbus/modbus/mb.c` | Pure C logic, không phụ thuộc hardware |
| FreeModbus RTU framing | `SynaptiX/services/Modbus/modbus/rtu/mbrtu.c` + `mbcrc.c` | Pure C algorithm |
| FreeModbus ASCII framing | `SynaptiX/services/Modbus/modbus/ascii/mbascii.c` | Pure C algorithm |
| FreeModbus function handlers | `SynaptiX/services/Modbus/modbus/functions/*.c` (7 files) | Pure C logic |
| FreeModbus headers | `SynaptiX/services/Modbus/modbus/include/*.h` (7 files) | Portable |
| Register callbacks | `SynaptiX/services/Modbus/port/user_mb_app.c` + `.h` | Pure C, register buffer logic |
| Event queue (portevent.c) | `SynaptiX/services/mbport/portevent.c` | Pure logic, flag-based, no HAL |
| Port type definitions | `SynaptiX/services/mbport/port.h` | Typedefs + macros, portable |
| CQueue data structure | `SynaptiX/utils/cqueue.h` + `cqueue.c` | Pure C data structure |

### Components Requiring Adaptation

| Component | Source V2 | Thay đổi cần thiết |
|---|---|---|
| `portserial.c` | `SynaptiX/services/mbport/portserial.c` | Rebind `bsp_com_*` và `bsp_de_*` calls sang BSP V3. Cập nhật `BSP_RS485_COM_PORT` constant nếu index khác |
| `porttimer.c` | `SynaptiX/services/mbport/porttimer.c` | Rebind `bsp_timer0_start/stop` sang timer hardware của V3. TIM instance thay đổi |
| `mbconfig.h` | `SynaptiX/services/Modbus/modbus/include/mbconfig.h` | Giữ nguyên feature flags. Cập nhật `N_MODBUS = 1` (V3 chỉ cần 1 instance RS485, không có RF Modbus) nếu xác nhận không cần RF Modbus |
| `user_mb_app.c` | `SynaptiX/apps/user_mb_app.c` | Decoupled `extern ZigbeeMesh_t zigbee` — thay bằng getter function. GPIO sync (IN/OUT ↔ Modbus registers) adapt sang V3 API |

### Components Requiring Rewrite

| Component | Lý do |
|---|---|
| BSP layer (`board.c` equivalent) | Không tồn tại trong V3. Phải tạo `libs/modbus/bsp/modbus_bsp.c` hoặc `libs/bsp/` chung cho tất cả services |
| Timer configuration (TIM2) | V3 không có TIM2 init. Phải: (1) thêm TIM2 vào `.ioc`, (2) regenerate CubeMX, (3) implement `HAL_TIM_PeriodElapsedCallback`. Với PCLK V3 = 32MHz, period cho 3.5T@115200 ≈ 0.3ms → prescaler phải cho resolution ≤ 100µs |
| `bsp_com_init()` | Bug `if(i = BSP_RF_COM_PORT)` phải fix. V3 không dùng GPDMA cho Modbus — USART1 RX vẫn dùng IT mode 1-byte |
| Modbus task (`modbus_task`) | V2 không có dedicated Modbus task — tất cả trong mainApp. V3 cần dedicated `modbusTask` theo KB_v3.md KB-5 |
| DE toggle naming | Đổi `bsp_uart2_de_on/off` → `bsp_rs485_de_on/off` |

### Target Location In V3

```
libs/
└── modbus/
    ├── CMakeLists.txt          (mới — đăng ký tất cả sources)
    ├── modbus_service.h        (mới — public API theo KB_v3 KB-5)
    ├── modbus_service.c        (mới — task wrapper, init, public API)
    ├── bsp/
    │   ├── modbus_bsp.h        (mới — BSP interface: COM, DE, timer)
    │   └── modbus_bsp.c        (mới — implement với HAL V3)
    ├── mbport/
    │   ├── port.h              (copy V2, cập nhật N_MODBUS)
    │   ├── portserial.c        (copy V2, adapt BSP calls)
    │   ├── porttimer.c         (copy V2, adapt timer)
    │   └── portevent.c         (copy V2 nguyên vẹn)
    ├── stack/                  (copy nguyên từ V2 Modbus/modbus/)
    │   ├── mb.c
    │   ├── ascii/mbascii.c
    │   ├── rtu/mbrtu.c + mbcrc.c
    │   ├── functions/ (7 files)
    │   └── include/ (7 headers)
    └── app/
        ├── user_mb_app.h       (copy V2, update register counts)
        └── user_mb_app.c       (copy V2, decoupled Zigbee reference)
```

### Required Interfaces

Modbus service cần các interfaces sau từ V3:

- `UART_HandleTypeDef huart1` — extern từ `Core/Src/usart.c`
- `HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, ...)` — DE pin control (GPIO đã init trong V3 `gpio.c`)
- HAL Timer handle (TIM2 hoặc timer khác) — phải thêm vào V3
- `HAL_UART_Receive_IT()` + `HAL_UART_RxCpltCallback` override
- `HAL_UART_Transmit_IT()` + `HAL_UART_TxCpltCallback` override
- `HAL_TIM_Base_Start_IT()` / `HAL_TIM_Base_Stop_IT()`
- `HAL_TIM_PeriodElapsedCallback` override

### Required RTOS Integration

- **Task mới:** `modbusTask` — FreeRTOS task, chạy `eMBPoll()` loop
- **Không cần Queue/Semaphore** nếu giữ polling model như V2
- **Optional:** FreeRTOS Queue để Modbus giao tiếp với DIO task và Mongoose REST handler
- Task stack: tối thiểu 512 words (1KB), đề xuất 1024 words (4KB) để an toàn

### Required Peripheral Integration

- **USART1:** Đã init trong V3 (`MX_USART1_UART_Init`, 115200 baud). Modbus cần override RX/TX callbacks.
- **TIM2 hoặc TIM phù hợp:** Chưa có trong V3. Phải thêm vào `.ioc` và init. Với PCLK = 16MHz (APB1 = HCLK/1 = HSI/2/1 = 32MHz → PCLK2 cho USART1 = 32MHz, APB1 TIM = 32MHz), timer period ≤ 100µs → prescaler ví dụ 31 (1µs resolution), period = 100 (100µs tick).
- **GPIO PB13:** Đã init là Output PP trong `gpio.c`. Dùng trực tiếp.

### Risks

| Risk | Severity | Mô tả |
|---|---|---|
| Clock 32MHz → Timer period phải recalculate | HIGH | V2 dùng 250MHz với prescaler 249 → 1MHz → period 999 → 1ms (vốn đã sai). V3 phải cấu hình đúng ≤ 100µs |
| TIM2 không có trong V3 `.ioc` | HIGH | Phải mở CubeMX, thêm TIM2, regenerate. Core/ files sẽ thay đổi — cần verify không break web/USB stack |
| `HAL_UART_RxCpltCallback` override conflict | HIGH | V3 có thể đã dùng UART callbacks cho LPUART1 (Shell) và USART2 (Zigbee). Callback là weak, chỉ 1 implementation được phép. Phải mux trong shared callback |
| `HAL_TIM_PeriodElapsedCallback` conflict | MEDIUM | Nếu Mongoose hoặc lwIP cũng dùng timer callback, phải mux |
| Memory budget | MEDIUM | V3 có lwIP (~30-50KB heap), Mongoose (~10-20KB stack), TinyUSB, FreeRTOS tasks. Còn lại cho Modbus buffers |
| ISR gọi FreeModbus stack | HIGH | Như V2 — `mb_uart_tx_task` gọi từ TxCpltCallback (ISR context). Cần quyết định giữ nguyên hay fix |

### Open Questions

1. V3 sẽ dùng TIM2 cho Modbus 3.5T hay timer khác? Có TIM nào đã free không?
2. `N_MODBUS = 1` hay `= 2` (có cần RF Modbus như V2 không)?
3. Modbus slave address: hardcode hay đọc từ Flash config?
4. Register map V3 có giữ nguyên V2 (100 Holding + 100 Input + 64 Coil + 64 Discrete) không?
5. DIO task và Modbus task chia sẻ register buffers qua mutex hay qua shared memory?
6. Modbus status có expose qua REST API `GET /api/modbus` không?

---

## Zigbee Migration Plan

### Reusable Components

| Component | Source V2 | Lý do reusable |
|---|---|---|
| `ZigbeeMesh_t` struct | `zigbee.h` | Pure C struct, không phụ thuộc HAL |
| `ZigbeeParameter_t`, `ZigbeeEvent_t`, `ZigbeeDriver_t` | `zigbee.h` | Portable struct definitions |
| `ZigbeeEventType_t` enum | `zigbee.h` | Portable |
| Tất cả `#define` constants (INS codes, role types, response codes) | `zigbee.h` | Portable |
| `zigbee_init()` | `zigbee.c` | Pure C init logic |
| `zigbee_poll()` | `zigbee.c` | State machine logic — chỉ cần `ZigbeeDriver_t` đúng |
| Tất cả public API (`connect`, `reset`, `read`, `write`, `request_signal`, `query_ed_pos`) | `zigbee.c` | Dùng driver abstraction, portable |
| Command frame arrays (`linkIns`, `readIns`, etc.) | `zigbee.c` | Static data, portable |
| `zigbeeGetWriteIns()` | `zigbee.c` | Pure logic |
| `zigbeeGetXY()` checksum | `zigbee.c` | Pure algorithm |
| Tất cả response process functions | `zigbee.c` | Pure parsing logic |
| `CQueue_t` | `utils/cqueue.h` + `cqueue.c` | Đã có trong V3 sau khi port |

### Components Requiring Adaptation

| Component | Source V2 | Thay đổi cần thiết |
|---|---|---|
| `ZigbeeDriver_t` binding | `rf_app.c` | Bind lại với UART API của V3. V3 không có `bsp_com_write(BSP_RF_COM_PORT)` — phải tạo equivalent |
| Callbacks (`zigbee_connect_cb`, `zigbee_read_cb`, etc.) | `rf_app.c` | Adapt sang indicator/LED API của V3 (GPIO PC13-PC15). Xóa V2-specific `indicator.c` dependency |
| `rf_app_init()` init sequence | `rf_app.c` | Adapt sang V3 boot sequence trong `StartDefaultTask` hoặc task init |
| `rf_app_poll()` timing | `rf_app.c` | Adapt timestamp source: dùng `HAL_GetTick()` của V3 (`sys_now()` đã có trong `app_freertos.c`) |
| Logger calls trong `zigbee.c` | `zigbee.c` | V3 có `libs/logger/logger.h` riêng — rebind |

### Components Requiring Rewrite

| Component | Lý do |
|---|---|
| `HAL_UARTEx_RxEventCallback` với Parency_Transfer logic | Phải decoupled khỏi `extern ZigbeeMesh_t zigbee`. V3 dùng volatile flag thay vì direct struct access |
| `bsp_com_write()` cho RF port | V3 cần implement tương đương. Phải quyết định blocking (như V2) hay non-blocking (DMA TX). Nếu DMA: cần cấu hình GPDMA trong V3 `.ioc` |
| GPDMA / USART2 RX mode | V3 không có GPDMA init. Phải chọn: (a) thêm GPDMA, regenerate CubeMX; hoặc (b) dùng `HAL_UARTEx_ReceiveToIdle_IT` không DMA |
| `app_config.h` | Tạo lại cho V3 build system: `#define ZIGBEE_ENABLE 1` trong CMakeLists.txt target compile definition |
| Parency_Transfer bridge logic | Nếu V3 giữ tính năng bridge RS485↔Zigbee, phải thiết kế lại không dùng extern coupling |

### Target Location In V3

```
libs/
└── zigbee/
    ├── CMakeLists.txt          (mới)
    ├── zigbee.h                (copy V2 nguyên vẹn)
    ├── zigbee.c                (copy V2, swap logger calls)
    ├── zigbee_service.h        (mới — public API cho V3: init, task entry)
    ├── zigbee_service.c        (mới — ZigbeeDriver_t binding + task wrapper)
    └── zigbee_bsp.c / .h       (mới — USART2 I/O: write/read/available functions)
```

### Required Interfaces

- `UART_HandleTypeDef huart2` — extern từ `Core/Src/usart.c`
- `HAL_UART_Transmit()` hoặc `HAL_UART_Transmit_IT()` (blocking vs non-blocking — cần quyết định)
- `HAL_UARTEx_ReceiveToIdle_IT()` hoặc `HAL_UARTEx_ReceiveToIdle_DMA()` + callback override
- `HAL_UARTEx_RxEventCallback` override (cần mux với USART1 nếu dùng cùng callback)
- `sys_now()` hoặc `HAL_GetTick()` cho timestamp — đã có trong V3 `app_freertos.c`

### Required RTOS Integration

- **Task mới:** `wirelessTask` theo KB_v3.md — FreeRTOS task chạy `rf_app_poll()` loop
- **Tick source:** `vTaskDelay(1)` để đảm bảo zigbee_poll được gọi mỗi ~1ms
- **Không cần Queue** nếu giữ nguyên polling model
- **Optional:** FreeRTOS Queue/Semaphore để Zigbee status expose cho Mongoose REST API
- Task stack: tối thiểu 512 words, đề xuất 1024 words (buffer `zigbee->buff[1024]` chiếm nhiều)

### Required Peripheral Integration

- **USART2:** Đã init trong V3 (`MX_USART2_UART_Init`). Cần cấu hình baud rate phù hợp Zigbee module.
- **GPDMA (tùy chọn):** Chưa có trong V3. Nếu giữ DMA RX: phải thêm vào `.ioc` và init.
- **GPIO PC13-PC15:** Đã init là Output — dùng cho indicator callbacks.

### Risks

| Risk | Severity | Mô tả |
|---|---|---|
| USART2 baud rate chưa xác định | HIGH | V2 load từ Flash `app_setting.rf.baudrate`. V3 chưa có Flash config. Phải hardcode hoặc implement config storage |
| GPDMA không có trong V3 | HIGH | Nếu dùng DMA RX: phải thêm CubeMX config. Nếu dùng IT: throughput thấp hơn nhưng đơn giản hơn |
| `HAL_UARTEx_RxEventCallback` conflict | HIGH | Zigbee và Modbus đều cần override UART callbacks. Phải mux chung |
| board.c–zigbee.h circular coupling | HIGH | Phải hoàn toàn loại bỏ khi port — không được để BSP biết về zigbee struct |
| Blocking TX trong task | MEDIUM | `HAL_UART_Transmit(HAL_MAX_DELAY)` block `wirelessTask`. Nếu module không respond, task treo |
| Memory: `zigbee->buff[1024]` | MEDIUM | 1KB buffer là tĩnh trong `ZigbeeMesh_t`. Với 32MHz + lwIP heap pressure, cần verify còn đủ RAM |

### Open Questions

1. GPDMA cho USART2: giữ DMA hay chuyển sang IT mode?
2. Zigbee baud rate: hardcode bao nhiêu? Hay implement Flash config ngay từ đầu?
3. Parency_Transfer mode: V3 có cần tính năng RS485↔Zigbee bridge không?
4. Zigbee params expose qua REST API `GET /api/wireless` hay qua Modbus Input Registers như V2?
5. `zigbee_query_ed_pos()` không validate response — có fix trong V3 không?

---

## Shell Migration Plan

### Reusable Components

| Component | Source V2 | Lý do reusable |
|---|---|---|
| Shell engine core | `services/shell/cli_shell.c` + `.h` | Portable 100%. Driver injection pattern (`sCliShellImpl_t`). Không phụ thuộc HAL hay FreeRTOS |
| Command struct `Cli_Shell_Cmd_t` | `cli_shell.h` | Pure C struct |
| Tokenizer `prv_process()` | `cli_shell.c` | Pure C string logic |
| Command dispatch `prv_find_command()` | `cli_shell.c` | Pure C, portable |
| Output primitives `cli_shell_put_line`, `cli_shell_printf` | `cli_shell.c` | Portable — dùng `sCliShellImpl_t.send_str` |
| Command table registration pattern | `cli_shell_command.c` | `g_shell_commands` + extern pattern — portable |

### Components Requiring Adaptation

| Component | Source V2 | Thay đổi cần thiết |
|---|---|---|
| `sCliShellImpl_t` binding | `shell_app.c` | Bind `send_char`/`send_str` vào: (a) LPUART1 output (KB_v3: Shell là LPUART1), (b) USB CDC ACM (TinyUSB API), hoặc (c) cả hai |
| `shell_receive_task` | `shell_app.c` | Đổi input source: LPUART1 RX (KB_v3 primary) + USB CDC ACM (TinyUSB) nếu cần dual input |
| Logger output binding | `board.c` `bsp_init()` | V3 logger (`libs/logger/`) đã tồn tại — rebind output sang `PUTCHAR_PROTOTYPE` → `HAL_UART_Transmit(&hlpuart1)` đã có trong `app_freertos.c` |
| Command handlers GPIO | `cli_shell_command.c` | `cli_set_output`/`cli_get_input` adapt sang GPIO V3 (cùng pin, label khác: OUT0→DO1, IN0→DI1) |
| `config` command | `cli_shell_command.c` | Slave address, Flash operations adapt cho V3. Flash sector có thể khác |
| Commands liên quan Modbus | `cli_shell_command.c` | Adapt sang API của `modbus_service.h` |
| Commands liên quan Zigbee | `cli_shell_command.c` | Adapt sang API của `zigbee_service.h` |
| `app_setting_save/load` | `app_settings.c` | Flash layout của V3 khác V2. Phải map lại sector/address |

### Components Requiring Rewrite

| Component | Lý do |
|---|---|
| `shell_send_str()` TX với `vTaskDelay(10)` | Quá chậm. V3 nên dùng `HAL_UART_Transmit_IT` + notification, hoặc đơn giản hơn: blocking `HAL_UART_Transmit` với timeout ngắn (không phải `vTaskDelay`) |
| Logger mutex | V2 `FREE_RTOS=0` → mutex disabled. V3 cần enable `FREE_RTOS=1` và cung cấp real mutex hoặc route qua task-safe queue |
| `cli_shell_printf` buffer | Đổi `vsprintf` → `vsnprintf(cli_buff, sizeof(cli_buff), ...)` để tránh buffer overflow |
| USB CDC ACM binding cho Shell | V2 dùng AL94 USB API (`bsp_usb_transmit/receiver`). V3 dùng TinyUSB — API hoàn toàn khác. Nếu Shell dùng USB: phải implement TinyUSB CDC ACM read/write wrapper |
| Shell input source | KB_v3 ghi Shell trên LPUART1. V3 cũng có USB CDC ACM qua TinyUSB. Cần quyết định: LPUART1 only, USB only, hay dual |
| `app_settings.c` Flash layer | Hoàn toàn rewrite vì Flash sector map V3 khác V2 |

### Target Location In V3

```
libs/
└── shell/
    ├── CMakeLists.txt          (mới)
    ├── cli_shell.h             (copy V2 nguyên vẹn)
    ├── cli_shell.c             (copy V2, fix vsprintf → vsnprintf)
    ├── shell_service.h         (mới — public API: init, task entry)
    ├── shell_service.c         (mới — I/O binding, FreeRTOS task)
    └── shell_commands.c        (adapt V2 cli_shell_command.c cho V3 APIs)
```

### Required Interfaces

- `UART_HandleTypeDef hlpuart1` — extern từ `Core/Src/usart.c` (LPUART1 Shell UART theo KB_v3 KB-2)
- `HAL_UART_Transmit()` cho Shell output qua LPUART1
- `HAL_UART_Receive_IT()` hoặc polling LPUART1 RX
- TinyUSB CDC ACM API (nếu Shell cũng dùng USB input) — từ `Middlewares/Third_Party/tinyusb`
- Logger interface: `libs/logger/logger.h` đã có trong V3
- GPIO API (`HAL_GPIO_WritePin`, `HAL_GPIO_ReadPin`) cho output/input commands

### Required RTOS Integration

- **Task mới:** `shellTask` — FreeRTOS task, priority cao (KB_v3 KB-6: `shellTask (LPUART1 + CDC ACM)`)
- **Không cần Queue** nếu Shell là standalone CLI
- **Optional:** Callback-based notification khi Zigbee/Modbus commands complete
- Task stack: 512 words (2KB) — shell response buffer 1KB cần stack đủ lớn

### Required Peripheral Integration

- **LPUART1:** Đã init trong V3 (`MX_LPUART1_UART_Init`, 115200 baud). Shell cần đọc RX từ đây.
- **USB CDC ACM (TinyUSB):** Đã có trong V3. Cần implement read/write wrapper nếu Shell dùng USB.
- **GPIO:** Đã init — dùng cho `output`/`input` shell commands.

### Risks

| Risk | Severity | Mô tả |
|---|---|---|
| LPUART1 RX chưa được arm | HIGH | V3 `MX_LPUART1_UART_Init()` chỉ init UART, không arm RX interrupt. Shell task phải arm `HAL_UART_Receive_IT` |
| TinyUSB CDC ACM API khác AL94 | HIGH | Nếu Shell cần USB input/output: phải học TinyUSB CDC API mới (không phải `CDC_Transmit` của STM32 middleware) |
| Shell output từ multiple tasks | HIGH | V2 có race condition tương tự — Zigbee callbacks gọi `cli_shell_put_line` từ task khác. V3 cần mutex bảo vệ shell TX |
| `cli_shell_printf` static buffer | MEDIUM | `static char cli_buff[1024]` không reentrant. Với multi-task V3, cần mutex hoặc stack-allocated buffer |
| Flash config location | MEDIUM | Shell `config` command save/load setting vào Flash. V3 Flash layout chưa được xác định |

### Open Questions

1. Shell primary I/O: LPUART1 only (KB_v3 mô tả) hay cũng có USB CDC ACM?
2. Shell logger: dùng `PUTCHAR_PROTOTYPE` → LPUART1 đã có, hay cần separate logger init?
3. `app_settings` Flash sector: V3 dùng sector nào cho persistent config?
4. Shell commands cho Modbus và Zigbee: dùng direct function call hay message passing?
5. Echo và prompt: có enable không? KB_v3 KB-9 list shell commands nhưng không mô tả echo behavior.

---

## Dependency Changes

### Dependency bị loại bỏ (V2 → V3)

| Dependency V2 | Lý do loại bỏ trong V3 |
|---|---|
| `AL94_USB_Composite` (Middlewares/Third_Party/AL94_USB_Composite/) | Thay bằng TinyUSB |
| `SynaptiX/board/board.c` + `board.h` | Không tồn tại trong V3 — phải tạo BSP layer mới |
| `SynaptiX/apps/` (toàn bộ) | V3 dùng `libs/` structure, không có `SynaptiX/` |
| TIM2, TIM3 (Modbus timer) | Chưa có trong V3 — phải thêm |
| GPDMA (USART2 Zigbee RX) | Chưa config trong V3 — phải quyết định giữ hay bỏ |
| SPI4 + `lora.c` | SPI disabled trong V3; LoRa không trong scope port |
| IWDG | Disabled trong V3 |
| GCC Makefile build system | Thay bằng CMake |
| `app_settings.c` Flash config (V2 sector layout) | Phải rewrite cho V3 Flash layout |
| `synaptix.mk` | V3 dùng CMakeLists.txt |

### Dependency mới trong V3

| Dependency mới | Vai trò |
|---|---|
| TinyUSB (Middlewares/Third_Party/tinyusb) | USB device stack (CDC ACM + ECM + RNDIS) |
| lwIP (libs/stm32-mw-lwip/) | TCP/IP networking qua USB ECM/RNDIS |
| Mongoose (mongoose/) | HTTP server + REST API |
| `libs/logger/` | Logger library đã có sẵn trong V3 |
| CMake 3.22+ / Ninja | Build system |

### HAL dependency mới

| HAL Module | Lý do cần |
|---|---|
| HAL_TIM (đã enabled) | Cần TIM2 (hoặc timer khác) cho Modbus 3.5T timer |
| HAL_DMA (đã enabled) | Nếu giữ DMA cho USART2 Zigbee |

### RTOS dependency mới

| RTOS Object | Service | Lý do |
|---|---|---|
| `modbusTask` FreeRTOS task | Modbus | Dedicated polling task |
| `shellTask` FreeRTOS task | Shell | CLI I/O task |
| `wirelessTask` FreeRTOS task | Zigbee | RF polling task |
| Optional: `osMutex` cho shell TX | Shell | Thread-safe output |
| Optional: `osQueue` cho inter-service | Modbus↔DIO↔REST | Data sharing |

---

## V3 Integration Map

### Modbus

**File cần tạo:**
- `libs/modbus/CMakeLists.txt`
- `libs/modbus/modbus_service.h` + `modbus_service.c`
- `libs/modbus/bsp/modbus_bsp.h` + `modbus_bsp.c`
- `libs/modbus/mbport/portserial.c` (adapt từ V2)
- `libs/modbus/mbport/porttimer.c` (adapt từ V2)
- `libs/modbus/mbport/portevent.c` (copy từ V2)
- `libs/modbus/mbport/port.h` (copy từ V2, update N_MODBUS)
- `libs/modbus/stack/` (copy toàn bộ từ V2 `Modbus/modbus/`)
- `libs/modbus/app/user_mb_app.h` + `user_mb_app.c` (adapt từ V2)

**File cần sửa:**
- `libs/CMakeLists.txt` → thêm `add_subdirectory(modbus)`
- `Core/Src/app_freertos.c` → spawn `modbusTask` trong `StartDefaultTask`
- `Core/Src/stm32h5xx_it.c` → thêm TIM2_IRQHandler (sau khi thêm TIM2 vào `.ioc`)
- `RF_IO_V3.ioc` → thêm TIM2 config
- `Core/Src/` → regenerate sau khi sửa `.ioc` (thêm `tim.c`/`tim.h` nếu chưa có)

**File cần giữ nguyên:**
- Toàn bộ `Core/` (chỉ sửa trong USER CODE zones)
- `Drivers/`
- `Middlewares/`
- `mongoose/`, `usb_net/`

### Zigbee

**File cần tạo:**
- `libs/zigbee/CMakeLists.txt`
- `libs/zigbee/zigbee.h` + `zigbee.c` (copy từ V2)
- `libs/zigbee/zigbee_service.h` + `zigbee_service.c`
- `libs/zigbee/zigbee_bsp.h` + `zigbee_bsp.c` (USART2 I/O wrapper)

**File cần sửa:**
- `libs/CMakeLists.txt` → thêm `add_subdirectory(zigbee)`
- `Core/Src/app_freertos.c` → spawn `wirelessTask`
- `RF_IO_V3.ioc` → thêm GPDMA config nếu cần DMA RX (tùy quyết định)

**File cần giữ nguyên:**
- `Core/Src/usart.c` (USART2 đã init)
- Toàn bộ networking stack

### Shell

**File cần tạo:**
- `libs/shell/CMakeLists.txt`
- `libs/shell/cli_shell.h` + `cli_shell.c` (copy từ V2, fix vsprintf)
- `libs/shell/shell_service.h` + `shell_service.c` (I/O binding + task)
- `libs/shell/shell_commands.c` (adapt từ V2 `cli_shell_command.c`)

**File cần sửa:**
- `libs/CMakeLists.txt` → thêm `add_subdirectory(shell)`
- `Core/Src/app_freertos.c` → spawn `shellTask`

**File cần giữ nguyên:**
- `libs/logger/` (đã có, không sửa)
- `Core/Src/usart.c` (LPUART1 đã init)

---

## FreeRTOS Task Architecture

### Task hiện tại trong V3 (trước khi port)

| Task | Priority | Stack | Chức năng |
|---|---|---|---|
| `defaultTask` (StartDefaultTask) | osPriorityNormal | 256×4 = 1KB | Init lwIP, USB netif, spawn web_server_task |
| `web_server_task` (Webserver) | 10 | 4096×2×4 = 32KB | Mongoose poll loop |

### Task cần thêm sau khi port

| Task | Priority đề xuất | Stack đề xuất | Chức năng |
|---|---|---|---|
| `modbusTask` | osPriorityNormal (7) | 1024×4 = 4KB | eMBPoll() loop, Modbus RTU slave |
| `shellTask` | osPriorityAboveNormal (8) | 512×4 = 2KB | CLI I/O, command dispatch |
| `wirelessTask` | osPriorityNormal (7) | 1024×4 = 4KB | zigbee_poll() loop, RF management |
| `dioTask` | osPriorityNormal (7) | 256×4 = 1KB | GPIO poll, DI/DO sync với Modbus registers |

### Queue / Semaphore / Event Flags (đề xuất — cần confirm)

| Object | Type | Producers | Consumers | Mục đích |
|---|---|---|---|---|
| Shell TX mutex | `osMutex` | shellTask, wirelessTask callbacks | shellTask | Bảo vệ Shell output |
| Modbus register mutex | `osMutex` | modbusTask (FC write), dioTask (GPIO sync) | mongooseTask (REST read) | Bảo vệ register buffers |
| DIO state queue (optional) | `osQueue` | dioTask | mongooseTask | Push DIO state thay đổi |
| Zigbee status (optional) | `osQueue` hoặc shared struct | wirelessTask | mongooseTask, shellTask | Expose Zigbee state |

**Lưu ý:** Nếu giữ nguyên polling model của V2 (không FreeRTOS IPC), có thể bắt đầu không có Queue/Semaphore và thêm dần khi cần. Rủi ro: không thread-safe giữa Modbus task và Mongoose task khi cùng đọc/ghi register buffer.

**Thông tin còn thiếu:** Stack size chính xác cho từng task phụ thuộc vào memory budget V3 sau khi tính lwIP + Mongoose + TinyUSB. Cần đo sau khi có initial build.

---

## Module Layout Proposal

Cấu trúc `libs/` cuối cùng sau khi port đầy đủ:

```
libs/
├── CMakeLists.txt              (sửa: thêm subdirectory modbus, zigbee, shell, bsp, utils)
│
├── logger/                     (GIỮ NGUYÊN — đã có)
│   ├── CMakeLists.txt
│   ├── logger.h
│   └── logger.c
│
├── stm32-mw-lwip/              (GIỮ NGUYÊN — networking)
├── tinyusb/                    (GIỮ NGUYÊN — USB stack)
│
├── utils/                      (MỚI — shared utilities)
│   ├── CMakeLists.txt
│   ├── cqueue.h
│   └── cqueue.c                (copy từ V2 SynaptiX/utils/)
│
├── bsp/                        (MỚI — Board Support Package cho V3)
│   ├── CMakeLists.txt
│   ├── bsp.h                   (common BSP: COM ports, GPIO, timer abstractions)
│   ├── bsp_uart.c / .h         (UART CQueue, ISR callbacks, COM abstraction)
│   └── bsp_gpio.c / .h         (DI/DO abstraction)
│
├── modbus/                     (MỚI — Modbus RTU Slave service)
│   ├── CMakeLists.txt
│   ├── modbus_service.h        (public API: init, task)
│   ├── modbus_service.c        (FreeRTOS task, init wrapper)
│   ├── mbport/
│   │   ├── port.h
│   │   ├── portserial.c
│   │   ├── porttimer.c
│   │   └── portevent.c
│   ├── stack/                  (FreeModbus core — copy từ V2)
│   │   ├── mb.c
│   │   ├── ascii/
│   │   ├── rtu/
│   │   ├── functions/
│   │   └── include/
│   └── app/
│       ├── user_mb_app.h
│       └── user_mb_app.c
│
├── zigbee/                     (MỚI — Zigbee RF service)
│   ├── CMakeLists.txt
│   ├── zigbee.h                (copy từ V2)
│   ├── zigbee.c                (copy từ V2, swap logger)
│   ├── zigbee_service.h        (public API: init, task)
│   └── zigbee_service.c        (ZigbeeDriver_t binding, FreeRTOS task)
│
└── shell/                      (MỚI — CLI Shell service)
    ├── CMakeLists.txt
    ├── cli_shell.h             (copy từ V2)
    ├── cli_shell.c             (copy từ V2, fix vsprintf)
    ├── shell_service.h         (public API: init, task)
    ├── shell_service.c         (I/O binding, FreeRTOS task)
    └── shell_commands.c        (adapt từ V2, commands cho V3)
```

---

## Implementation Backlog

### Phase 1 - Modbus

**Files cần tạo:**
- `libs/utils/cqueue.h` + `cqueue.c`
- `libs/bsp/bsp_uart.h` + `bsp_uart.c`
- `libs/bsp/bsp_gpio.h` + `bsp_gpio.c`
- `libs/modbus/CMakeLists.txt`
- `libs/modbus/modbus_service.h` + `modbus_service.c`
- `libs/modbus/mbport/port.h` (copy + update)
- `libs/modbus/mbport/portserial.c` (adapt)
- `libs/modbus/mbport/porttimer.c` (adapt)
- `libs/modbus/mbport/portevent.c` (copy)
- `libs/modbus/stack/` (copy FreeModbus core)
- `libs/modbus/app/user_mb_app.h` + `user_mb_app.c` (adapt)

**Files cần sửa:**
- `libs/CMakeLists.txt` → thêm `modbus`, `bsp`, `utils`
- `RF_IO_V3.ioc` → thêm TIM2, xác nhận USART1 config
- `Core/Src/app_freertos.c` → spawn `modbusTask`

**APIs cần implement:**
- `modbus_service_init(UART_HandleTypeDef*, GPIO_TypeDef*, uint16_t de_pin)`
- `modbus_task(void *arg)` — FreeRTOS task entry
- `bsp_uart_init()` — CQueue init, arm UART IT
- `bsp_rs485_de_on()` / `bsp_rs485_de_off()` — PB13 GPIO toggle
- `bsp_timer_start()` / `bsp_timer_stop()` — TIM2 control
- Override `HAL_UART_RxCpltCallback` (mux LPUART1 + USART1 + USART2)
- Override `HAL_UART_TxCpltCallback` (mux)
- Override `HAL_TIM_PeriodElapsedCallback` (mux TIM2 + TIM6 base + Mongoose timer nếu có)

**Acceptance Criteria:**
- Modbus slave đáp ứng FC03 (Read Holding Registers) đúng từ PC master tool (ví dụ QModMaster)
- Modbus slave đáp ứng FC06 (Write Single Register) — giá trị được ghi vào register buffer
- FC01 (Read Coils) và FC05 (Write Single Coil) hoạt động
- RS485 DE pin HIGH trước byte đầu tiên TX, LOW sau khi TX complete
- Timer 3.5T hoạt động đúng: frame được detect sau inter-frame gap
- Không crash sau 24h chạy liên tục với traffic Modbus
- `modbusTask` không ảnh hưởng đến Mongoose web server (web vẫn serve HTTP)

---

### Phase 2 - Shell

**Files cần tạo:**
- `libs/shell/CMakeLists.txt`
- `libs/shell/cli_shell.h` + `cli_shell.c` (copy + fix)
- `libs/shell/shell_service.h` + `shell_service.c`
- `libs/shell/shell_commands.c`

**Files cần sửa:**
- `libs/CMakeLists.txt` → thêm `shell`
- `Core/Src/app_freertos.c` → spawn `shellTask`
- `Core/Src/stm32h5xx_it.c` → xác nhận LPUART1 IRQ routing

**APIs cần implement:**
- `shell_service_init(UART_HandleTypeDef*)` — bind LPUART1
- `shell_task(void *arg)` — FreeRTOS task entry
- `shell_send_str(str)` → `HAL_UART_Transmit(&hlpuart1, ...)`
- `shell_receive_char()` → từ LPUART1 RX IT callback
- Tất cả command handlers: `help`, `restart`, `modbus`, `zigbee`, `output`, `input`, `network`, `reboot` (theo KB_v3 KB-9)
- Logger rebind: `logger_init(level, output_fn)` với `output_fn` → LPUART1

**Acceptance Criteria:**
- Kết nối terminal (115200 baud) vào LPUART1 PA9/PA10 → gõ `help` → hiện danh sách commands
- `system info` hiện version, MCU info
- `di read` hiện trạng thái DI1-4
- `do set 1 1` điều khiển DO1 HIGH
- `modbus status` hiện trạng thái Modbus service
- `reboot` reset device sau 2s countdown
- Shell không block Modbus hoặc Mongoose khi đang process command
- `vsprintf` overflow không xảy ra (dùng `vsnprintf` với size guard)

---

### Phase 3 - Zigbee

**Files cần tạo:**
- `libs/zigbee/CMakeLists.txt`
- `libs/zigbee/zigbee.h` + `zigbee.c` (copy từ V2)
- `libs/zigbee/zigbee_service.h` + `zigbee_service.c`
- `libs/zigbee/zigbee_bsp.h` + `zigbee_bsp.c`

**Files cần sửa:**
- `libs/CMakeLists.txt` → thêm `zigbee`
- `Core/Src/app_freertos.c` → spawn `wirelessTask`
- `Core/Src/stm32h5xx_it.c` → USART2 IRQ (xác nhận routing)
- `RF_IO_V3.ioc` → GPDMA config (nếu cần DMA RX)
- `libs/modbus/app/user_mb_app.c` → replace `extern ZigbeeMesh_t zigbee` bằng getter function từ `zigbee_service.h`

**APIs cần implement:**
- `zigbee_service_init()` — setup ZigbeeDriver_t, bind USART2
- `wireless_task(void *arg)` — FreeRTOS task entry, gọi `rf_app_poll()`
- `zigbee_bsp_write(uint8_t*, uint32_t)` → USART2 TX
- `zigbee_bsp_read(uint8_t*, uint32_t)` → đọc từ CQueue USART2
- `zigbee_bsp_available()` → count bytes trong USART2 RX queue
- `zigbee_get_params()` — getter function cho Modbus `user_mb_app.c` (decoupled)
- Shell commands: `zigbee status`, `zigbee -c`, `zigbee -r`, `zigbee -p` (adapt từ V2)

**Acceptance Criteria:**
- Kết nối Zigbee module vào USART2 → `zigbee status` hiện params đọc được từ module
- `zigbee -c` (connect) → response success trong 300ms
- `zigbee -r` (read) → params hiện đúng trong shell output
- Modbus FC04 Input Registers addr 21-40 hiện Zigbee params (nếu giữ V2 mapping)
- Không crash khi Zigbee module không connected (timeout → `ZIGBEE_RES_TIMEOUT`)
- `wirelessTask` không ảnh hưởng đến Modbus hoặc Mongoose
- `board.c`–`zigbee.h` circular coupling đã được loại bỏ hoàn toàn

---

## Missing Information

Các thông tin còn thiếu và file cần đọc thêm trước khi code:

| # | File cần đọc | Lý do cần đọc | Kiến thức còn thiếu |
|---|---|---|---|
| M1 | `RF_IO_V3.ioc` (xem bằng STM32CubeMX) | Xác nhận timer nào còn free cho Modbus 3.5T | TIM2 có được cấu hình không? Có timer nào khác free? |
| M2 | `RF_IO_V3.ioc` | Xác nhận có GPDMA không, channels nào | V3 có GPDMA init không? USART2 DMA có được cấu hình? |
| M3 | `Core/Src/stm32h5xx_it.c` V3 đầy đủ | Xem tất cả IRQ handlers đã có | Handler nào đã tồn tại, có conflict với Modbus/Zigbee/Shell không |
| M4 | `usb/Target/usb_descriptors.c` | TinyUSB USB descriptor — CDC ACM interface index | Shell bind vào CDC ACM interface index bao nhiêu? PID/VID? |
| M5 | `mongoose/mongoose_glue.c` | Xem Mongoose task có dùng timer callback không | `HAL_TIM_PeriodElapsedCallback` có bị Mongoose dùng không? |
| M6 | `Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOSConfig.h` | FreeRTOS config thực tế của V3 | `configMAX_PRIORITIES`, `configTOTAL_HEAP_SIZE` thực tế. Còn bao nhiêu heap sau khi lwIP + Mongoose? |
| M7 | `usb_net/usb_netif.c` đầy đủ | Xem có conflict UART callback không | `usb_netif.c` có override `HAL_*Callback` không? |
| M8 | `SynaptiX/utils/cqueue.h` + `cqueue.c` (V2) | Xác nhận full implementation trước khi copy | Có version mới hơn V2 không? `cqueue_init_static` signature? |
| M9 | `SynaptiX/apps/app_settings.c` (V2) | Xác nhận Flash sector/address layout V2 | Sector nào được dùng? CRC có không? Để estimate effort rewrite cho V3 |
| M10 | `SynaptiX/board/board.c` (V2) đầy đủ | Xác nhận `HAL_TIM_PeriodElapsedCallback` override location | Callback được override trong board.c hay stm32h5xx_it.c? Cần biết để tránh conflict khi porting |
| M11 | Clock PCLK V3 chính xác | PCLK2 (USART1 source) = bao nhiêu MHz? | Với HSI 64MHz / DIV2 = 32MHz, PCLK2 = 32MHz (nếu APB2 = DIV1). Cần verify để tính đúng UART baud prescaler và TIM period |

---

*Tài liệu này là Migration Blueprint. Không chứa code, không chứa patch. Mọi implementation phải theo từng Phase và verify Acceptance Criteria trước khi chuyển Phase tiếp theo.*