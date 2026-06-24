# KB_V2_SHELL
**Source Repository:** `logan123synaptix/rs485v2` (RS485_IO_RF_V2)
**Base Reference:** `docs/KB_v2.md` (Source of Truth — không lặp lại ở đây)
**Scope:** Implementation knowledge bổ sung cho việc port sang RF_IO_RS485_V3

---

## Purpose

Shell subsystem cung cấp một **CLI (Command-Line Interface) tương tác** cho phép người dùng cấu hình và điều khiển thiết bị trong runtime qua USB CDC. Đây là công cụ vận hành và debug chính của firmware, cho phép:

- Đọc/ghi cấu hình serial (RS485, RF, USB)
- Đọc/ghi cấu hình Zigbee/LoRa module
- Điều khiển GPIO output, đọc GPIO input
- Thay đổi app mode (Parency Transfer vs Modbus)
- Thay đổi log level runtime
- Cấu hình Modbus slave address
- Restart thiết bị

---

## Responsibilities

| Layer | File | Responsibility |
|---|---|---|
| Core shell engine | `services/shell/cli_shell.c/.h` | RX buffer, tokenizer, command dispatch, output primitives |
| Command table | `apps/cli_shell_command.c` | Define tất cả commands + handlers, export global table |
| Shell app binding | `apps/shell_app.c` | Bind USB CDC I/O vào shell, tạo FreeRTOS RX task |
| Logger | `services/logger.c/.h` | Output log qua `printf` → `bsp_com_write(BSP_DEBUG_COM_PORT)` |

---

## Architecture

```
┌────────────────────────────────────────────────────────────────┐
│              USB CDC Channel 0 (SHELL_USB_CH = 0)              │
│                   bsp_usb_transmit / bsp_usb_receiver          │
└──────────────────┬──────────────────────────┬──────────────────┘
                   │ TX (char/string)          │ RX (char)
        ┌──────────▼──────────┐   ┌───────────▼────────────────┐
        │  shell_send_char()  │   │  shell_receive_task()       │
        │  shell_send_str()   │   │  (FreeRTOS task, pri MAX-1) │
        │  [vTaskDelay(10ms)] │   │  polls bsp_usb_available()  │
        └──────────┬──────────┘   └───────────┬────────────────┘
                   │                           │
        ┌──────────▼───────────────────────────▼────────────────┐
        │                  ShellContext_t shell                  │
        │  ┌─────────────────────────────────────────────────┐  │
        │  │  sCliShellImpl_t impl                           │  │
        │  │    .send_char = shell_send_char                 │  │
        │  │    .send_str  = shell_send_str                  │  │
        │  ├─────────────────────────────────────────────────┤  │
        │  │  rx_buffer[256]  ←  cli_shell_receive_char()   │  │
        │  │  rx_size                                        │  │
        │  └─────────────────────────────────────────────────┘  │
        └────────────────────────┬───────────────────────────────┘
                                 │ prv_process() on '\n' or buffer full
        ┌────────────────────────▼───────────────────────────────┐
        │               Command Dispatch                         │
        │  tokenize rx_buffer → argv[] / argc                   │
        │  prv_find_command(argv[0])                             │
        │  → linear search g_shell_commands[g_num_shell_commands]│
        └────────────────────────┬───────────────────────────────┘
                                 │ command->handler(shell, argc, argv)
        ┌────────────────────────▼───────────────────────────────┐
        │              Command Handlers (cli_shell_command.c)    │
        │  restart | rs485 | usb | rf | config | output | input │
        │  mode | zigbee | lora | help                          │
        └────────────────────────────────────────────────────────┘
```

**Shell là polling + event-driven kết hợp:**
- **RX side:** FreeRTOS task riêng (`shell_receive_task`) — poll USB buffer mỗi 1ms
- **TX side:** synchronous trong cùng context gọi (handler hoặc callback)
- **Processing:** synchronous tại thời điểm nhận `'\n'`

---

## Module Structure

```
SynaptiX/services/shell/
    cli_shell.h     — Public API, struct definitions, constants, extern table declarations
    cli_shell.c     — Core engine: buffer management, tokenizer, dispatch, output primitives

SynaptiX/apps/
    shell_app.c     — I/O binding (USB CDC), FreeRTOS task, shell instance
    cli_shell_command.c — All command handlers + g_shell_commands table definition
```

---

## Command Framework

### Struct định nghĩa command

```c
struct Cli_Shell_Cmd_t {
    const char *command;                                           // command name (string match)
    int (*handler)(ShellContext_t *shell, int argc, char *argv[]); // handler function
    const char *help;                                              // help text string
};
```

### Bảng command (global, read-only)

```c
// cli_shell_command.c — định nghĩa
static const Cli_Shell_Cmd s_shell_commands[] = { ... };
const Cli_Shell_Cmd *const g_shell_commands = s_shell_commands;
const size_t g_num_shell_commands = ARRAY_SIZE(s_shell_commands);

// cli_shell.h — extern declaration (consumed by cli_shell.c)
extern const Cli_Shell_Cmd *const g_shell_commands;
extern const size_t g_num_shell_commands;
```

### Danh sách commands hiện tại

| Command | Handler | Mô tả |
|---|---|---|
| `help` | `cli_shell_help_handler` | In toàn bộ command list + help text |
| `restart` | `cli_cmd_restart` | Đếm ngược 2s rồi `NVIC_SystemReset()` |
| `rs485` | `cli_serial_param` | Đọc/ghi cấu hình RS485 serial |
| `usb` | `cli_serial_param` | Đọc/ghi cấu hình USB serial |
| `rf` | `cli_serial_param` | Đọc/ghi cấu hình RF serial |
| `config` | `cli_config_param` | Đọc/ghi config tổng hợp (addr, baud, log level) |
| `output` | `cli_set_output` | Điều khiển OUT0–OUT3 (`#if APPLICATION_MODE_GPIO`) |
| `input` | `cli_get_input` | Đọc IN0–IN3 (`#if APPLICATION_MODE_GPIO`) |
| `mode` | `cli_mode` | Chuyển giữa Parency_Transfer và Parency_Modbus |
| `lora` | `cli_lora` | Quản lý LoRa module (`#if LORA_ENABLE`) |
| `zigbee` | `cli_zigbee` | Quản lý Zigbee module (`#if ZIGBEE_ENABLE`) |

---

## Command Registration Model

**Model: Static compile-time table — không có dynamic registration.**

```
Compile time:
    cli_shell_command.c định nghĩa s_shell_commands[] (static const array)
    → export qua g_shell_commands pointer và g_num_shell_commands size

Runtime:
    cli_shell.c iterate qua g_shell_commands[0..g_num_shell_commands-1]
    → không có add/remove command tại runtime
    → không có hashtable, không có linked list
    → linear search O(n) mỗi lần dispatch
```

**Cách thêm command mới trong V2:**
1. Khai báo `static int my_handler(ShellContext_t*, int, char**)` trong `cli_shell_command.c`
2. Thêm entry `{"mycommand", my_handler, "help text"}` vào `s_shell_commands[]`
3. Không cần thay đổi file nào khác — `g_num_shell_commands` tự cập nhật qua `ARRAY_SIZE()`

**Compile-time guards:**
- `output`, `input`: `#if APPLICATION_MODE_GPIO`
- `lora`: `#if LORA_ENABLE`
- `zigbee`: `#if ZIGBEE_ENABLE`

---

## Command Dispatch Flow

```
cli_shell_receive_char(shell, '\n')
    ↓
prv_process(shell)
    ├── [guard] last char != '\n' && buffer not full → return (wait more)
    ├── Tokenize rx_buffer:
    │     for each char in rx_buffer:
    │       space / '\n' → terminate token, save argv[argc++]
    │       non-space   → mark start of next token (next_arg)
    │     Result: argv[0..argc-1] = pointers into rx_buffer (in-place mutation)
    ├── [guard] argc == 0 → skip dispatch, just reset + prompt
    ├── prv_find_command(argv[0])
    │     → linear scan g_shell_commands[]
    │     → strcmp(command->command, name)
    │     → return first match or NULL
    ├── [if NULL] → print "Unknown command: X\nType 'help'..."
    └── [if found] → command->handler(shell, argc, argv)
                      → handler prints result via cli_shell_printf/put_line
    ↓
prv_reset_rx_buffer(shell)   // memset 0, rx_size = 0
prv_send_prompt(shell)       // print SHELL_PROMPT (= "" — empty)
```

**Tokenizer đặc điểm:**
- In-place: không copy, argv[] là pointers vào `rx_buffer` được null-terminated tại chỗ
- Delimiter: space (`' '`) và newline (`'\n'`)
- Max args: `SHELL_MAX_ARGS = 16`
- Không support quoted strings, không support escape sequences

---

## Input Processing Flow

### Primary path: USB CDC → FreeRTOS task

```
USB hardware RX interrupt
    ↓
HAL_PCD_IRQHandler → USBD stack → CDC_Receive_FS callback
    ↓
bsp_usb_rx_callback(ch=0, buff, len)
    → for each byte: cqueue_send(&usb_rx_queue[0], byte)
    ↓
[FreeRTOS task: shell_receive_task, priority = configMAX_PRIORITIES-1]
    loop:
        bsp_usb_available(SHELL_USB_CH=0) → usb_rx_queue[0].count
        if > 0:
            for each available byte:
                bsp_usb_receiver(0, &c, 1) → cqueue_receive
                cli_shell_receive_char(&shell, c)
        vTaskDelay(1)   // yield 1ms
```

### Secondary path: UART (BSP_DEBUG_COM_PORT = LPUART1) — `shell_app_poll()`

```c
// shell_app_poll() — gọi từ app_poll() nhưng hiện bị comment out:
// shell_app_poll();  ← disabled trong app.c

void shell_app_poll() {
    char c = 0;
    if(bsp_com_read(BSP_DEBUG_COM_PORT, (uint8_t*)&c, 1) == 1)
        cli_shell_receive_char(&shell, c);
}
```

**Quan trọng:** `shell_app_poll()` hiện **bị comment out** trong `app.c`. UART path không hoạt động trong V2. Chỉ USB CDC path hoạt động.

### Xử lý ký tự đặc biệt

| Char | Hành động |
|---|---|
| `'\r'` | **Bị drop hoàn toàn** — `cli_shell_receive_char` return ngay |
| `'\n'` | Trigger `prv_process()` — execute command |
| `'\b'` | Backspace: `rx_buffer[--rx_size] = '\0'` |
| Buffer full | `prv_process()` được trigger kể cả khi không có `'\n'` |
| Echo | **Disabled** — `prv_echo(s_shell, c)` bị comment out trong `cli_shell_receive_char` |

**Không có history, không có auto-completion, không có arrow key support.**

---

## Output Processing Flow

### Shell output (command response)

```
handler gọi cli_shell_printf(shell, fmt, ...) hoặc cli_shell_put_line(shell, str)
    ↓
cli_shell_printf:
    vsprintf(cli_buff[1024], format, args)   ← static buffer, không reentrant
    cli_shell_put_line(shell, cli_buff)
    ↓
cli_shell_put_line:
    prv_echo_str(shell, str)
    prv_echo(shell, '\n')
    ↓
prv_echo_str:
    shell->impl.send_str(shell->impl.arg, str)
    = shell_send_str(NULL, str)
    = bsp_usb_transmit(SHELL_USB_CH=0, str, strlen(str))
    + vTaskDelay(10)    ← 10ms delay sau mỗi string TX
```

### Logger output (log_info / log_debug / v.v.)

```
log_info(TAG, fmt, ...)
    ↓
log_func(LOGGER_INFO, TAG, fmt, ...)
    ↓
vsprintf(buff[2048], ...)   ← static global buffer
serial_write(buff)
    = log_puts() (được set tại bsp_init())
    = bsp_com_write(BSP_DEBUG_COM_PORT=0, str, len)
    = HAL_UART_Transmit(&hlpuart1, ...)   ← LPUART1, blocking
```

**Logger và Shell output đi ra 2 interface khác nhau:**
- Shell response → **USB CDC channel 0**
- Log output → **LPUART1 (BSP_DEBUG_COM_PORT)**

---

## UART Integration

Shell subsystem có **2 UART liên quan**, với vai trò hoàn toàn khác nhau:

| UART | BSP index | HAL handle | Vai trò trong Shell |
|---|---|---|---|
| LPUART1 | `BSP_DEBUG_COM_PORT = 0` | `hlpuart1` | **Logger output only** — `log_*` macros ghi ra đây |
| (không dùng) | `BSP_RS485_COM_PORT = 1` | `huart1` | Không liên quan đến Shell |
| USART2 | `BSP_RF_COM_PORT = 2` | `huart2` | Không liên quan đến Shell (Zigbee) |

**Secondary UART input path (`shell_app_poll`):** `BSP_DEBUG_COM_PORT` (LPUART1) có thể là input nếu `shell_app_poll()` được bật — nhưng hiện bị disabled.

---

## USB Integration

```
USB CDC Channel 0 (SHELL_USB_CH = 0)
    ↑↓
bsp_usb_transmit(0, buff, len)  → CDC_Transmit(0, buff, len)
bsp_usb_receiver(0, buff, len)  → cqueue_receive(&usb_rx_queue[0], ...)
bsp_usb_available(0)            → usb_rx_queue[0].count

RX buffer: usb_rx_queue[0] — CQueue_t, 256 bytes (MAX_UART_BUFF_SIZE)
TX: trực tiếp vào USB stack (không có TX queue — blocking nếu USB busy)
```

**TX delay:** `shell_send_char()` và `shell_send_str()` đều gọi `vTaskDelay(10)` sau mỗi lần transmit. Với một chuỗi dài, tổng delay = `N_calls × 10ms`.

**USB availability:** Shell không check USB enumeration status trước khi transmit. Nếu USB chưa connected, `CDC_Transmit()` sẽ trả error nhưng shell không xử lý.

---

## Logging Integration

| Property | Value |
|---|---|
| Logger là singleton | `static` internal state trong `logger.c` |
| Output function | `p_log_func serial_write` — function pointer, set tại `bsp_init()` |
| Default output | `log_puts()` → `bsp_com_write(0, ...)` → LPUART1 blocking TX |
| Log levels | `DEBUG=0, INFO=1, WARNING=2, ERROR=3, OFF=4` |
| Log level filter | `if (LOG_LEVEL > level) return;` — runtime configurable |
| Log level config | Thay đổi qua shell: `config -c -l [0-4]` → `app_setting_save()` |
| Thread safety (V2) | `FREE_RTOS = 0` → **mutex disabled** → `log_func` không thread-safe |
| Static buffer | `char buff[2048]` — global, shared, không reentrant |

**Shell commands dùng cả logger lẫn shell output:**
```c
// Pattern trong cli_shell_command.c:
log_info(TAG, "Connect to Zigbee module success");           // → LPUART1
cli_shell_put_line(shell, "{\"result\":\"success\"}\r\n");  // → USB CDC
```
Hai output channel phục vụ mục đích khác nhau: logger cho developer debug, shell output cho client/automation.

---

## Buffer Ownership

| Buffer | Kích thước | Owner | Thread |
|---|---|---|---|
| `ShellContext_t shell.rx_buffer[256]` | 256 bytes | `shell_app.c` (static global) | Duy nhất: `shell_receive_task` |
| `cli_buff[1024]` | 1024 bytes | `cli_shell.c` (static) | Shared — không reentrant |
| `buff[2048]` (logger) | 2048 bytes | `logger.c` (static global) | Shared — không reentrant |
| `usb_rx_queue[0]` | 256 bytes | `board.c` (static) | ISR write, task read |
| `argv[SHELL_MAX_ARGS]` | 16 × 4 bytes | Stack của `prv_process()` | Task-local |

**In-place tokenization:** `rx_buffer` bị mutate trực tiếp bởi tokenizer (null-terminate tại chỗ). Sau `prv_process()`, `rx_buffer` không còn giữ original string.

---

## State Machine

Shell không có explicit state enum. State được biểu diễn qua:

```
State 1 — NOT BOOTED:
    shell->impl.send_char == NULL
    → cli_shell_receive_char: return ngay (prv_booted() = false)
    → prv_send_char: return ngay

State 2 — IDLE (booted, rx_size == 0):
    Chờ input từ USB CDC

State 3 — ACCUMULATING:
    rx_buffer đang được fill, rx_size > 0, last char != '\n'
    → cli_shell_receive_char thêm char, không trigger process

State 4 — PROCESSING:
    '\n' received hoặc buffer full
    → prv_process() chạy synchronously trong receive task context
    → dispatch → handler → reset buffer → send prompt
    → return to IDLE
```

**Single-threaded processing:** Không có concurrent processing. `shell_receive_task` handle cả input accumulation lẫn command processing trong cùng 1 task.

---

## Concurrency Model

| Component | Thread/Context | Note |
|---|---|---|
| `shell_receive_task` | FreeRTOS task, priority `configMAX_PRIORITIES - 1` (cao nhất) | RX poll + command processing |
| USB RX ISR → `usb_rx_queue` | ISR context | CDC receive callback enqueue byte |
| `cli_shell_printf` / `cli_buff[1024]` | `shell_receive_task` | Static buffer — safe vì single task |
| `log_func` / `buff[2048]` | Bất kỳ task nào gọi `log_*` | **KHÔNG thread-safe** — `FREE_RTOS=0` nên không có mutex |
| `app_setting_save()` từ command handler | `shell_receive_task` | Write Flash — `__disable_irq()` bên trong |
| `zigbee_connect()`, `zigbee_read_module()`, v.v. | `shell_receive_task` | Chỉ enqueue event vào Zigbee queue — không block |
| Zigbee callbacks (`zigbee_connect_cb`, v.v.) | Zigbee poll context (mainApp task) | Gọi `cli_shell_put_line(shell, ...)` → `bsp_usb_transmit` → `vTaskDelay(10)` trong mainApp task context |

**Race condition tiềm ẩn:**
- `cli_shell_put_line(shell, ...)` được gọi từ **2 task khác nhau**:
  - `shell_receive_task` (trực tiếp từ handler)
  - `mainApp task` (từ Zigbee/LoRa callbacks sau khi command được enqueue)
- Cả hai đều gọi `shell_send_str()` → `bsp_usb_transmit()` → `vTaskDelay(10)`
- `ShellContext_t shell` là shared global — không có mutex bảo vệ

**Logger race condition:**
- `log_func()` dùng `static char buff[2048]` không có mutex
- Nếu cả `shell_receive_task` và `mainApp task` đều gọi `log_*` đồng thời → buffer corruption
- `FREE_RTOS = 0` nên mutex code bị compile out

---

## Dependency Graph

```
cli_shell.c / cli_shell.h
    ← string.h, stddef.h, stdbool.h, stdarg.h, stdio.h
    ← g_shell_commands, g_num_shell_commands (extern, defined in cli_shell_command.c)

cli_shell_command.c
    ← cli_shell.h
    ← board.h                  (bsp_output_on/off, bsp_get_input, bsp_restart, bsp_delay)
    ← app_settings.h           (APP_Settings_t app_setting, app_setting_save)
    ← rf_app.h                 (extern ZigbeeMesh_t zigbee, extern LoRaMesh_t lora)
    ← zigbee.h                 (zigbee_connect, zigbee_read_module, zigbee_write_module, ...)
    ← lora.h [#if LORA_ENABLE] (lora_connect, lora_read_module, ...)
    ← logger.h                 (log_info, log_error)
    ← app_config.h             (ZIGBEE_ENABLE, LORA_ENABLE, APPLICATION_MODE_GPIO)
    ← string.h, stdio.h, stddef.h

shell_app.c
    ← cli_shell.h
    ← board.h                  (bsp_usb_transmit, bsp_usb_receiver, bsp_usb_available, bsp_com_read)
    ← FreeRTOS.h, task.h, queue.h

logger.c / logger.h
    ← stdio.h, stdarg.h, string.h
    ← FreeRTOS.h, semphr.h [#if FREE_RTOS — hiện disabled]
    ← printf → newlib → bsp_com_write via syscalls (gián tiếp)

Hardware dependencies của Shell:
    USB CDC channel 0     ← shell_app.c (TX/RX)
    LPUART1 (hlpuart1)    ← logger.c output
    Flash (sector 14)     ← app_setting_save() từ config commands
```

---

## Hidden Assumptions

| # | Assumption | Vị trí | Rủi ro khi port |
|---|---|---|---|
| 1 | `shell_app_poll()` bị comment out trong `app.c` | `app.c` | UART input path không hoạt động — chỉ USB hoạt động |
| 2 | `cli_buff[1024]` static không reentrant | `cli_shell.c` | Nếu `cli_shell_printf` được gọi từ 2 task → corruption. Hiện tại an toàn vì chỉ 1 task dùng |
| 3 | `vTaskDelay(10)` trong mỗi TX call | `shell_app.c` | Mỗi `send_str()` block 10ms. Với response dài → tổng delay hàng trăm ms. Có thể gây watchdog concern |
| 4 | Logger output và shell output đi 2 interface khác nhau | Design | Developer phải monitor cả LPUART1 và USB để xem đầy đủ thông tin |
| 5 | `FREE_RTOS = 0` trong logger.h | `logger.h` | Mutex bị compile out. Log từ nhiều task → buffer corruption |
| 6 | `SHELL_PROMPT = ""` | `cli_shell.h` | Shell không in prompt — không có visual indicator chờ input |
| 7 | Echo bị disabled | `cli_shell.c` | User không thấy ký tự đang gõ phản hồi lại — khó dùng với terminal thủ công |
| 8 | Zigbee/LoRa callbacks gọi `cli_shell_put_line` từ mainApp task | `cli_shell_command.c` | `shell` global bị access từ 2 task không có sync |
| 9 | `vsprintf` không giới hạn output | `cli_shell.c` `cli_buff[1024]` | Nếu format string tạo ra > 1024 bytes → stack/buffer overflow |
| 10 | Không check USB connect state | `shell_app.c` | TX khi USB chưa enumerate → error bị silently dropped |
| 11 | `cli_zigbee -w -sc` đọc `argv[i+2..i+4]` mà không check bounds | `cli_shell_command.c` | Nếu user nhập thiếu args → out-of-bounds access trên argv[] |

---

## Design Decisions

| Decision | Thực tế | Rationale |
|---|---|---|
| Static compile-time command table | `s_shell_commands[]` constant array | Đơn giản, zero overhead, no heap |
| Extern split: table defined in `command.c`, consumed in `cli_shell.c` | `g_shell_commands` extern | Tách shell engine khỏi application commands — engine có thể reuse |
| In-place tokenization | `rx_buffer` mutated trực tiếp | Không cần copy buffer, zero allocation |
| Driver injection (`sCliShellImpl_t`) | `send_char` / `send_str` function pointers | Shell engine portable — swap USB với UART chỉ bằng đổi impl |
| FreeRTOS task riêng cho RX | `shell_receive_task` priority MAX-1 | Đảm bảo responsive input ngay cả khi main task busy |
| Output qua JSON-like strings | `{"cmd":"...", "result":"..."}` | Machine-readable responses cho automation/testing |
| Logger và shell output tách biệt | LPUART1 vs USB CDC | Developer monitor LPUART1, client/app dùng USB CDC |
| `vTaskDelay(10)` trong TX | Sau mỗi `bsp_usb_transmit` | Tránh USB buffer overflow, cho phép USB stack process |

---

## Risks

| Risk | Severity | Mô tả |
|---|---|---|
| Shared `shell` global từ 2 tasks không có mutex | HIGH | `shell_receive_task` và `mainApp task` đều gọi `cli_shell_put_line(shell, ...)` — Zigbee/LoRa callbacks chạy trong mainApp context |
| Logger `buff[2048]` không thread-safe | HIGH | `FREE_RTOS=0` → mutex disabled. Multi-task log → corrupt output |
| `vsprintf` unbounded vào `cli_buff[1024]` | MEDIUM | Format string lớn → buffer overflow |
| `argv[]` out-of-bounds trong `-sc` handler | MEDIUM | `cli_zigbee -w -sc` đọc `argv[i+2..i+4]` không check bounds |
| `vTaskDelay(10)` trong TX path | MEDIUM | 10ms per call → chậm, block task, có thể gây watchdog nếu response dài |
| USB transmit không check connection | LOW | Nếu USB disconnect giữa chừng → TX error bị ignore |
| Echo disabled, prompt empty | LOW | Khó debug tay qua terminal; cần terminal app tự echo |
| `shell_app_poll()` disabled | LOW | UART input path chết — nếu V3 không có USB, cần re-enable |

---

## Porting Notes For RF_IO_RS485_V3

### Reusable Components

| Component | File | Ghi chú |
|---|---|---|
| Shell engine hoàn toàn | `services/shell/cli_shell.c/.h` | Portable 100%. Không phụ thuộc HAL, không phụ thuộc FreeRTOS. Chỉ cần cung cấp `sCliShellImpl_t` |
| Command struct definition | `cli_shell.h` | Copy nguyên |
| Tokenizer logic | `cli_shell.c` `prv_process()` | Copy nguyên |
| Command dispatch | `cli_shell.c` `prv_find_command()` | Copy nguyên |
| Output primitives | `cli_shell.c` `cli_shell_put_line`, `cli_shell_printf` | Copy nguyên |
| Logger API | `services/logger.h` | Interface portable — chỉ cần rebind `p_log_func` |
| Command table registration pattern | `cli_shell_command.c` bottom | `g_shell_commands` + `g_num_shell_commands` extern pattern — copy nguyên |

### Components Requiring Adaptation

| Component | File | Thay đổi cần thiết |
|---|---|---|
| `sCliShellImpl_t` binding | `shell_app.c` | Bind `send_char` / `send_str` vào I/O interface của V3 (USB, UART, hoặc cả hai) |
| `shell_receive_task` | `shell_app.c` | Adapt input source: nếu V3 dùng UART → đổi `bsp_usb_*` thành `bsp_com_*`. Nếu V3 có cả UART + USB → cần mux |
| Logger output binding | `bsp_init()` trong `board.c` | `logger_init(level, log_puts)` → `log_puts` phải point đến I/O đúng của V3 |
| Command handlers liên quan GPIO | `cli_shell_command.c` | `cli_set_output`, `cli_get_input` → adapt BSP API của V3 |
| `config` command | `cli_shell_command.c` | Slave address, Flash sector address có thể thay đổi trên V3 |
| `app_setting_save()` / `app_setting_load()` | `app_settings.c` | Flash sector/bank address phụ thuộc hardware V3 |

### Components Requiring Rewrite

| Component | Lý do |
|---|---|
| `shell_app.c` `shell_send_str` TX delay | `vTaskDelay(10)` per call quá chậm. V3 nên dùng DMA TX hoặc non-blocking queue + notification |
| Logger mutex | Enable `FREE_RTOS = 1` và cung cấp mutex thực sự, hoặc route tất cả log qua task-safe queue |
| `cli_shell_printf` buffer | Thay `vsprintf(cli_buff[1024], ...)` bằng `vsnprintf` với size guard để tránh overflow |
| Zigbee/LoRa callback safety | Callbacks phải gọi shell output thread-safe. Options: (a) mutex bảo vệ shell TX, (b) callback post message vào queue để shell task print, (c) callback chỉ set flag, shell task poll |
| `shell_app_poll()` re-enable | Nếu V3 không có USB, enable UART input path và disable USB path |

### Checklist Porting

```
□ Copy cli_shell.h + cli_shell.c nguyên vẹn
□ Implement sCliShellImpl_t với I/O phù hợp V3
□ Tạo shell_receive_task hoặc polling loop với input source của V3
□ Copy cli_shell_command.c, remove lora/zigbee/output/input nếu không áp dụng
□ Update rf_app.h extern references nếu Zigbee/LoRa interface thay đổi
□ Fix vTaskDelay(10) trong TX — dùng non-blocking approach
□ Enable FREE_RTOS=1 trong logger.h và test mutex
□ Thay vsprintf → vsnprintf với size limit
□ Verify app_setting Flash address đúng cho V3 hardware
□ Test: gõ "help" → toàn bộ command list hiện ra
□ Test: "zigbee -c" → response đúng từ Zigbee callback trong mainApp context (race condition check)
□ Test: concurrent log output từ 2 task không bị corrupt
```

---

*Tài liệu này bổ sung KB_v2.md — không thay thế.*