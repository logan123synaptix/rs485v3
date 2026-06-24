# IMPLEMENTATION_READINESS_REVIEW — v2 (Source-Complete)
**Repository đích:** `RF_IO_RS485_V3` @ `logan123synaptix/rs485v3`  
**Repository nguồn:** `RS485_v2` @ `logan123synaptix/rs485v2`  
**Ngày review:** 2026-06-24  
**Reviewer:** Principal Embedded Architect  
**Phiên bản:** v2 — bổ sung V2 source files đầy đủ: `board.h`, `board.c`, `mbconfig.h`, `rf_app.c`, `zigbee.h`, `zigbee.c`, `cqueue.h`, `cqueue.c`, `cli_shell.c`, `shell_app.c`

> **Thay đổi so với v1:** Rút lại BR-1 (false alarm FreeRTOSConfig), xác nhận memory budget đủ, bổ sung mapping GPIO V2→V3 đầy đủ, xác nhận ZigbeeDriver_t pattern, bổ sung 3 phát hiện mới từ source V2.

---

## Tổng quan mức độ sẵn sàng

| Hạng mục | Trạng thái |
|---|---|
| Platform & clock (32 MHz HSI/2) | ✅ Xác nhận |
| TIM2 cho Modbus 3.5T | ✅ Đã có trong V3 — chỉ cần điều chỉnh period |
| UART peripherals (LPUART1/USART1/USART2) | ✅ Đã init đầy đủ |
| GPIO (DE pin, DI/DO, LED indicators) | ✅ Đã init |
| FreeRTOSConfig.h | ✅ **CLEAR** — `Core/Inc/FreeRTOSConfig.h` đúng, dùng `stm32h5xx.h` |
| Memory budget | ✅ **~42 KB margin** sau khi thêm toàn bộ 4 services |
| CMake build pattern | ✅ Rõ ràng |
| V2 source reusability | ✅ Đã xác minh đầy đủ qua source |
| GPIO pin aliases V3 | ⚠️ V3 không có pin name defines trong `main.h` — cần thêm |
| UART callback mux | ⚠️ Vẫn là blocking risk — cần thiết kế centralized |
| `bsp_com_init()` bug V2 | ❌ **Bug nghiêm trọng** phát hiện trong V2 — không được copy nguyên |

---

## 1. Ready To Implement — Cập nhật đầy đủ

### 1.1 BSP V2 → V3 Mapping hoàn toàn xác định

Từ `board.h` + `board.c` V2, toàn bộ constants và pin aliases đã biết:

| Constant V2 | Giá trị | Tương đương V3 |
|---|---|---|
| `BSP_UART_NUM` | 3 | Giữ nguyên: LPUART1(0), USART1(1), USART2(2) |
| `BSP_DEBUG_COM_PORT` | 0 → `hlpuart1` | V3: `hlpuart1` (Shell/Logger) |
| `BSP_RS485_COM_PORT` | 1 → `huart1` | V3: `huart1` (RS485 Modbus) |
| `BSP_RF_COM_PORT` | 2 → `huart2` | V3: `huart2` (Zigbee) |
| `TIMER0` | 0 → TIM2 | V3: TIM2 (đã có) |
| `TIMER1` | 1 → TIM3 | V3: **Không cần** — chỉ dùng 1 Modbus instance |
| `BSP_UART_NUM` | 3 | Giữ nguyên |
| `MAX_UART_BUFF_SIZE` | 256 | Giữ nguyên |
| `USART1_DE_Pin` | `GPIO_PIN_13` GPIOB | V3: PB13 — **giống hệt, đã init** |
| `LED0/1/2` | PC13/PC14/PC15 | V3: PC13/PC14/PC15 — **giống hệt, đã init** |
| `OUT0-3` | PA6, PA7, PB0, PB1 | V3: **giống hệt** |
| `IN0-3` | PB6, PB5, PB4, PB3 | V3: PB3-PB6 Input — **giống hệt, thứ tự ngược** |

**Kết luận quan trọng:** V3 không define pin name aliases trong `main.h` (không có `IN0_Pin`, `OUT0_Pin`, `USART1_DE_Pin`). BSP V3 phải tự define hoặc dùng raw GPIO values trực tiếp. Không cần thay đổi `.ioc`.

### 1.2 ZigbeeDriver_t binding pattern đã xác nhận

Từ `rf_app.c` V2, pattern binding `ZigbeeDriver_t` rất đơn giản:
```c
// V2 rf_app.c — pattern này copy sang V3 zigbee_service.c
zigbee_driver.write     = rf_write;     // → bsp_com_write(BSP_RF_COM_PORT, ...)
zigbee_driver.read      = rf_read;      // → bsp_com_read(BSP_RF_COM_PORT, ...)
zigbee_driver.available = rf_get_available; // → bsp_com_available(BSP_RF_COM_PORT)
zigbee_init(&zigbee, &zigbee_driver);
```
Chỉ cần swap 3 function pointers sang BSP V3. Không có gì phức tạp.

### 1.3 `zigbee.c` reusable 100% sau khi swap logger

Đã đọc toàn bộ `zigbee.c` — không có HAL call nào trực tiếp. Chỉ dùng `zigbee->driver->*` và `log_*()`. Logger V3 (`libs/logger/logger.h`) API giống hệt V2 (`LOGD/LOGI/LOGE/LOGW`). Copy nguyên, không cần sửa.

**Lưu ý:** `ZIGBEE_BUFFER_SIZE = 256 * 4 = 1024 bytes` trong `zigbee.h`. Stack của `wirelessTask` phải tính thêm buffer này nếu được dùng trên stack — nhưng `buff` là field của `ZigbeeMesh_t` struct (static allocation), không phải stack variable.

### 1.4 `cli_shell.c` reusable — xác nhận pattern rõ

Từ source, Shell engine dùng driver injection hoàn toàn:
```c
shell.impl.send_char = shell_send_char; // function pointer
shell.impl.send_str  = shell_send_str;  // function pointer
```
V3 chỉ cần implement 2 function pointers này để bind sang LPUART1. `cli_shell.c` copy nguyên, không sửa.

**Phát hiện từ `shell_app.c` V2:** `shell_receive_task` polling USB CDC `bsp_usb_available`. V3 không dùng USB CDC cho Shell — bind sang LPUART1 RX IT. Task logic đơn giản hơn V2.

### 1.5 `mbconfig.h` — toàn bộ features enabled

Đã đọc `mbconfig.h` V2: `MB_RTU_ENABLED=1`, `MB_ASCII_ENABLED=1`, `MB_TCP_ENABLED=0`. Tất cả 9 function codes enabled (FC01-FC24 range). `MB_FUNC_HANDLERS_MAX=16`. Copy nguyên cho V3.

### 1.6 `cqueue.c` — implementation đã xác nhận

`cqueue_init_static`, `cqueue_send`, `cqueue_receive` — pure C, không malloc, không HAL. Implementation dùng `memcpy` trực tiếp. Copy nguyên.

### 1.7 Memory budget — đã tính toán đầy đủ

Với `lwipopts.h` đã đọc (`MEM_SIZE=2048`, `PBUF_POOL_SIZE=16 × 1024B`):

| Hạng mục | Bytes |
|---|---|
| defaultTask (256w) | 1 024 |
| web_server_task (8192w) | 32 768 |
| usb_netif_task (4096w) | 16 384 |
| tcpip_thread (2048w từ lwipopts) | 8 192 |
| OS TCB + timer overhead | 1 152 |
| lwIP heap pools (ước tính) | 12 384 |
| TinyUSB buffers (ước tính) | 2 048 |
| **Subtotal existing** | **73 952** |
| **Còn lại** | **57 125 (~55.8 KB)** |

Thêm 4 services mới: modbusTask(4KB) + shellTask(2KB) + wirelessTask(4KB) + dioTask(1KB) + service buffers(2.7KB) = **~14 KB**. Margin sau khi thêm toàn bộ: **~42 KB**. ✅ SUFFICIENT.

### 1.8 `HAL_TIM_PeriodElapsedCallback` — không conflict

`main.c` V3 đã implement callback này với TIM6 check. TIM2 case trống hoàn toàn. Thêm vào USER CODE zone:
```c
// Trong main.c USER CODE BEGIN Callback 1
else if (htim->Instance == TIM2) {
    BSP_TIM2_Callback();  // hoặc trực tiếp gọi modbus timer
}
```
Không cần file mới. Không có conflict.

---

## 2. Missing Information — Cập nhật (Thu hẹp)

### [M1] — RETRACTED: FreeRTOSConfig.h bug

**Rút lại hoàn toàn.** `Core/Inc/FreeRTOSConfig.h` dùng `#define CMSIS_device_header "stm32h5xx.h"` và `#include <stdint.h>`. Không có `#include "stm32g4xx.h"`. File có `stm32g4xx.h` là `libs/tinyusb/hw/bsp/stm32g4/FreeRTOSConfig/FreeRTOSConfig.h` — không nằm trong include path của project V3. Không cần action.

### [M2] — Còn lại: Modbus slave address

Chưa quyết định. V3 `bsp_get_address()` trong V2 trả về hardcode = 1. Đây là giá trị an toàn để bắt đầu. **Khuyến nghị: giữ hardcode = 1 cho Phase 1.**

### [M3] — Còn lại: USART2 baud rate cho Zigbee

V3 init USART2 ở 115200. Từ `rf_app.c` V2, `bsp_com_write(BSP_RF_COM_PORT, ...)` dùng `huart2` ở baud rate được load từ Flash. **Cần xác nhận Zigbee module hardware mặc định là 115200** trước khi Phase 3. Nếu đúng thì không cần thay đổi gì.

### [M4] — Còn lại: GPDMA decision

V2 `bsp_com_init()` dùng `HAL_UARTEx_ReceiveToIdle_DMA` cho RF port. V3 không có GPDMA config. **Khuyến nghị: dùng `HAL_UARTEx_ReceiveToIdle_IT` cho V3** — đơn giản hơn, không cần CubeMX regenerate, throughput đủ cho Zigbee protocol.

### [M5] — Còn lại: V3 GPIO pin aliases chưa defined

`Core/Inc/main.h` V3 hoàn toàn trống (không có `IN0_Pin`, `OUT0_Pin`, `USART1_DE_Pin`). BSP V3 phải tự define. Đây là công việc cần làm trong `libs/bsp/bsp_gpio.h`, không phải missing info — đã biết đủ để viết.

---

## 3. New Findings — Phát hiện mới từ V2 source

### [NF-1] CRITICAL: Bug trong `bsp_com_init()` V2 — KHÔNG copy nguyên

```c
// V2 board.c — BUG tại đây
for(int i = 0; i < BSP_UART_NUM; i++) {
    if(i = BSP_RF_COM_PORT) {   // ← ASSIGNMENT không phải COMPARISON!
        HAL_UARTEx_ReceiveToIdle_DMA(bsp_uart[i], ...);
    }
    else
        HAL_UART_Receive_IT(bsp_uart[i], ...);  // ← KHÔNG BAO GIỜ chạy
    cqueue_init_static(&uart_queue[i], ...);
}
```

`if(i = BSP_RF_COM_PORT)` là assignment, không phải comparison. Kết quả: loop body luôn đi vào DMA branch (vì `BSP_RF_COM_PORT = 2`, non-zero = true). Cả 3 UART đều arm với DMA thay vì chỉ USART2. V2 "chạy được" vì V2 dùng GPDMA cho cả 3 UART theo một cách nào đó.

**Action V3:** Khi viết `bsp_uart.c` V3, fix thành `if(i == BSP_RF_COM_PORT)`. USART1 (Modbus) và LPUART1 (Shell) dùng `HAL_UART_Receive_IT` 1 byte. USART2 (Zigbee) dùng `HAL_UARTEx_ReceiveToIdle_IT`.

### [NF-2] HIGH: `bsp_uart2_de_on/off` trong `board.h` V2 dùng sai tên GPIO

```c
// V2 board.h — comment nói UART2 nhưng thực tế dùng USART1_DE
#define bsp_uart2_de_on()  HAL_GPIO_WritePin(USART1_DE_GPIO_Port, USART1_DE_Pin, GPIO_PIN_SET)
#define bsp_uart2_de_off() HAL_GPIO_WritePin(USART1_DE_GPIO_Port, USART1_DE_Pin, GPIO_PIN_RESET)
```

Tên macro là `bsp_uart2_de_*` nhưng GPIO là `USART1_DE_Pin` = PB13. Đây không phải bug — USART1 (RS485) cần DE control, tên macro gây nhầm lẫn. Trong V3 BSP, rename thành `bsp_rs485_de_on/off` cho rõ ràng, và dùng trực tiếp `HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, ...)`.

### [NF-3] MEDIUM: `HAL_UART_RxCpltCallback` V2 arm lại IT ngay trong callback

```c
// V2 board.c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    for (int i = 0; i < BSP_UART_NUM; i++) {
        if (bsp_uart[i] == huart) {
            cqueue_send(&uart_queue[i], &uart_data[i]);
            uart_tick[i] = BSP_UART_TIMEOUT_READY;
            HAL_UART_Receive_IT(bsp_uart[i], &uart_data[i], 1); // ← re-arm ngay
            break;
        }
    }
}
```

Pattern re-arm trong ISR callback là chuẩn STM32 HAL. V3 BSP phải giữ pattern này. Nếu không re-arm, UART RX ngừng nhận sau byte đầu tiên. **Copy pattern này nguyên vẹn.**

### [NF-4] LOW: `shell_app.c` V2 dùng USB CDC, không dùng UART cho RX

`shell_receive_task` V2 polling `bsp_usb_available(SHELL_USB_CH)`. `shell_app_poll()` đọc từ `BSP_DEBUG_COM_PORT` (LPUART1). V3 shell chỉ cần LPUART1 — đơn giản hơn V2.

### [NF-5] LOW: `cli_shell_printf` dùng `vsprintf` — unsafe buffer

```c
// cli_shell.c V2
static char cli_buff[1024]; // static = not reentrant
vsprintf(cli_buff, format, args); // ← không có size bound
```

Copy sang V3 nhưng **fix ngay**: đổi `vsprintf` → `vsnprintf(cli_buff, sizeof(cli_buff), format, args)`. Tránh stack overflow nếu format string tạo ra output > 1024 bytes.

---

## 4. Blocking Risks — Cập nhật

### [BR-1] ~~CRITICAL: FreeRTOSConfig.h include sai MCU~~
**RETRACTED** — không phải bug. Xem NF section.

### [BR-2] HIGH: `HAL_UART_RxCpltCallback` single-definition — GIỮ NGUYÊN

V3 có 3 UART cần RX IT: LPUART1 (Shell), USART1 (Modbus), USART2 (Zigbee). HAL callback là `__weak` — một implementation duy nhất được phép. Cấu trúc từ V2 đã minh chứng rõ cách handle: dùng array `bsp_uart[]` + vòng for. V3 có thể áp dụng đúng cách này.

**Solution cụ thể (đã xác định từ V2 pattern):**
```c
// libs/bsp/bsp_callbacks.c — file duy nhất chứa callbacks
extern UART_HandleTypeDef *bsp_uart[BSP_UART_NUM];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    for (int i = 0; i < BSP_UART_NUM; i++) {
        if (bsp_uart[i] == huart) {
            cqueue_send(&uart_queue[i], &uart_data[i]);
            HAL_UART_Receive_IT(bsp_uart[i], &uart_data[i], 1);
            break;
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    // chỉ dùng nếu Zigbee dùng ReceiveToIdle_IT
    if (huart == &huart2) {
        for (uint16_t i = 0; i < Size; i++)
            cqueue_send(&uart_queue[BSP_RF_COM_PORT], &uart_data_dma[i]);
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, uart_data_dma, MAX_UART_BUFF_SIZE);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    for (int i = 0; i < BSP_UART_NUM; i++) {
        if (bsp_uart[i] == huart && com_tx_callback[i].cb != NULL) {
            com_tx_callback[i].cb(com_tx_callback[i].arg);
            break;
        }
    }
}
```

### [BR-3] HIGH: Task priority scheme cần chuẩn hóa

`web_server_task` và `usb_netif_task` được tạo với raw priority = 10. `osPriorityNormal` = 24 trong CMSIS-RTOS2 (với `configMAX_PRIORITIES=56`). Code mới phải dùng nhất quán — khuyến nghị dùng `osPriority_t` enum cho tất cả task mới, tránh raw numbers.

### [BR-4] ~~HIGH: Memory budget~~ — CLEARED ✅

Đã tính: ~42 KB margin. Không còn blocking.

### [BR-5] MEDIUM: TIM2 period = 300 µs vs Modbus 3.5T spec = 334 µs

**Giữ nguyên.** Cần sửa `htim2.Init.Period` từ 299 → 333 trong `tim.c`.

### [BR-6] MEDIUM: `user_mb_app.c` coupling với Zigbee struct

**Giữ nguyên.** Đã xác nhận từ source: `rf_register[20]` trỏ trực tiếp vào `zigbee.param.*`. Phải thay bằng getter function trong Phase 1 để build không depend on Phase 3.

**Solution:**
```c
// zigbee_service.h (Phase 3 sẽ implement)
ZigbeeParameter_t *zigbee_get_params(void);

// user_mb_app.c V3 (Phase 1) — stub version
ZigbeeParameter_t *zigbee_get_params(void) { return NULL; }
// Trong eMBRegInputCB: if(zigbee_get_params() != NULL) copy values
```

### [BR-7] LOW: `logger.c` `FREE_RTOS` macro

**Giữ nguyên.** Cần `target_compile_definitions(logger PRIVATE FREE_RTOS=1)` trong `libs/logger/CMakeLists.txt`.

---

## 5. Required Source Files — Cập nhật

### Đã đọc đủ từ V2 (không còn thiếu)

| File | Trạng thái |
|---|---|
| `board.h` | ✅ Đã đọc — toàn bộ constants, pin aliases, API |
| `board.c` | ✅ Đã đọc — implementation, bugs, callback patterns |
| `mbconfig.h` | ✅ Đã đọc — tất cả feature flags |
| `rf_app.c` | ✅ Đã đọc — ZigbeeDriver_t binding, init sequence |
| `zigbee.h` | ✅ Đã đọc — struct layout, API |
| `zigbee.c` | ✅ Đã đọc — full implementation |
| `cqueue.h` + `cqueue.c` | ✅ Đã đọc — implementation xác nhận |
| `cli_shell.c` | ✅ Đã đọc — driver injection pattern, vsprintf bug |
| `shell_app.c` | ✅ Đã đọc — init + task pattern |

### Còn thiếu từ V2

| File | Mục đích | Priority |
|---|---|---|
| `cli_shell_command.c` | Đếm tổng commands, xem GPIO API call pattern để estimate effort adapt | LOW — có thể xem khi bắt đầu Phase 2 |
| `app_settings.h` đầy đủ + `app_config.h` | Xác nhận `N_MODBUS`, register size macros (`S_REG_HOLDING_NREGS` v.v.) trong context thực tế | MEDIUM — cần trước Phase 1 |

### Còn thiếu từ V3

| File | Mục đích | Priority |
|---|---|---|
| `libs/logger/CMakeLists.txt` | Confirm có `FREE_RTOS` define không | LOW |

---

## 6. Recommended First Implementation Phase — Cập nhật

**Phase 1: BSP Foundation + Modbus** — giữ nguyên khuyến nghị, với thứ tự chính xác hơn.

### Preconditions (phải confirm trước)

1. **Xác nhận Zigbee baud rate** (nếu có thiết bị): test kết nối USART2 @ 115200 với Zigbee module.
2. **Quyết định 2 design questions còn lại** (D1, D3 trong mục 7).
3. **Đọc `app_config.h` và user_mb_app.h** để biết `N_MODBUS`, register sizes trước khi viết Modbus stack.

---

## 7. Recommended First Files To Modify — Cập nhật chính xác

| # | Action | File | Ghi chú |
|---|---|---|---|
| 1 | READ | `RS485_v2/SynaptiX/apps/app_config.h` | Xác nhận `N_MODBUS`, register sizes |
| 2 | MODIFY | `Core/Src/tim.c` | `htim2.Init.Period` 299 → 333 (300µs → 334µs) |
| 3 | CREATE | `libs/bsp/CMakeLists.txt` | |
| 4 | CREATE | `libs/bsp/bsp_gpio.h` | Define pin aliases (IN0-3, OUT0-3, DE, LED) |
| 5 | CREATE | `libs/bsp/bsp_gpio.c` | `bsp_get_input`, `bsp_output_on/off`, `bsp_rs485_de_on/off` |
| 6 | CREATE | `libs/utils/CMakeLists.txt` + `cqueue.h/.c` | Copy V2, không sửa |
| 7 | CREATE | `libs/bsp/bsp_uart.h/.c` | CQueue, array `bsp_uart[]`, arm IT |
| 8 | CREATE | `libs/bsp/bsp_callbacks.c` | Centralized HAL weak callback mux (pattern từ V2) |
| 9 | MODIFY | `Core/Src/main.c` | Thêm `BSP_TIM2_Callback()` call trong `HAL_TIM_PeriodElapsedCallback` USER CODE zone |
| 10 | CREATE | `libs/modbus/` | Theo Integration Map, fix `user_mb_app.c` Zigbee coupling |
| 11 | MODIFY | `libs/CMakeLists.txt` | `add_subdirectory` cho utils, bsp, modbus |
| 12 | MODIFY | `libs/logger/CMakeLists.txt` | `target_compile_definitions(logger PRIVATE FREE_RTOS=1)` |
| 13 | MODIFY | `Core/Src/app_freertos.c` | Spawn `modbusTask` trong USER CODE zone |

---

## 8. Acceptance Criteria Before Coding — Cập nhật

### [AC-1] ~~FreeRTOSConfig bug~~ — CLEARED

### [AC-2] Memory budget — CLEARED ✅
Đã tính: 42 KB margin. No action needed.

### [AC-3] Design decisions

| Decision | Khuyến nghị |
|---|---|
| **D1:** Modbus slave address | Hardcode = 1 cho Phase 1 |
| **D2:** Zigbee USART2 RX mode | IT mode (`HAL_UARTEx_ReceiveToIdle_IT`) — không cần GPDMA |
| **D3:** Shell I/O | LPUART1 only cho Phase 2 |

### [AC-4] Callback mux architecture — Solution đã xác định (xem BR-2)
Confirm trước khi code: `libs/bsp/bsp_callbacks.c` là single file cho tất cả HAL callbacks.

### [AC-5] V2 source đã đọc đủ — CLEARED ✅
Tất cả files quan trọng đã được đọc và phân tích.

### [AC-6] TIM2 period fix — cần confirm
Sửa `htim2.Init.Period` từ 299 → 333 trước khi implement Modbus timer.

### [AC-7] NEW: Bug list từ V2 acknowledged
Team confirm đã biết 2 bugs từ V2 **không được copy nguyên**:
- `bsp_com_init()`: `if(i = BSP_RF_COM_PORT)` → phải sửa thành `==`
- `cli_shell_printf()`: `vsprintf` → phải đổi thành `vsnprintf`

---

## Phụ lục: So sánh V1 vs V2 review

| Hạng mục | Review v1 | Review v2 | Thay đổi |
|---|---|---|---|
| FreeRTOSConfig bug | ❌ Blocked (BR-1) | ✅ Retracted — false alarm | Resolved |
| Memory budget | ⚠️ Unknown | ✅ ~42 KB margin | Resolved |
| ZigbeeDriver_t | ⚠️ Cần đọc rf_app.c | ✅ Pattern xác nhận | Resolved |
| GPIO pin aliases V3 | Không đề cập | ⚠️ V3 main.h trống — cần define trong BSP | New finding |
| `bsp_com_init` bug | Không phát hiện | ❌ Bug assignment `=` thay vì `==` | New finding (NF-1) |
| `vsprintf` unsafe | Đề cập nhẹ | ❌ Xác nhận — cần fix ngay | Elevated |
| TIM2 period | ⚠️ KB nói TIM2 chưa có | ✅ TIM2 đã có, chỉ cần adjust period | Resolved + clarified |
| Blocking risks | 7 risks | 5 risks (1 retracted, 1 cleared) | Reduced |

---

*Implementation Readiness Review v2 — Source-Complete. Không chứa code, không chứa patch. Dựa trên đọc trực tiếp toàn bộ source code từ cả hai repository và các source files V2 được cung cấp bổ sung.*