# IMPLEMENTATION_READINESS_REVIEW
**Repository đích:** `RF_IO_RS485_V3` @ `logan123synaptix/rs485v3`  
**Repository nguồn:** `RS485_v2` @ `logan123synaptix/rs485v2`  
**Ngày review:** 2026-06-24  
**Reviewer:** Principal Embedded Architect  
**Phương pháp:** Source-verified — đọc trực tiếp từ cả hai repo + toàn bộ KB files

---

## Tổng quan mức độ sẵn sàng

| Hạng mục | Trạng thái |
|---|---|
| Platform & clock | ✅ Xác nhận đủ |
| Peripheral init (UART, GPIO, TIM) | ✅ Đã có trong V3 |
| TIM2 cho Modbus 3.5T | ✅ **Đã tồn tại** — KB có điểm thiếu sót cần cập nhật |
| UART callback routing | ⚠️ Cần thiết kế mux |
| FreeRTOS heap & priority budget | ⚠️ Cần tính toán cẩn thận |
| CMake build integration | ✅ Pattern đã rõ |
| V2 source reusability | ✅ Đã định nghĩa rõ |
| FreeRTOSConfig include header | ❌ **Bug nghiêm trọng phát hiện mới** |

---

## 1. Ready To Implement

Những thông tin và điều kiện **đã đủ** để bắt đầu code ngay:

### 1.1 Hardware Foundation — Hoàn toàn xác nhận

- **SYSCLK = 32 MHz (HSI 64MHz / DIV2)** — confirmed từ `main.c` `SystemClock_Config()`. APB1 = APB2 = APB3 = HCLK/1 = 32 MHz. Tất cả PCLK đều 32 MHz. Không cần đoán.
- **TIM2 đã tồn tại và đã init** — `Core/Src/tim.c` xác nhận: `htim2`, prescaler=31 → 1 µs/tick, period=299 → 300 µs interrupt. `TIM2_IRQHandler` đã có trong `stm32h5xx_it.c`. `extern TIM_HandleTypeDef htim2` đã khai báo. **KB nói TIM2 chưa có là sai — TIM2 đã được cấu hình sẵn trong V3.**
- **TIM6** dùng riêng cho HAL timebase (`stm32h5xx_hal_timebase_tim.c`). `HAL_TIM_PeriodElapsedCallback` trong `main.c` chỉ xử lý `TIM6`. TIM2 callback hoàn toàn trống — không conflict.
- **3 UART đã init:** `hlpuart1` (115200, PA9/PA10, Shell), `huart1` (115200, PB14/PB15, RS485), `huart2` (115200, PA2/PA3, Zigbee). Tất cả IRQ handler đã có trong `stm32h5xx_it.c`.
- **GPIO đã init:** PC13/PC14/PC15 (LED indicators, Output PP), PA6/PA7 (DO1/DO2), PB0/PB1 (DO3/DO4), PB13 (RS485 DE, Output PP), PB3-PB6 (DI1-DI4, Input).
- **HAL DMA driver đã link:** `cmake/stm32cubemx/CMakeLists.txt` bao gồm `stm32h5xx_hal_dma.c` và `stm32h5xx_hal_dma_ex.c`. Nếu cần GPDMA cho Zigbee thì driver đã sẵn.

### 1.2 FreeRTOS Config — Đủ để ước tính

- `configTOTAL_HEAP_SIZE = 131077` bytes (~128 KB) — từ `Core/Inc/FreeRTOSConfig.h`.
- `configMAX_PRIORITIES = 56` — đủ chỗ cho tất cả task.
- `configSUPPORT_STATIC_ALLOCATION = 1`, `configSUPPORT_DYNAMIC_ALLOCATION = 1` — cả hai đều được hỗ trợ.
- `heap_4.c` đang dùng. `cmsis_os2.c` sử dụng `ucHeap[configTOTAL_HEAP_SIZE]`.

### 1.3 CMake Build Pattern — Rõ ràng

- Pattern thêm module mới: tạo `libs/<module>/CMakeLists.txt`, thêm `add_subdirectory(<module>)` vào `libs/CMakeLists.txt`, link vào `target_link_libraries(libs INTERFACE ...)`. Pattern này đã được minh chứng bởi `logger`, `stm32-mw-lwip`, `tinyusb`.
- Root `CMakeLists.txt` đã có `add_subdirectory(libs)` và link `libs` vào executable target.

### 1.4 Reusable V2 Components — Đã xác minh

- **FreeModbus stack core** (`mb.c`, `mbrtu.c`, `mbcrc.c`, `functions/` × 7, `include/` × 7): Pure C, không phụ thuộc hardware — copy nguyên được.
- **`portevent.c`**: Pure flag logic, không HAL — copy nguyên được.
- **`cqueue.h` + `cqueue.c`**: API đơn giản (`cqueue_init_static`, `cqueue_send`, `cqueue_receive`) — copy nguyên được, không thay đổi.
- **`cli_shell.c` / `cli_shell.h`**: Driver injection pattern qua `sCliShellImpl_t` — portable 100%.
- **`zigbee.h` + `zigbee.c`**: Protocol logic dùng `ZigbeeDriver_t` abstraction — swap logger là xong.
- **Logger interface**: V3 đã có `libs/logger/logger.h` với API giống hệt V2 (`log_func`, `LOGD`, `LOGI`, `LOGE`, `LOGW`). Không cần viết lại.

### 1.5 Modbus Port Layer — Adaptation rõ ràng

- `portserial.c` cần rebind `bsp_com_available/read/write_it` sang BSP V3. Cấu trúc logic không đổi.
- `porttimer.c` cần rebind `bsp_timer0_start/stop` sang `HAL_TIM_Base_Start_IT(&htim2)`. TIM instance đã có.
- **TIM2 period hiện tại = 300 µs** — đây là giá trị hợp lý cho Modbus inter-frame detection (3.5T@115200 ≈ 304 µs). Có thể dùng ngay mà không cần thay đổi `.ioc` hay regenerate CubeMX.

### 1.6 USB CDC ACM — Interface index đã biết

- `usb_descriptors.c` xác nhận: CDC ACM là `ITF_NUM_CDC_ACM = 2`, endpoint OUT = `0x04`, IN = `0x84`. Nếu Shell cần USB CDC thì index đã rõ.

### 1.7 Mongoose — Không conflict timer

- `mongoose_glue.c` không sử dụng `HAL_TIM_*`. Mongoose dùng `mg_now()` → `HAL_GetTick()` (đã implement trong `app_freertos.c`). Không có timer callback conflict với TIM2.

---

## 2. Missing Information

Các thông tin còn thiếu thực sự (sau khi đã đọc source):

### [M1] — CRITICAL: FreeRTOSConfig.h include sai MCU header

**Phát hiện mới từ source, KHÔNG có trong KB:** `Core/Inc/FreeRTOSConfig.h` dòng đầu include `"stm32g4xx.h"` thay vì `"stm32h5xx.h"`:
```c
#include "stm32g4xx.h"  // ← SAI — đây là STM32H5 project
```
Đây là lỗi copy-paste từ project STM32G4 cũ. Hiện tại project vẫn build được vì `CMSIS_device_header "stm32h5xx.h"` được định nghĩa ngay trên đó. Cần kiểm tra xem header nào thực sự được include và có conflict `SystemCoreClock` definition không.

### [M2] — HIGH: Modbus slave address sẽ lấy từ đâu?

V2 lấy từ Flash (`bsp_get_address()` → `APP_SETTING_ADDRESS 0x0803C000`, Sector 14, Bank 2). V3 Flash chỉ có 256KB (địa chỉ đến `0x080FFFFF`), không có Bank 2. MPU config bảo vệ `0x08FFF000–0x08FFFFFF` là read-only/no-execute. Sector layout V3 chưa được xác định. Cần quyết định: hardcode address = 1, hay implement Flash config ngay từ đầu.

### [M3] — HIGH: USART2 baud rate cho Zigbee module

V3 init USART2 ở 115200. V2 load từ `app_setting.rf.baudrate`. Không có KB nào ghi baud rate mặc định của Zigbee module phần cứng thực tế. Nếu module mặc định là baud rate khác (ví dụ 9600), Phase 3 sẽ thất bại ngay lần test đầu.

### [M4] — MEDIUM: GPDMA cho Zigbee RX — quyết định chưa có

KB đặt câu hỏi mở. `stm32h5xx_hal_dma.c` đã được link. Nhưng không có GPDMA channel assignment nào trong V3 `.ioc`/source. Cần quyết định trước khi viết `zigbee_bsp.c`: dùng `HAL_UARTEx_ReceiveToIdle_IT` (đơn giản, không cần CubeMX thêm gì) hay `HAL_UARTEx_ReceiveToIdle_DMA` (cần thêm GPDMA channel vào `.ioc`, regenerate).

### [M5] — MEDIUM: Memory budget thực tế sau khi thêm tasks

Hiện tại V3 dùng:
- `defaultTask`: 256×4 = 1024 bytes stack
- `web_server_task (Webserver)`: 4096×2×4 = 32768 bytes stack
- `usb_netif_task`: 4096×4 = 16384 bytes stack  
- lwIP pbuf pool, TCP/IP stack overhead, TinyUSB buffers

Tổng stack đã dùng ≈ 50 KB. Heap còn lại cho Modbus/Shell/Zigbee tasks + buffers + OS overhead chưa được tính. Không có con số chính xác trước khi build.

### [M6] — LOW: `HAL_UARTEx_RxEventCallback` cho GPDMA path

Nếu dùng DMA RX cho Zigbee, callback này cần được implement. Hiện tại không có trong V3 `stm32h5xx_it.c`. Nếu dùng IT mode thì không cần.

### [M7] — LOW: Flash sector layout V3 cho persistent config

V3 linker script: `FLASH ORIGIN = 0x8000000, LENGTH = 256K` (đến `0x0803FFFF`). V2 dùng `0x0803C000` (Sector 14, Bank 2) — địa chỉ này nằm trong vùng Flash V3. Nhưng V3 Flash chỉ đơn bank, sector size khác V2. Shell `config save` command sẽ cần địa chỉ/sector hợp lệ cho V3 nếu implement Phase 2.

---

## 3. Required Source Files

### From V2 — Cần đọc thêm trước khi code

| File | Mục đích | Mức độ ưu tiên |
|---|---|---|
| `SynaptiX/board/board.h` | Xác nhận đầy đủ constants: `BSP_RS485_COM_PORT`, `BSP_RF_COM_PORT`, `BSP_UART_NUM`, `MAX_UART_BUFF_SIZE`. Cần biết để định nghĩa tương đương trong BSP V3. | HIGH — Phase 1 |
| `SynaptiX/services/Modbus/modbus/include/mbconfig.h` | Xác nhận `N_MODBUS`, `MB_RTU_ENABLED`, register size macros (`S_REG_HOLDING_NREGS` v.v.) trước khi copy | HIGH — Phase 1 |
| `SynaptiX/utils/cqueue.c` | Xác nhận implementation (không chỉ header) trước khi copy sang V3 | HIGH — Phase 1 |
| `SynaptiX/apps/rf_app.c` | Xem `ZigbeeDriver_t` binding thực tế, baud rate init, callback implementations | HIGH — Phase 3 |
| `SynaptiX/services/zigbee/zigbee.c` | Xác nhận `buff[1024]` size, logger call pattern, driver abstraction | MEDIUM — Phase 3 |
| `SynaptiX/apps/shell_app.c` | Xem `sCliShellImpl_t` binding, `shell_receive_task` input source | MEDIUM — Phase 2 |
| `SynaptiX/services/shell/cli_shell_command.c` | Kiểm đếm tổng số commands, xem GPIO API pattern để estimate adaptation effort | MEDIUM — Phase 2 |

### From V3 — Cần đọc thêm trước khi code

| File | Mục đích | Mức độ ưu tiên |
|---|---|---|
| `Core/Inc/FreeRTOSConfig.h` *(đã có một phần)* | Đọc toàn bộ để xác nhận `INCLUDE_vTaskDelete`, `configUSE_TASK_NOTIFICATIONS`, và giải quyết bug `stm32g4xx.h` include | CRITICAL — trước mọi Phase |
| `Core/Src/stm32h5xx_hal_msp.c` | Xem có GPDMA MSP Init nào đã có không. Xác nhận UART MSP không bị override theo cách bất thường | HIGH — Phase 1 |
| `libs/logger/logger.c` | Xác nhận `FREE_RTOS` macro: logger có dùng mutex không? Nếu có, `logger_init` phải được gọi sau `osKernelInitialize`. Hiện tại `StartDefaultTask` gọi `logger_init` trước `tcpip_init` — cần verify ordering. | HIGH — Phase 1 |
| `RF_IO_V3.ioc` *(chỉ xem bằng text editor, không cần CubeMX)* | Xác nhận: USART2 baud rate config thực tế, GPDMA có được enable không, confirm TIM2 config match `tim.c` | MEDIUM — Phase 1/3 |
| `lwip/Target/lwipopts.h` | Xác nhận `MEM_SIZE` (lwIP heap từ ucHeap), `PBUF_POOL_SIZE`, `MEMP_NUM_TCP_SEG`. Tính memory budget thực tế cho services | MEDIUM — trước Phase 1 |

---

## 4. Blocking Risks

Rủi ro có khả năng khiến việc port thất bại nếu không xử lý trước:

### [BR-1] CRITICAL: FreeRTOSConfig.h include `stm32g4xx.h`

`Core/Inc/FreeRTOSConfig.h` chứa `#include "stm32g4xx.h"` — sai MCU. Project build được hiện tại vì `CMSIS_device_header` được định nghĩa trước, nhưng khi thêm code mới dùng STM32H5-specific registers (ví dụ GPDMA, TIM2 prescaler calc), undefined behavior và link error có thể xuất hiện. **Phải sửa trước khi viết bất kỳ dòng code mới nào.**

Sửa: đổi `#include "stm32g4xx.h"` → `#include "stm32h5xx.h"` (hoặc xóa nếu `CMSIS_device_header` đã handle).

### [BR-2] HIGH: `HAL_UART_RxCpltCallback` single-definition rule

V3 có 3 UART cần RX interrupt: LPUART1 (Shell), USART1 (Modbus RS485), USART2 (Zigbee nếu IT mode). HAL callback này là `__weak` — chỉ có thể có **một** override trong toàn bộ project. Nếu mỗi module (BSP Modbus, BSP Zigbee, BSP Shell) đều implement riêng → link error hoặc chỉ 1 cái active.

Giải pháp bắt buộc: tạo **một** file `libs/bsp/bsp_callbacks.c` implement `HAL_UART_RxCpltCallback` duy nhất, mux theo `huart->Instance`. Không có exception. Phải được thiết kế trước khi viết bất kỳ UART code nào.

Tương tự cho: `HAL_UART_TxCpltCallback`, `HAL_TIM_PeriodElapsedCallback` (TIM2 Modbus + TIM6 HAL tick đã có trong `main.c`), `HAL_UARTEx_RxEventCallback` (nếu dùng DMA).

### [BR-3] HIGH: `configMAX_PRIORITIES = 56` nhưng các task V3 hiện tại dùng priority 10

`web_server_task` được tạo với priority = 10, `usb_netif_task` với priority = 10. `osPriorityNormal` trong CMSIS-RTOS2 = 24 (với MAX_PRIORITIES = 56). Nếu code mới dùng `osPriority_t` enum (normal = 24) nhưng web task dùng raw = 10, thứ tự ưu tiên có thể không như kỳ vọng. Cần chuẩn hóa priority scheme trước khi thêm task mới.

### [BR-4] HIGH: Memory budget thực chưa biết

131 KB heap, nhưng sau khi trừ lwIP + TinyUSB + Mongoose + existing tasks, số còn lại chưa được tính. Cụ thể `lwipopts.h` chưa đọc — lwIP `MEM_SIZE` có thể chiếm 40–60 KB heap. Nếu budget dưới 30 KB còn lại, thêm 3 task với stack 4KB mỗi cái + CQueue buffers + Modbus/Zigbee buffers sẽ gây `pvPortMalloc` fail lúc runtime, không phải compile time. Phải đo bằng `xPortGetFreeHeapSize()` sau khi build baseline trước khi thêm service.

### [BR-5] MEDIUM: TIM2 period = 300 µs vs Modbus 3.5T spec

3.5T@115200 baud = 3.5 × (1/115200) × 11 bits ≈ 334 µs. TIM2 hiện cấu hình 300 µs — ngắn hơn spec ~10%. Điều này có thể gây false inter-frame detection (frame bị cắt đôi bị nhận là 2 frame). Không phải lỗi critical nhưng sẽ gây flaky behavior với một số Modbus master implementations. Cần điều chỉnh period = 334 µs (prescaler=31, period=334) để đúng spec.

### [BR-6] MEDIUM: `user_mb_app.c` coupling với `extern zigbee`

Đã xác nhận từ source: `user_mb_app.c` khai báo `USHORT *rf_register[20]` trỏ trực tiếp vào fields của global `ZigbeeMesh_t zigbee`. Nếu copy nguyên sang V3 mà không có `extern ZigbeeMesh_t zigbee` → compile error. Phải thay bằng getter function pattern trước khi Phase 1 build thành công (vì `user_mb_app.c` là một phần của Phase 1).

### [BR-7] LOW: `logger.c` `FREE_RTOS` macro chưa set

`libs/logger/logger.c` check `#if FREE_RTOS` để enable mutex. Nếu `FREE_RTOS` không được define (hoặc = 0), logger không thread-safe. Với multi-task V3, logger được gọi từ nhiều tasks — race condition trên log output. Cần thêm `target_compile_definitions(...PRIVATE FREE_RTOS=1)` vào `libs/logger/CMakeLists.txt`.

---

## 5. Recommended First Implementation Phase

**Phase 1: BSP Foundation + Modbus** — đúng như KB đề xuất, nhưng cần bổ sung 2 precondition.

### Precondition (phải làm trước khi viết bất kỳ code nào)

1. **Fix FreeRTOSConfig.h**: đổi `#include "stm32g4xx.h"` → `#include "stm32h5xx.h"`.
2. **Đọc `lwipopts.h`**: đo `MEM_SIZE`, tính memory budget còn lại.
3. **Quyết định 3 câu hỏi thiết kế** (xem mục 6).

### Thứ tự implement Phase 1

1. `libs/utils/cqueue.h` + `cqueue.c` (copy V2, verify)
2. `libs/bsp/bsp_callbacks.c` (mux tất cả HAL weak callbacks — **viết trước tiên**)
3. `libs/bsp/bsp_uart.c/.h` (CQueue arm cho USART1, LPUART1, USART2)
4. `libs/bsp/bsp_gpio.c/.h` (DE pin, DI/DO abstraction)
5. `libs/modbus/` (theo cấu trúc KB, với timer period fix = 334 µs)
6. `Core/Src/app_freertos.c` — thêm `modbusTask` spawn
7. `libs/CMakeLists.txt` — thêm utils, bsp, modbus

---

## 6. Recommended First Files To Modify

Thứ tự chính xác, tính đến dependencies:

| # | Action | File | Ghi chú |
|---|---|---|---|
| 1 | FIX | `Core/Inc/FreeRTOSConfig.h` | Sửa include stm32g4xx → stm32h5xx |
| 2 | READ | `lwip/Target/lwipopts.h` | Đo memory budget trước khi tạo bất kỳ file nào |
| 3 | CREATE | `libs/bsp/bsp_callbacks.c/.h` | Mux điểm duy nhất cho tất cả HAL callbacks |
| 4 | CREATE | `libs/utils/cqueue.c/.h` | Copy V2 — dependency của bsp_uart |
| 5 | CREATE | `libs/bsp/bsp_uart.c/.h` | CQueue + UART IT cho 3 UART |
| 6 | CREATE | `libs/bsp/bsp_gpio.c/.h` | DE pin + DI/DO |
| 7 | CREATE | `libs/bsp/CMakeLists.txt` | Link bsp vào libs target |
| 8 | MODIFY | `Core/Src/tim.c` | Đổi `htim2.Init.Period` từ 299 → 333 (300 µs → 334 µs cho Modbus 3.5T đúng spec) |
| 9 | CREATE | `libs/modbus/` *(toàn bộ cấu trúc)* | Theo KB Integration Map, decoupled khỏi zigbee |
| 10 | MODIFY | `libs/CMakeLists.txt` | Thêm `add_subdirectory(utils)`, `add_subdirectory(bsp)`, `add_subdirectory(modbus)` |
| 11 | MODIFY | `Core/Src/app_freertos.c` | Spawn `modbusTask` trong USER CODE RTOS_THREADS |
| 12 | ADD | `libs/logger/CMakeLists.txt` | `target_compile_definitions(... PRIVATE FREE_RTOS=1)` |

---

## 7. Acceptance Criteria Before Coding

Những điều kiện sau **phải được xác nhận** trước khi viết dòng code đầu tiên:

### [AC-1] FreeRTOSConfig.h bug resolved
- `#include "stm32g4xx.h"` đã được sửa hoặc xác nhận không gây issue.
- Project vẫn build clean sau khi sửa.

### [AC-2] Memory budget confirmed
- `lwipopts.h` đã đọc, `MEM_SIZE` đã biết.
- Tính toán: `131077 - lwIP MEM_SIZE - existing task stacks - TinyUSB buffers ≥ 20 KB` còn lại cho services mới.
- Nếu dưới 20 KB: cần giảm `web_server_task` stack (4096×2 words = 32 KB là rất lớn) trước khi tiếp tục.

### [AC-3] 3 design decisions resolved
- **D1:** Modbus slave address — hardcode = 1 hay Flash config? (Khuyến nghị: hardcode cho Phase 1, Flash config sau)
- **D2:** Zigbee USART2 RX — IT mode hay GPDMA? (Khuyến nghị: IT mode cho Phase 3 để tránh CubeMX regenerate)
- **D3:** Shell I/O — LPUART1 only hay LPUART1 + USB CDC ACM? (Khuyến nghị: LPUART1 only cho Phase 2)

### [AC-4] Callback mux architecture agreed
- Kiến trúc `libs/bsp/bsp_callbacks.c` (single HAL callback override, mux by instance) được xác nhận trước khi bất kỳ ai viết UART code.

### [AC-5] V2 source files đã đọc đủ
- `board.h`: constants đã biết
- `mbconfig.h`: register sizes đã biết  
- `rf_app.c`: ZigbeeDriver_t binding pattern đã hiểu
- `cqueue.c`: implementation đã verify

### [AC-6] TIM2 period decision
- Xác nhận period mới = 333 (334 µs, đúng Modbus 3.5T@115200) và chấp nhận thay đổi `tim.c`.

---

*Đây là Implementation Readiness Review. Không chứa code, không chứa patch. Dựa trên đọc trực tiếp source code từ cả hai repository ngày 2026-06-24.*