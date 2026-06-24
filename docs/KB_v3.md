PHÂN TÍCH REPOSITORY: rs485v3 (RF_IO_V3)
1. MỤC ĐÍCH DỰ ÁN
Đây là firmware cho thiết bị SynaptiX Industrial IO Remote - một gateway công nghiệp chạy trên STM32H5, kết nối thế giới công nghiệp (RS485/Modbus, DI/DO 24V, Zigbee, LoRa) với thế giới IT (USB Ethernet, Web Server, REST API). Tên chính thức trong build system là RF_IO_V3, version 3.0.0.
Tên thực sự của repo gốc là SynaptiXCompany/RF_IO_RS485_V3 - repo hiện tại (logan123synaptix/rs485v3) là bản fork/copy cá nhân.

2. KIẾN TRÚC TỔNG THỂ
Hardware Platform

MCU: STM32H523CCUx - gói UFQFPN48 (48-pin QFN)
Core: ARM Cortex-M33 (không TrustZone - TrustZoneDisabled trong .ioc)
Flash: 256KB (linker script: LENGTH = 256K, bắt đầu từ 0x08000000)
RAM: 272KB (bắt đầu từ 0x20000000)
Clock: SYSCLK = 32MHz (HSE = 25MHz ngoài), USB = 48MHz (từ HSI48)
Firmware Package: STM32Cube FW_H5 V1.6.0

Peripheral Mapping (từ .ioc)
LPUART1  : PA9(TX), PA10(RX)  -> Console/Shell @ 115200 baud
USART1   : PB14(TX), PB15(RX) -> RS485 port
USART2   : PA2(TX), PA3(RX)   -> Second serial (Zigbee hoặc LoRa)
USB      : PA11(DM), PA12(DP) -> USB Device (TinyUSB)

GPIO Output: PA6, PA7, PB0, PB1, PB13, PC13, PC14, PC15
GPIO Input : PB3, PB4, PB5, PB6

Debug: PA13(SWDIO), PA14(SWCLK), PB3(SWO)
Timer: TIM6 -> HAL timebase (thay vì SysTick, nhường SysTick cho FreeRTOS)
ICACHE: Enabled (Direct Mapped Cache)
Ước đoán GPIO mapping:

PA6, PA7, PB0, PB1 -> Digital Outputs DO1~DO4 (hoặc DO + RS485 DE/RE)
PB3, PB4, PB5, PB6 -> Digital Inputs DI1~DI4
PB13 -> RS485 DE (Direction Enable)
PC13, PC14, PC15 -> LEDs hoặc control signals


3. KIẾN TRÚC PHẦN MỀM
Software Stack (từ trên xuống dưới)
┌─────────────────────────────────────────────────────────┐
│                   Web Browser / PC                       │
│              http://192.168.7.1                          │
└──────────────────────┬──────────────────────────────────┘
                       │ HTTP/REST
┌──────────────────────▼──────────────────────────────────┐
│              Mongoose Web Server                         │
│         (mongoose/ - embedded HTTP server)               │
│   Dashboard | DI/DO | Modbus | Wireless | Network        │
└──────────────────────┬──────────────────────────────────┘
                       │ Network Stack (TCP/IP over USB)
┌──────────────────────▼──────────────────────────────────┐
│                 usb_net layer                            │
│    ECM (Linux/macOS) + RNDIS (Windows) Network Adapters  │
└──────────────────────┬──────────────────────────────────┘
                       │ USB Device Stack
┌──────────────────────▼──────────────────────────────────┐
│                  TinyUSB Stack                           │
│       CDC ACM (VCP) + CDC ECM + RNDIS                   │
│        (Middlewares/Third_Party/tinyusb)                 │
└──────────────────────┬──────────────────────────────────┘
                       │ HAL USB Driver
┌──────────────────────▼──────────────────────────────────┐
│              STM32H5 USB DRD FS Hardware                 │
│                  PA11/PA12 (D-, D+)                      │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│           Application Services Layer (libs/)             │
├──────────────┬──────────────┬──────────────┬────────────┤
│ Modbus RTU   │  DI/DO Svc   │ Zigbee/LoRa  │   Shell    │
│ (USART1)     │  (GPIO)      │ (USART2)     │  (LPUART1) │
└──────────────┴──────────────┴──────────────┴────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│                 FreeRTOS Kernel                          │
│   (STMicroelectronics.X-CUBE-FREERTOS 1.5.0)            │
│   Heap4, CMSIS-RTOS2, Total Heap = 131077 bytes         │
│   Minimal Stack = 256 words                             │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│              STM32 HAL / LL Drivers                      │
│         (Drivers/ - STM32Cube FW_H5 V1.6.0)             │
└──────────────────────┬──────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────┐
│            STM32H523CCUx Hardware                        │
│    FLASH:256KB @0x08000000 | RAM:272KB @0x20000000       │
└─────────────────────────────────────────────────────────┘

4. CẤU TRÚC THƯ MỤC THỰC TẾ
rs485v3/
├── CMakeLists.txt              # Root build - links 4 targets
├── CMakePresets.json           # Preset debug/release
├── RF_IO_V3.ioc                # STM32CubeMX pin config
├── STM32H523xx_FLASH.ld        # Linker script (FLASH boot)
├── STM32H523xx_RAM.ld          # Linker script (RAM boot - debug)
├── startup_stm32h523xx.s       # ASM startup, vector table
├── flash.sh                    # Flash script (OpenOCD/STLink)
├── Build.md                    # Build guide
├── Readme.md                   # Project overview
│
├── Core/                       # STM32CubeMX generated HAL init
│   ├── Src/
│   │   ├── main.c             # Entry point - HAL init + RTOS start
│   │   ├── stm32h5xx_it.c     # Interrupt handlers
│   │   ├── syscalls.c         # Newlib syscall stubs
│   │   └── sysmem.c           # Heap memory management
│   └── Inc/
│       ├── main.h             # Pin defines, extern declarations
│       └── stm32h5xx_hal_conf.h
│
├── Drivers/                    # STM32 HAL + BSP drivers
│   ├── STM32H5xx_HAL_Driver/  # HAL library source
│   └── CMSIS/                 # ARM CMSIS headers
│
├── Middlewares/
│   └── Third_Party/           # TinyUSB, FreeRTOS
│
├── libs/                       # Application code (CUSTOM)
│   └── CMakeLists.txt         # App services: Modbus, DIO, Shell...
│
├── usb_net/                    # USB networking layer (CUSTOM)
│   └── CMakeLists.txt         # ECM + RNDIS + TinyUSB integration
│
├── mongoose/                   # Mongoose web server (CUSTOM)
│   └── CMakeLists.txt         # Web server + REST API + UI
│
├── cmake/
│   └── stm32cubemx/           # STM32CubeMX CMake integration
│
├── lwip/Target/                # lwIP config (if used as network stack)
├── usb/Target/                 # USB HAL target config
├── Target/                     # Board-specific config
└── .vscode/                    # VS Code + Cortex-Debug config

5. MODULE CHÍNH
5.1 CMake Build Graph
RF_IO_V3 (executable)
    ├── stm32cubemx    (cmake/stm32cubemx/) - HAL, startup, Core
    ├── libs           (libs/)              - App logic
    ├── usb_net        (usb_net/)           - USB networking
    └── mongoose       (mongoose/)          - Web server
5.2 libs/ - Application Services
Dựa trên README và Build.md, module này chứa:

modbus/ - Modbus RTU Master/Slave trên USART1 + RS485 DE control (PB13)
dio/ - Digital I/O: đọc DI1~4 (PB3~6), điều khiển DO1~4 (GPIO Out)
shell/ - CLI qua LPUART1 (115200) và USB CDC ACM
zigbee/ - Driver giao tiếp module Zigbee qua USART2
lora/ - Driver giao tiếp module LoRa qua USART2
network/ - Network configuration management (IP, GW, DNS)
webserver/ - HTTP handler callbacks cho Mongoose

5.3 usb_net/ - USB Networking
Composite USB device gồm 3 interface:

CDC ACM - Virtual COM Port (dành cho Shell)
CDC ECM - Ethernet cho Linux/macOS
RNDIS - Ethernet cho Windows

IP tĩnh: 192.168.7.1/24
5.4 mongoose/ - Web Server
Event-driven HTTP server, tích hợp với lwIP hoặc trực tiếp qua USB network. Phục vụ:

Static Web UI (HTML/JS/CSS)
REST API endpoints (JSON)
Real-time DI/DO monitoring


6. ENTRY POINT & EXECUTION FLOW
Boot Sequence
Reset_Handler (startup_stm32h523xx.s)
    │
    ├── Copy .data từ FLASH → RAM
    ├── Zero .bss
    ├── Gọi __libc_init_array() (C++ constructors)
    └── Nhảy vào main()
            │
            ├── HAL_Init()                     # Khởi tạo HAL tick (TIM6)
            ├── SystemClock_Config()            # PLL: HSE 25MHz → SYSCLK 32MHz
            ├── MX_GPIO_Init()                  # Cấu hình tất cả GPIO
            ├── MX_ICACHE_Init()               # Bật Instruction Cache
            ├── MX_LPUART1_UART_Init()         # Shell UART (115200)
            ├── MX_USART1_UART_Init()          # RS485
            ├── MX_USART2_UART_Init()          # Zigbee/LoRa
            ├── MX_USB_PCD_Init()              # USB hardware
            ├── MX_CORTEX_M33_NS_Init()        # MPU setup
            ├── MX_PWR_Init()                  # Power optimization
            │
            └── osKernelStart()                # FreeRTOS start
                    │
                    └── StartDefaultTask()      # Default task entry
                            │
                            ├── USB stack init (TinyUSB)
                            ├── Network init (ECM/RNDIS)
                            ├── Mongoose init
                            ├── Modbus task spawn
                            ├── DIO task spawn
                            ├── Shell task spawn
                            └── Wireless task spawn
FreeRTOS Task Architecture
┌────────────────────────────────────────────────────────┐
│                   FreeRTOS Tasks                        │
├────────────────┬────────────────┬───────────────────────┤
│ defaultTask    │  modbusTask    │  dioTask              │
│ (init +        │  (USART1 RTU)  │  (GPIO poll/IRQ)     │
│  main loop)    │                │                       │
├────────────────┼────────────────┼───────────────────────┤
│ shellTask      │ wirelessTask   │  usbTask              │
│ (LPUART1 +     │ (USART2:       │  (TinyUSB device_task)│
│  CDC ACM)      │  Zigbee/LoRa)  │                       │
├────────────────┼────────────────┼───────────────────────┤
│ mongooseTask   │                │                       │
│ (HTTP server   │                │                       │
│  poll loop)    │                │                       │
└────────────────┴────────────────┴───────────────────────┘

7. DATA FLOW
Modbus RTU Flow
PLC/Master ──RS485──► USART1 RX IRQ ──► Rx Buffer ──► Modbus Parser
                                                             │
                                                    Process Request
                                                             │
                                                    Build Response
                                                             │
RS485 DE assert (PB13 HIGH) ──► USART1 TX ──► PLC/Master
RS485 DE deassert (PB13 LOW) [after TX complete]
Digital I/O Flow
External 24V Signal ──► Opto-isolator ──► GPIO PB3~PB6
                                               │
                                         DIO Service poll/IRQ
                                               │
                                       State Buffer
                                               │
                              ┌────────────────┴────────────┐
                              │                              │
                         Web API (GET)               Modbus Registers
                         /api/dio                    Holding/Input Regs
USB Networking Flow
Host PC
  │
  USB (PA11/PA12)
  │
  TinyUSB Stack
  ├── CDC ACM ──────────────────────────► Shell Service
  └── ECM/RNDIS ──► Network Stack ──► Mongoose ──► REST API / Web UI
                         │
                    IP: 192.168.7.1

8. DEPENDENCY GIỮA CÁC MODULE
libs (app services)
  ├── depends on: Drivers (HAL UART, HAL GPIO, HAL TIM)
  ├── depends on: FreeRTOS (tasks, queues, semaphores)
  └── provides API to: mongoose (web handlers), usb_net (shell)

usb_net
  ├── depends on: Drivers (HAL USB PCD)
  ├── depends on: TinyUSB (Middlewares/Third_Party)
  └── provides: network interface to mongoose

mongoose
  ├── depends on: usb_net (network I/O)
  └── depends on: libs (data read/write callbacks)

stm32cubemx
  ├── provides: HAL init functions (MX_*)
  └── is base dependency for everything

9. INTERFACE VỚI HỆ THỐNG BÊN NGOÀI
InterfacePhysicalProtocolDirectionRS485USART1 + PB13(DE)Modbus RTUMaster & SlaveZigbeeUSART2AT/BinaryMasterLoRaUSART2AT/BinaryMasterDI1~4PB3~PB6 (Opto)GPIOInputDO1~4PA6,PA7,PB0,PB1GPIOOutputUSB HostPA11/PA12USB 2.0 FSDeviceWeb UIUSB ECM/RNDISHTTP/RESTServerConsoleLPUART1 PA9/PA10UART 115200BidirectionalDebugPA13/PA14SWDIn

10. CODING PATTERN & DESIGN PATTERN
Coding Conventions

Ngôn ngữ: C11 (CMAKE_C_STANDARD 11), chiếm 91% codebase
Naming: STM32 HAL convention - snake_case, prefix theo module (MX_, HAL_, osTask)
HAL timebase: Dùng TIM6 thay vì SysTick (SysTick nhường cho FreeRTOS)
Build system: CMake + Ninja, không dùng Makefile hay STM32CubeIDE project file
Toolchain: ARM GNU GCC 13+, cross-compile arm-none-eabi-gcc
Compiler flags: C11, extensions ON, -O2 for Release, -flto, gc-sections

Design Patterns

Event-driven: Mongoose sử dụng event loop pattern (poll-based)
Task-per-service: Mỗi service (Modbus, DIO, Shell, Wireless) chạy như FreeRTOS task riêng biệt
Layered architecture: Hardware → HAL → RTOS → Middleware → App → Web
HAL abstraction: Tất cả hardware access qua STM32 HAL, không trực tiếp register
CMSIS-RTOS2 API: Dùng osTask*, osSemaphore*, osQueue* thay vì FreeRTOS native API trực tiếp (nhờ X-CUBE-FREERTOS)
Heap4: FreeRTOS heap4 (best-fit, không fragmentation-resistant như heap5)

Quy tắc ngầm của dự án

Không sửa Core/: Đây là generated code từ STM32CubeMX. Chỉ thêm code vào vùng USER CODE BEGIN/END
Không sửa Drivers/: HAL library nguyên vẹn từ ST
Custom code đặt trong libs/, usb_net/, mongoose/: Đây là vùng developer được phép sửa
Mỗi thư mục con có CMakeLists.txt riêng: Thêm tính năng = thêm file vào CMakeLists của module tương ứng
TIM6 là HAL timebase: Không dùng TIM6 cho mục đích khác
USB IP tĩnh 192.168.7.1: Fixed trong usb_net config
LPUART1 là debug console: Không dùng cho RS485 hay wireless
NVIC priority group 4: 4 bits preemption, 0 bits subpriority. USART/USB IRQ priority = 5


PROJECT KNOWLEDGE BASE
Đây là nguồn sự thật duy nhất cho toàn bộ dự án RF_IO_V3. Mọi code mới phải tương thích với nội dung sau.

KB-1: IDENTITY
Thuộc tínhGiá trịTên dự ánRF_IO_V3 (SynaptiX Industrial IO Remote)Version3.0.0MCUSTM32H523CCUx (UFQFPN48)CoreARM Cortex-M33 (no TrustZone)FLASH256KB @ 0x08000000RAM272KB @ 0x20000000SYSCLK32 MHz (PLL từ HSE 25MHz)USB clock48 MHz (HSI48)HAL PackageSTM32Cube FW_H5 V1.6.0RTOSFreeRTOS (X-CUBE-FREERTOS 1.5.0), CMSIS-RTOS2, Heap4Heap131077 bytesStack (min)0x400 (1KB)Build systemCMake 3.22+, Ninja, arm-none-eabi-gcc 13+LanguageC11, extensions ON

KB-2: PERIPHERAL MAP
PeripheralPinsChức năngIRQ PriorityLPUART1PA9(TX), PA10(RX)Console Shell, 1152005USART1PB14(TX), PB15(RX)RS4855USART2PA2(TX), PA3(RX)Zigbee / LoRa5USB_DRDPA11(DM), PA12(DP)USB Device (TinyUSB)5TIM6-HAL Timebase (không dùng SysTick)15SysTick-FreeRTOS tick15PB13OutputRS485 DE (Direction Enable)-PB3,PB4,PB5,PB6InputDI1~4 (opto-isolated 24V)-PA6,PA7,PB0,PB1OutputDO1~4-PC13,PC14,PC15OutputStatus LEDs-PA13,PA14SWDDebug-PH0,PH1HSEExternal oscillator 25MHz-ICACHE-Direct Mapped Cache, ON-

KB-3: CMAKE BUILD STRUCTURE
add_executable(RF_IO_V3)
    target_link_libraries:
        stm32cubemx  ← cmake/stm32cubemx/CMakeLists.txt (HAL, Core, startup)
        libs         ← libs/CMakeLists.txt       (App: Modbus, DIO, Shell...)
        usb_net      ← usb_net/CMakeLists.txt    (TinyUSB, ECM, RNDIS)
        mongoose     ← mongoose/CMakeLists.txt   (HTTP server, Web UI)

Output: RF_IO_V3.elf, RF_IO_V3.bin, RF_IO_V3.hex
Flash addr: 0x08000000
Thêm code mới: Sửa CMakeLists.txt của module tương ứng, không sửa root CMakeLists.txt trừ khi thêm module cấp top.

KB-4: EXECUTION FLOW
startup_stm32h523xx.s → Reset_Handler
    → main() [Core/Src/main.c]
        → HAL_Init() [TIM6 timebase]
        → SystemClock_Config() [HSE 25MHz → SYSCLK 32MHz, USB 48MHz]
        → MX_GPIO_Init()
        → MX_ICACHE_Init()
        → MX_LPUART1_UART_Init()  [115200]
        → MX_USART1_UART_Init()   [RS485, baud config từ app]
        → MX_USART2_UART_Init()   [Zigbee/LoRa]
        → MX_USB_PCD_Init()
        → MX_CORTEX_M33_NS_Init() [MPU]
        → MX_PWR_Init()
        → osKernelStart()
            → StartDefaultTask()
                → [khởi tạo tất cả services]
                → [spawn tasks]
                → vTaskDelete(NULL) hoặc vào idle loop

KB-5: MODULE API (DỰ KIẾN)
Modbus Service (libs/modbus/)
cvoid modbus_init(UART_HandleTypeDef *huart, GPIO_TypeDef *de_port, uint16_t de_pin);
void modbus_task(void *arg);  // FreeRTOS task
int  modbus_read_registers(uint8_t addr, uint16_t reg, uint16_t count, uint16_t *out);
int  modbus_write_register(uint8_t addr, uint16_t reg, uint16_t value);
DIO Service (libs/dio/)
cvoid     dio_init(void);
uint8_t  dio_read_inputs(void);   // trả về bitmask DI1~4
void     dio_set_output(uint8_t ch, uint8_t state);  // ch: 0~3
uint8_t  dio_read_outputs(void);  // đọc trạng thái DO hiện tại
Shell Service (libs/shell/)
cvoid shell_init(UART_HandleTypeDef *huart);
void shell_task(void *arg);
void shell_register_cmd(const char *name, shell_cmd_fn fn);
Mongoose Web Service (mongoose/)
cvoid mongoose_init(void);
void mongoose_task(void *arg);
// REST API endpoints:
// GET  /api/dio      → JSON DI/DO states
// POST /api/do       → set DO
// GET  /api/modbus   → Modbus status
// GET  /api/network  → Network config
// POST /api/network  → Set network config
// GET  /api/info     → Firmware info
USB Net (usb_net/)
cvoid usb_net_init(void);
// TinyUSB device task tự động chạy
// IP: 192.168.7.1
// Netmask: 255.255.255.0

KB-6: LINKER MEMORY MAP
FLASH (rx) : 0x08000000 - 0x0803FFFF  (256KB)
    .isr_vector
    .text
    .rodata
    .ARM.extab
    .ARM (exidx)
    .preinit_array / .init_array / .fini_array
    _sidata (LMA of .data)

RAM (xrw)  : 0x20000000 - 0x20043FFF  (272KB)
    .data  (copied from FLASH at boot)
    .tdata
    .tbss / .bss
    ._user_heap_stack (heap: 0x200, stack: 0x400)

Stack top (_estack) : 0x20044000
Heap FreeRTOS: 131077 bytes (riêng biệt, trong .bss, quản lý bởi heap4.c)

KB-7: NETWORK CONFIG
Tham sốGiá trịDevice IP192.168.7.1Netmask255.255.255.0Host IP (USB)192.168.7.2 (auto-assigned)Web server port80USB PID/VIDCần đọc thêm từ TinyUSB configUSB ClassesCDC ACM + CDC ECM + RNDIS (composite)

KB-8: MODBUS PROTOCOL
Function CodeMô tả0x01Read Coils0x02Read Discrete Inputs0x03Read Holding Registers0x04Read Input Registers0x05Write Single Coil0x06Write Single Register0x0FWrite Multiple Coils0x10Write Multiple Registers
RS485 DE: PB13, HIGH khi transmit, LOW khi receive.

KB-9: SHELL COMMANDS
> help
system info        - thông tin firmware/hardware
network status     - trạng thái mạng
modbus status      - trạng thái Modbus RTU
lora status        - trạng thái LoRa module
zigbee status      - trạng thái Zigbee module
di read            - đọc Digital Inputs (DI1~4)
do set <ch> <0|1>  - điều khiển Digital Output
reboot             - khởi động lại thiết bị
factory-reset      - reset về cài đặt gốc

KB-10: QUYẾT ĐỊNH THIẾT KẾ CHÍNH

STM32H523 thay vì H743/F4: Chọn H5 vì có TrustZone (dù hiện tại disabled), hardware crypto, và Cortex-M33 hiệu năng cao trong gói 48-pin nhỏ gọn - phù hợp thiết bị công nghiệp compact.
TinyUSB thay vì STM32 USB middleware: TinyUSB dễ cấu hình composite device (ACM + ECM + RNDIS trong cùng một device), tài liệu tốt hơn.
Mongoose thay vì lwIP httpd: Mongoose là single-file, event-driven, dễ thêm REST API và WebSocket về sau (trong Future Enhancements có MQTT, HTTPS, OTA).
TIM6 là HAL timebase: Giải phóng SysTick để FreeRTOS dùng, đây là best practice cho STM32 + FreeRTOS.
USB networking thay vì Ethernet PHY: Giảm chi phí BOM (không cần Ethernet PHY chip), kết nối trực tiếp qua USB với PC quản lý.
Heap4: Cho tốc độ, chấp nhận không có memory protection. Nếu cần heap5 (nhiều memory region) có thể nâng cấp.
CMSIS-RTOS2 wrapper: Tạo portability nếu sau này chuyển RTOS khác (Zephyr, ThreadX).
Project version 3.0.0: Đây là V3 của sản phẩm, ngụ ý có V1, V2 trước đó - cần cẩn thận khi merge code từ version cũ.


KB-11: THÔNG TIN CÒN THIẾU (cần đọc thêm)
Các file chưa đọc được do GitHub robots.txt block directory listing:

libs/CMakeLists.txt - cần biết chính xác source files trong libs
usb_net/CMakeLists.txt - TinyUSB config (descriptors, PID/VID)
mongoose/CMakeLists.txt - REST API endpoint implementation
Core/Src/main.c - xác nhận task creation code
Core/Inc/main.h - xác nhận GPIO pin defines

Khi cần code liên quan đến các module này, hãy yêu cầu paste nội dung file tương ứng thay vì suy đoán.

KB-12: HƯỚNG DẪN TÍCH HỢP TÍNH NĂNG MỚI
Thêm tính năng mới vào dự án theo quy trình:

Thêm service mới (ví dụ: MQTT) → tạo thư mục libs/mqtt/, thêm vào libs/CMakeLists.txt
Thêm REST API endpoint → sửa handler trong mongoose/, đăng ký route trong mongoose init
Thêm Shell command → gọi shell_register_cmd() trong init của service mới
Thêm peripheral mới → mở RF_IO_V3.ioc bằng STM32CubeMX, generate code, commit diff trong Core/
Sửa network config → tìm trong usb_net/ file cấu hình IP
Thêm Modbus register → sửa register map trong libs/modbus/, cập nhật REST API GET /api/modbus tương ứng