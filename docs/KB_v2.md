PHÂN TÍCH REPOSITORY: logan123synaptix/rs485v2
1. Tổng quan & Mục đích
Đây là firmware embedded cho một board tùy chỉnh tên RS485_IO_RF_V2 của công ty/nhóm SynaptiX. Board này đóng vai trò gateway/IO module với khả năng:

Giao tiếp RS485 (USART1 - half-duplex với DE pin điều khiển)
Giao tiếp serial thứ hai (USART2 - có DMA, vai trò debug hoặc RF/LoRa/Zigbee)
Giao tiếp LPUART1 (có thể thêm thiết bị ngoại vi tốc độ thấp)
USB CDC (3 virtual COM port composite)
Digital IO: 4 đầu vào (IN0-IN3), 4 đầu ra (OUT0-OUT3)
SPI4 (2 Mbps - giao tiếp module LoRa)
USART2 (giao tiếp module Zigbee — UART, không phải SPI)
Modbus RTU/ASCII stack
FreeRTOS làm RTOS
IWDG watchdog

MCU: STM32H523CCU6 (Cortex-M33, 250 MHz, package UFQFPN48, 256KB Flash, 272KB RAM)

2. Cấu trúc thư mục
rs485v2/
├── Core/
│   ├── Inc/                    # Header files HAL + user
│   │   └── main.h, gpio.h, usart.h, tim.h, usb.h, spi.h, gpdma.h, ...
│   └── Src/                    # Source files chính
│       ├── main.c              # Entry point: init + khởi động FreeRTOS
│       ├── app_freertos.c      # Định nghĩa tasks FreeRTOS
│       ├── gpio.c              # Init GPIO (IN0-IN3, OUT0-OUT3, LED0-LED2)
│       ├── usart.c             # Init USART1 (RS485), USART2 (DMA), LPUART1
│       ├── tim.c               # Init TIM1-TIM5 (prescaler=250-1, period=1000-1 → 1kHz)
│       ├── usb.c               # Init USB PCD
│       ├── spi.c               # Init SPI4 (2 Mbps, Master, Full-duplex)
│       ├── gpdma.c             # Init GPDMA (USART2 RX/TX DMA)
│       ├── icache.c            # Init I-Cache
│       ├── iwdg.c              # Init IWDG (prescaler 64 → ~2s timeout)
│       ├── stm32h5xx_it.c      # IRQ handlers
│       ├── stm32h5xx_hal_msp.c # HAL MSP callbacks
│       ├── stm32h5xx_hal_timebase_tim.c  # TIM6 → HAL timebase
│       ├── system_stm32h5xx.c  # System clock init
│       ├── sysmem.c / syscalls.c # newlib stubs
│       └── ...
├── Drivers/
│   ├── STM32H5xx_HAL_Driver/   # STM32 HAL (Cube FW_H5 V1.5.1)
│   └── CMSIS/                  # CMSIS core + device headers
├── Middlewares/Third_Party/
│   ├── FreeRTOS/               # FreeRTOS 10.6.2, port ARM_CM33_NTZ, heap_4
│   │   └── Source/CMSIS_RTOS_V2/  # CMSIS-RTOS2 wrapper
│   └── AL94_USB_Composite/     # USB Composite (3× CDC ACM)
│       └── COMPOSITE/
│           ├── App/            # usb_device.c, usbd_desc.c, usbd_cdc_acm_if.c
│           ├── Class/CDC_ACM/  # CDC class driver
│           ├── Class/COMPOSITE/# Composite handler
│           ├── Core/           # USB Core (usbd_core, usbd_ctlreq, usbd_ioreq)
│           └── Target/         # usbd_conf.c (HW binding)
├── SynaptiX/                   # Business logic của SynaptiX (module chính)
│   ├── synaptix.mk             # Sub-makefile: khai báo DEVICE_SOURCE, MODBUS_SOURCE
│   ├── apps/                   # Application layer
│   ├── board/                  # Board abstraction layer
│   ├── services/
│   │   ├── mbport/             # Modbus port abstraction
│   │   ├── shell/              # Debug shell
│   │   ├── lora/               # LoRa service (qua SPI4)
│   │   ├── zigbee/             # Zigbee service (qua USART2 — KHÔNG phải SPI4)
│   │   └── Modbus/modbus/      # Modbus stack
│   │       ├── include/        # API headers
│   │       ├── ascii/          # Modbus ASCII impl
│   │       └── rtu/            # Modbus RTU impl
│   └── utils/                  # Utility functions
├── Composite/                  # Có thể chứa thêm USB composite config
├── Documents/                  # Tài liệu dự án
├── RS485_IO_RF_V2.ioc          # STM32CubeMX project file
├── Makefile                    # Build system (GCC ARM)
├── STM32H523xx_FLASH.ld        # Linker script (Flash boot)
├── STM32H523xx_RAM.ld          # Linker script (RAM boot/debug)
└── startup_stm32h523xx.s       # Startup assembly

3. Kiến trúc tổng thể (Text Diagram)
┌─────────────────────────────────────────────────────────────┐
│                    STM32H523CCU6 @ 250MHz                   │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              FreeRTOS (CMSIS-RTOS2)                  │   │
│  │   ┌──────────┐  ┌──────────┐  ┌──────────┐ ...      │   │
│  │   │ mainApp  │  │ ModbusTask│  │ RF Task  │          │   │
│  │   │  Task    │  │          │  │(LoRa/Zigb)│          │   │
│  │   └────┬─────┘  └────┬─────┘  └────┬─────┘          │   │
│  └────────┼─────────────┼─────────────┼──────────────────   │
│           │             │             │                  │   │
│  ┌────────▼─────────────▼─────────────▼──────────────┐  │   │
│  │              SynaptiX Services Layer               │  │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │  │   │
│  │  │  Modbus  │ │  Shell   │ │ LoRa/ZB  │           │  │   │
│  │  │  RTU/ASC │ │  Service │ │ Service  │           │  │   │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘           │  │   │
│  │       │             │             │                 │  │   │
│  │  ┌────▼─────────────▼─────────────▼──────────────┐ │  │   │
│  │  │           Board Abstraction Layer              │ │  │   │
│  │  └────┬──────────┬──────────┬──────────┬─────────┘ │  │   │
│  └───────┼──────────┼──────────┼──────────┼───────────┘  │   │
│          │          │          │          │               │   │
│  ┌───────▼──┐ ┌─────▼───┐ ┌───▼────┐ ┌───▼────┐         │   │
│  │ USART1   │ │ USART2  │ │  SPI4  │ │  USB   │         │   │
│  │ RS485    │ │ +DMA    │ │  2MHz  │ │ 3×CDC  │         │   │
│  │ DE=PB13  │ │LPUART1  │ │Master  │ │ ACM    │         │   │
│  └───────┬──┘ └─────┬───┘ └───┬────┘ └───┬────┘         │   │
│          │          │          │          │               │   │
│  ┌───────▼──┐ ┌─────▼───┐ ┌───▼────┐ ┌───▼────┐         │   │
│  │  GPIO    │ │  GPIO   │ │ GPIO   │ │ PA11/12│         │   │
│  │ IN0-IN3  │ │OUT0-OUT3│ │LED0-2  │ │USB D+/-│         │   │
│  └──────────┘ └─────────┘ └────────┘ └────────┘         │   │
└─────────────────────────────────────────────────────────────┘
                    │          │          │          │
               RS485 Bus   Serial/RF   SPI RF     USB to PC
              (Modbus)    (Debug/RF)  (LoRa/ZB)  (3 VCP)

4. Hardware Pin Mapping
PinLabelChức năngPA0SPI4_SCKSPI4 Clock (RF module)PA2USART2_TXSerial 2 TX (DMA)PA3USART2_RXSerial 2 RX (DMA)PA6OUT0Digital Output 0PA7OUT1Digital Output 1PA8SPI4_MOSISPI4 MOSIPA9LPUART1_TXLPUART TXPA10LPUART1_RXLPUART RXPA11USB_DMUSB D-PA12USB_DPUSB D+PB0OUT2Digital Output 2PB1OUT3Digital Output 3PB6IN0Digital Input 0PB5IN1Digital Input 1PB4IN2Digital Input 2PB3IN3Digital Input 3PB7SPI4_MISOSPI4 MISOPB13USART1_DERS485 Driver EnablePB14USART1_TXRS485 TXPB15USART1_RXRS485 RXPC13LED0LEDPC14LED1LEDPC15LED2LEDPH0/PH1HSE8 MHz crystal

5. Peripheral & Clock Config

SYSCLK: 250 MHz (HSE 8 MHz → PLL × 250 / 4 / 4 = 250 MHz)
HCLK/APBx: 250 MHz
SPI4: nguồn HSI = 32 MHz → prescaler 16 → 2 Mbps
USB: 48 MHz (HSI48 + CRS)
TIM1-TIM5: prescaler=249, period=999 → 1ms interrupt (1 kHz)
TIM6: HAL timebase source
IWDG: prescaler 64, LSI ~32kHz → timeout ≈ 2 giây
GPDMA: CH0 = USART2 RX, CH1 = USART2 TX
Heap FreeRTOS: 128 KB (heap_4)
Stack task: configMINIMAL = 256 words, mainApp task = 256 words


6. Execution Flow
Reset → startup_stm32h523xx.s
         ├── Init stack, BSS, data
         └── → main()
               ├── HAL_Init()
               ├── SystemClock_Config()       [250 MHz PLL]
               ├── MX_GPIO_Init()
               ├── MX_GPDMA1_Init()
               ├── MX_ICACHE_Init()
               ├── MX_IWDG_Init()
               ├── MX_LPUART1_UART_Init()
               ├── MX_USART1_UART_Init()      [RS485]
               ├── MX_USART2_UART_Init()      [+DMA]
               ├── MX_TIM1..5_Init()
               ├── MX_SPI4_Init()
               ├── MX_USB_PCD_Init()
               ├── MX_USB_DEVICE_Init()       [Composite 3×CDC]
               ├── MX_FREERTOS_Init()
               │   └── osThreadNew("mainApp", main_app, ...)
               └── osKernelStart()            [FreeRTOS scheduler bắt đầu]
                     └── main_app task
                           ├── SynaptiX app init
                           ├── Modbus init
                           ├── RF service init
                           └── main loop

7. Dependency Graph
main.c
  ├── FreeRTOS (CMSIS-RTOS2)
  │     └── port ARM_CM33_NTZ + heap_4
  ├── HAL (STM32H5xx_HAL_Driver)
  │     ├── hal_uart, hal_uart_ex
  │     ├── hal_spi, hal_spi_ex
  │     ├── hal_tim, hal_tim_ex
  │     ├── hal_dma, hal_dma_ex
  │     ├── hal_pcd, hal_pcd_ex + ll_usb
  │     ├── hal_gpio, hal_iwdg, hal_icache
  │     └── hal_rcc, hal_rcc_ex
  ├── AL94_USB_Composite (Third-party)
  │     ├── usbd_composite → usbd_cdc_acm (×3)
  │     ├── usbd_core, usbd_ctlreq, usbd_ioreq
  │     └── usbd_conf (hardware binding)
  └── SynaptiX/
        ├── apps/       → gọi services
        ├── board/      → wrap HAL
        ├── services/
        │     ├── Modbus (RTU + ASCII) → dùng USART1/RS485
        │     ├── mbport               → abstract UART cho Modbus
        │     ├── shell                → dùng USART2 hoặc USB CDC
        │     ├── lora                 → dùng SPI4
        │     └── zigbee               → dùng USART2 (KHÔNG phải SPI4)
        └── utils/

8. Coding Patterns & Conventions
Pattern chính:

STM32CubeMX auto-generated code cho Core/ và Drivers/ — không chỉnh sửa trực tiếp những file trong vùng USER CODE BEGIN/END
Business logic hoàn toàn nằm trong SynaptiX/ — tách biệt khỏi HAL
FreeRTOS task-based architecture: mỗi service chạy trong task riêng hoặc dùng queue/semaphore
Board Abstraction Layer (BAL) trong SynaptiX/board/ — wrap HAL calls

Naming convention:

HAL: MX_<PERIPHERAL>_Init(), HAL_<PERIPHERAL>_<Action>()
FreeRTOS task entry: lowercase với suffix _task hoặc _app (vd: main_app)
GPIO labels: ALL_CAPS (OUT0, IN0, LED0, USART1_DE)

Build system:

GCC ARM Makefile (auto-gen bởi CubeMX tool 4.7.0)
SynaptiX/synaptix.mk include vào Makefile chính → biến DEVICE_SOURCE, MODBUS_SOURCE
Toolchain: arm-none-eabi-gcc
Flash target: STM32_Programmer_CLI -c port=SWD
Optimization: -Os (size), debug: -g -gdwarf-2

Interrupt priority:

NVIC_PRIORITYGROUP_4 → 4-bit preemption, 0-bit sub-priority
FreeRTOS (PendSV/SysTick): priority 15 (lowest)
Peripherals (USART, TIM, DMA, USB): priority 5
HAL timebase (TIM6): priority 15


PROJECT KNOWLEDGE BASE
Mục tiêu hệ thống
Firmware embedded cho board RS485_IO_RF_V2 (SynaptiX). Đây là một IO gateway module công nghiệp với chức năng: nhận/gửi dữ liệu qua RS485 bằng giao thức Modbus RTU/ASCII; điều khiển/đọc 4 digital output + 4 digital input; giao tiếp không dây qua module RF (LoRa hoặc Zigbee) trên SPI; cung cấp debug/config interface qua USB CDC (3 virtual COM port); watchdog tự phục hồi.
MCU & Platform

MCU: STM32H523CCU6 — ARM Cortex-M33, 250 MHz, 256KB Flash, 272KB RAM, UFQFPN48
SDK: STM32Cube FW_H5 V1.5.1
RTOS: FreeRTOS 10.6.2 (CMSIS-RTOS2), heap_4, ARM_CM33_NTZ port
Build: GCC ARM Makefile, arm-none-eabi-gcc, -Os, -mcpu=cortex-m33 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
Linker script: STM32H523xx_FLASH.ld (default)

Kiến trúc Layer
[SynaptiX Apps]  ←→  [SynaptiX Services: Modbus, Shell, LoRa, Zigbee]
       ↓
[SynaptiX Board Abstraction Layer]
       ↓
[HAL: UART, SPI, TIM, DMA, USB, GPIO, IWDG]
       ↓
[Hardware: STM32H523CCU6]
Peripheral Map (quan trọng)
PeripheralVai tròPinGhi chúUSART1RS485PB14/PB15, DE=PB13Half-duplex, ModbusUSART2ZigbeePА2/PA3DMA CH0(RX)/CH1(TX) — Zigbee module UARTLPUART1Aux UARTPA9/PA10SPI4LoRa modulePA0(SCK)/PA8(MOSI)/PB7(MISO)2 Mbps, Master — CHỈ LoRa, không dùng cho ZigbeeUSBVirtual COMPA11(DM)/PA12(DP)3×CDC ACM compositeTIM1-TIM5Timing-1kHz (250MHz/250/1000)TIM6HAL timebase-GPDMA1USART2 DMA-CH0=RX, CH1=TXIWDGWatchdog-~2s timeout, prescaler 64GPIO OUTOutputsPA6,PA7,PB0,PB1OUT0-OUT3GPIO INInputsPB6,PB5,PB4,PB3IN0-IN3GPIO LEDStatusPC13,PC14,PC15LED0-LED2
Modules chính

Core/Src/main.c — entry point, init sequence
Core/Src/app_freertos.c — FreeRTOS task definitions, main_app là task duy nhất được khai báo trong .ioc
SynaptiX/apps/ — application logic (main_app entry đây)
SynaptiX/board/ — HAL wrapper, board-specific functions
SynaptiX/services/Modbus/modbus/ — Modbus RTU + ASCII stack
SynaptiX/services/mbport/ — Modbus port: kết nối Modbus stack với USART1/RS485
SynaptiX/services/shell/ — Debug CLI shell
SynaptiX/services/lora/ — LoRa driver (SPI4)
SynaptiX/services/zigbee/ — Zigbee driver (USART2 — KHÔNG phải SPI4)
SynaptiX/utils/ — tiện ích chung
Middlewares/Third_Party/AL94_USB_Composite/ — USB 3×CDC ACM

Data Flow
RS485 Bus → USART1 IRQ → Modbus RTU Parser → App Logic → Modbus Response → USART1 TX (DE toggle)
SPI4 ←→ LoRa Module ←→ LoRa Service Task ←→ App Logic
USART2 (DMA) ←→ Zigbee Module ←→ Zigbee Service Task ←→ App Logic
USB CDC ← Shell Service ← Debug commands
App Logic ←→ GPIO (IN0-IN3 read, OUT0-OUT3 write)
USART2 DMA ← RF serial data (nếu RF dùng UART thay SPI)
FreeRTOS Task Config

Heap: 128 KB (heap_4)
Task mainApp: priority osPriorityNormal, stack 256 words, dynamic alloc
Timebase: TIM6 (không dùng SysTick để tránh conflict với FreeRTOS)
NVIC priority group: 4 (all preemption)
FreeRTOS tasks priority: 15 (lowest preemption)
Peripheral IRQ priority: 5

USB Composite Config

Library: AL94 I-CUBE-USBD-COMPOSITE v1.0.3
Cấu hình: 3× CDC ACM (_USBD_CDC_ACM_COUNT=3)
Files user-edit: usbd_cdc_acm_if.c (callback data received/sent)

Coding Conventions

HAL init: MX_<PERIPHERAL>_Init() trong Core/Src/<peripheral>.c
User code: trong vùng /* USER CODE BEGIN */ / /* USER CODE END */ trong Core files
Business logic: KHÔNG được sửa Core/ trực tiếp ngoài USER CODE zones
SynaptiX/synaptix.mk phải update khi thêm file mới vào SynaptiX/
Prefix naming: HAL_, MX_, os (FreeRTOS CMSIS), USBD_, Modbus_ (quy ước stack Modbus)

Build & Flash
bashmake -j4                    # build
make flash                  # flash qua SWD (STM32_Programmer_CLI)
make clean                  # clean
Dependency Graph tóm tắt
SynaptiX/apps
  → SynaptiX/services (Modbus, Shell, LoRa, Zigbee)
    → SynaptiX/board
      → HAL (Core/Src + Drivers/STM32H5xx_HAL_Driver)
        → CMSIS
SynaptiX/services/Modbus → SynaptiX/services/mbport → HAL UART (USART1)
SynaptiX/services/lora    → HAL SPI (SPI4)
SynaptiX/services/zigbee  → HAL UART (USART2)
USB → AL94_USB_Composite → HAL PCD + LL USB
FreeRTOS → CMSIS-RTOS2 → HAL TIM6 (timebase)

Quyết định thiết kế quan trọng

TIM6 làm HAL timebase thay SysTick — bắt buộc khi dùng FreeRTOS để tránh xung đột
SPI4 nguồn HSI (32 MHz) thay vì PLL — đảm bảo clock LoRa module không bị ảnh hưởng khi thay đổi PLL (chỉ áp dụng cho LoRa; Zigbee dùng USART2)
3 CDC ACM trên USB composite — có thể 1 cho Modbus bridge, 1 cho shell, 1 cho RF monitor
GPDMA cho USART2 nhưng USART1 (RS485) dùng IRQ — Modbus yêu cầu timing chính xác nên IRQ-driven; USART2 dùng DMA cho throughput
heap_4 thay heap_1/2 — hỗ trợ free() + tái cấp phát, phù hợp với shell và dynamic protocol handling
SynaptiX/ tách hoàn toàn khỏi Core/ — cho phép regenerate code từ CubeMX mà không mất business logic
UFQFPN48 package — form factor nhỏ gọn cho board công nghiệp, giới hạn số chân dẫn đến chọn STM32H523CCU6

=====CẤU trúc của SynaptiX======

D:\Synaptix\SynaptiX\Checkout\rs485\rs485v2\SynaptiX\services>tree /F
Folder PATH listing
Volume serial number is 6F52-D360
D:.
│   logger.c
│   logger.h
│   
├───communicate
│       communicate.c
│       communicate.h
│       
├───lora
│       lora.c
│       lora.h
│       
├───mbport
│       port.h
│       portevent.c
│       portserial.c
│       porttimer.c
│       
├───Modbus
│   │   bsd.txt
│   │   Changelog.txt
│   │   gpl.txt
│   │   lgpl.txt
│   │   README.md
│   │   
│   ├───demo
│   │   └───STM32F0
│   │       │   STM32F051R8TX_FLASH.ld
│   │       │   TestJSON.ioc
│   │       │   TestJSON.launch
│   │       │   
│   │       ├───Core
│   │       │   ├───Inc
│   │       │   │       cJSON.h
│   │       │   │       cJSON_Utils.h
│   │       │   │       main.h
│   │       │   │       stm32f0xx_hal_conf.h
│   │       │   │       stm32f0xx_it.h
│   │       │   │       
│   │       │   ├───Src
│   │       │   │       cJSON.c
│   │       │   │       cJSON_Utils.c
│   │       │   │       main.c
│   │       │   │       stm32f0xx_hal_msp.c
│   │       │   │       stm32f0xx_it.c
│   │       │   │       syscalls.c
│   │       │   │       sysmem.c
│   │       │   │       system_stm32f0xx.c
│   │       │   │       
│   │       │   └───Startup
│   │       │           startup_stm32f051r8tx.s
│   │       │           
│   │       ├───Drivers
│   │       │   ├───CMSIS
│   │       │   │   ├───Device
│   │       │   │   │   └───ST
│   │       │   │   │       └───STM32F0xx
│   │       │   │   │           └───Include
│   │       │   │   │                   stm32f051x8.h
│   │       │   │   │                   stm32f0xx.h
│   │       │   │   │                   system_stm32f0xx.h
│   │       │   │   │                   
│   │       │   │   └───Include
│   │       │   │           cmsis_armcc.h
│   │       │   │           cmsis_armclang.h
│   │       │   │           cmsis_compiler.h
│   │       │   │           cmsis_gcc.h
│   │       │   │           cmsis_iccarm.h
│   │       │   │           cmsis_version.h
│   │       │   │           core_armv8mbl.h
│   │       │   │           core_armv8mml.h
│   │       │   │           core_cm0.h
│   │       │   │           core_cm0plus.h
│   │       │   │           core_cm1.h
│   │       │   │           core_cm23.h
│   │       │   │           core_cm3.h
│   │       │   │           core_cm33.h
│   │       │   │           core_cm4.h
│   │       │   │           core_cm7.h
│   │       │   │           core_sc000.h
│   │       │   │           core_sc300.h
│   │       │   │           mpu_armv7.h
│   │       │   │           mpu_armv8.h
│   │       │   │           tz_context.h
│   │       │   │           
│   │       │   └───STM32F0xx_HAL_Driver
│   │       │       ├───Inc
│   │       │       │   │   stm32f0xx_hal.h
│   │       │       │   │   stm32f0xx_hal_cortex.h
│   │       │       │   │   stm32f0xx_hal_def.h
│   │       │       │   │   stm32f0xx_hal_dma.h
│   │       │       │   │   stm32f0xx_hal_dma_ex.h
│   │       │       │   │   stm32f0xx_hal_exti.h
│   │       │       │   │   stm32f0xx_hal_flash.h
│   │       │       │   │   stm32f0xx_hal_flash_ex.h
│   │       │       │   │   stm32f0xx_hal_gpio.h
│   │       │       │   │   stm32f0xx_hal_gpio_ex.h
│   │       │       │   │   stm32f0xx_hal_i2c.h
│   │       │       │   │   stm32f0xx_hal_i2c_ex.h
│   │       │       │   │   stm32f0xx_hal_pwr.h
│   │       │       │   │   stm32f0xx_hal_pwr_ex.h
│   │       │       │   │   stm32f0xx_hal_rcc.h
│   │       │       │   │   stm32f0xx_hal_rcc_ex.h
│   │       │       │   │   stm32f0xx_hal_tim.h
│   │       │       │   │   stm32f0xx_hal_tim_ex.h
│   │       │       │   │   stm32f0xx_hal_uart.h
│   │       │       │   │   stm32f0xx_hal_uart_ex.h
│   │       │       │   │   
│   │       │       │   └───Legacy
│   │       │       │           stm32_hal_legacy.h
│   │       │       │           
│   │       │       └───Src
│   │       │               stm32f0xx_hal.c
│   │       │               stm32f0xx_hal_cortex.c
│   │       │               stm32f0xx_hal_dma.c
│   │       │               stm32f0xx_hal_exti.c
│   │       │               stm32f0xx_hal_flash.c
│   │       │               stm32f0xx_hal_flash_ex.c
│   │       │               stm32f0xx_hal_gpio.c
│   │       │               stm32f0xx_hal_i2c.c
│   │       │               stm32f0xx_hal_i2c_ex.c
│   │       │               stm32f0xx_hal_pwr.c
│   │       │               stm32f0xx_hal_pwr_ex.c
│   │       │               stm32f0xx_hal_rcc.c
│   │       │               stm32f0xx_hal_rcc_ex.c
│   │       │               stm32f0xx_hal_tim.c
│   │       │               stm32f0xx_hal_tim_ex.c
│   │       │               stm32f0xx_hal_uart.c
│   │       │               stm32f0xx_hal_uart_ex.c
│   │       │               
│   │       └───port
│   │               port.h
│   │               portevent.c
│   │               portserial.c
│   │               porttimer.c
│   │               user_mb_app.c
│   │               user_mb_app.h
│   │               
│   ├───doc
│   │       dox.css
│   │       doxygen.conf
│   │       dox_html_footer
│   │       dox_html_header
│   │       main.dox
│   │       memory.ods
│   │       porting.dox
│   │       tips.dox
│   │       TODO.txt
│   │       
│   ├───modbus
│   │   │   mb.c
│   │   │   
│   │   ├───ascii
│   │   │       mbascii.c
│   │   │       mbascii.h
│   │   │       
│   │   ├───functions
│   │   │       mbfunccoils.c
│   │   │       mbfuncdiag.c
│   │   │       mbfuncdisc.c
│   │   │       mbfuncholding.c
│   │   │       mbfuncinput.c
│   │   │       mbfuncother.c
│   │   │       mbutils.c
│   │   │       
│   │   ├───include
│   │   │       mb.h
│   │   │       mbconfig.h
│   │   │       mbframe.h
│   │   │       mbfunc.h
│   │   │       mbport.h
│   │   │       mbproto.h
│   │   │       mbutils.h
│   │   │       
│   │   ├───rtu
│   │   │       mbcrc.c
│   │   │       mbcrc.h
│   │   │       mbrtu.c
│   │   │       mbrtu.h
│   │   │       
│   │   └───tcp
│   │           mbtcp.c
│   │           mbtcp.h
│   │           
│   ├───port
│   │       port.h
│   │       portevent.c
│   │       portserial.c
│   │       porttimer.c
│   │       user_mb_app.c
│   │       user_mb_app.h
│   │       
│   └───tools
│           doxygen.exe
│           indent.sh
│           lint-arm.sh
│           lint-avr.sh
│           README.txt
│           
├───shell
│       cli_shell.c
│       cli_shell.h
│       
└───zigbee
        zigbee.c
        zigbee.h
        

D:\Synaptix\SynaptiX\Checkout\rs485\rs485v2\SynaptiX\services>

7. Dependency Graph
main.c
  ├── FreeRTOS (CMSIS-RTOS2)
  │     └── port ARM_CM33_NTZ + heap_4
  ├── HAL (STM32H5xx_HAL_Driver)
  │     ├── hal_uart, hal_uart_ex
  │     ├── hal_spi, hal_spi_ex
  │     ├── hal_tim, hal_tim_ex
  │     ├── hal_dma, hal_dma_ex
  │     ├── hal_pcd, hal_pcd_ex + ll_usb
  │     ├── hal_gpio, hal_iwdg, hal_icache
  │     └── hal_rcc, hal_rcc_ex
  ├── AL94_USB_Composite (Third-party)
  │     ├── usbd_composite → usbd_cdc_acm (×3)
  │     ├── usbd_core, usbd_ctlreq, usbd_ioreq
  │     └── usbd_conf (hardware binding)
  └── SynaptiX/
        ├── apps/       → gọi services
        ├── board/      → wrap HAL
        ├── services/
        │     ├── Modbus (RTU + ASCII) → dùng USART1/RS485
        │     ├── mbport               → abstract UART cho Modbus
        │     ├── shell                → dùng USART2 hoặc USB CDC
        │     ├── lora                 → dùng SPI4
        │     └── zigbee               → dùng SPI4
        └── utils/

8. Coding Patterns & Conventions
Pattern chính:

STM32CubeMX auto-generated code cho Core/ và Drivers/ — không chỉnh sửa trực tiếp những file trong vùng USER CODE BEGIN/END
Business logic hoàn toàn nằm trong SynaptiX/ — tách biệt khỏi HAL
FreeRTOS task-based architecture: mỗi service chạy trong task riêng hoặc dùng queue/semaphore
Board Abstraction Layer (BAL) trong SynaptiX/board/ — wrap HAL calls

Naming convention:

HAL: MX_<PERIPHERAL>_Init(), HAL_<PERIPHERAL>_<Action>()
FreeRTOS task entry: lowercase với suffix _task hoặc _app (vd: main_app)
GPIO labels: ALL_CAPS (OUT0, IN0, LED0, USART1_DE)

Build system:

GCC ARM Makefile (auto-gen bởi CubeMX tool 4.7.0)
SynaptiX/synaptix.mk include vào Makefile chính → biến DEVICE_SOURCE, MODBUS_SOURCE
Toolchain: arm-none-eabi-gcc
Flash target: STM32_Programmer_CLI -c port=SWD
Optimization: -Os (size), debug: -g -gdwarf-2

Interrupt priority:

NVIC_PRIORITYGROUP_4 → 4-bit preemption, 0-bit sub-priority
FreeRTOS (PendSV/SysTick): priority 15 (lowest)
Peripherals (USART, TIM, DMA, USB): priority 5
HAL timebase (TIM6): priority 15


PROJECT KNOWLEDGE BASE
Mục tiêu hệ thống
Firmware embedded cho board RS485_IO_RF_V2 (SynaptiX). Đây là một IO gateway module công nghiệp với chức năng: nhận/gửi dữ liệu qua RS485 bằng giao thức Modbus RTU/ASCII; điều khiển/đọc 4 digital output + 4 digital input; giao tiếp không dây qua module RF (LoRa hoặc Zigbee) trên SPI; cung cấp debug/config interface qua USB CDC (3 virtual COM port); watchdog tự phục hồi.
MCU & Platform

MCU: STM32H523CCU6 — ARM Cortex-M33, 250 MHz, 256KB Flash, 272KB RAM, UFQFPN48
SDK: STM32Cube FW_H5 V1.5.1
RTOS: FreeRTOS 10.6.2 (CMSIS-RTOS2), heap_4, ARM_CM33_NTZ port
Build: GCC ARM Makefile, arm-none-eabi-gcc, -Os, -mcpu=cortex-m33 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
Linker script: STM32H523xx_FLASH.ld (default)

Kiến trúc Layer
[SynaptiX Apps]  ←→  [SynaptiX Services: Modbus, Shell, LoRa, Zigbee]
       ↓
[SynaptiX Board Abstraction Layer]
       ↓
[HAL: UART, SPI, TIM, DMA, USB, GPIO, IWDG]
       ↓
[Hardware: STM32H523CCU6]
Peripheral Map (quan trọng)
PeripheralVai tròPinGhi chúUSART1RS485PB14/PB15, DE=PB13Half-duplex, ModbusUSART2Debug/RFPA2/PA3DMA CH0(RX)/CH1(TX)LPUART1Aux UARTPA9/PA10SPI4RF modulePA0(SCK)/PA8(MOSI)/PB7(MISO)2 Mbps, MasterUSBVirtual COMPA11(DM)/PA12(DP)3×CDC ACM compositeTIM1-TIM5Timing-1kHz (250MHz/250/1000)TIM6HAL timebase-GPDMA1USART2 DMA-CH0=RX, CH1=TXIWDGWatchdog-~2s timeout, prescaler 64GPIO OUTOutputsPA6,PA7,PB0,PB1OUT0-OUT3GPIO INInputsPB6,PB5,PB4,PB3IN0-IN3GPIO LEDStatusPC13,PC14,PC15LED0-LED2
Modules chính

Core/Src/main.c — entry point, init sequence
Core/Src/app_freertos.c — FreeRTOS task definitions, main_app là task duy nhất được khai báo trong .ioc
SynaptiX/apps/ — application logic (main_app entry đây)
SynaptiX/board/ — HAL wrapper, board-specific functions
SynaptiX/services/Modbus/modbus/ — Modbus RTU + ASCII stack
SynaptiX/services/mbport/ — Modbus port: kết nối Modbus stack với USART1/RS485
SynaptiX/services/shell/ — Debug CLI shell
SynaptiX/services/lora/ — LoRa driver (SPI4)
SynaptiX/services/zigbee/ — Zigbee driver (SPI4)
SynaptiX/utils/ — tiện ích chung
Middlewares/Third_Party/AL94_USB_Composite/ — USB 3×CDC ACM

Data Flow
RS485 Bus → USART1 IRQ → Modbus RTU Parser → App Logic → Modbus Response → USART1 TX (DE toggle)
SPI4 ←→ RF Module (LoRa/Zigbee) ←→ RF Service Task ←→ App Logic
USB CDC ← Shell Service ← Debug commands
App Logic ←→ GPIO (IN0-IN3 read, OUT0-OUT3 write)
USART2 DMA ← RF serial data (nếu RF dùng UART thay SPI)
FreeRTOS Task Config

Heap: 128 KB (heap_4)
Task mainApp: priority osPriorityNormal, stack 256 words, dynamic alloc
Timebase: TIM6 (không dùng SysTick để tránh conflict với FreeRTOS)
NVIC priority group: 4 (all preemption)
FreeRTOS tasks priority: 15 (lowest preemption)
Peripheral IRQ priority: 5

USB Composite Config

Library: AL94 I-CUBE-USBD-COMPOSITE v1.0.3
Cấu hình: 3× CDC ACM (_USBD_CDC_ACM_COUNT=3)
Files user-edit: usbd_cdc_acm_if.c (callback data received/sent)

Coding Conventions

HAL init: MX_<PERIPHERAL>_Init() trong Core/Src/<peripheral>.c
User code: trong vùng /* USER CODE BEGIN */ / /* USER CODE END */ trong Core files
Business logic: KHÔNG được sửa Core/ trực tiếp ngoài USER CODE zones
SynaptiX/synaptix.mk phải update khi thêm file mới vào SynaptiX/
Prefix naming: HAL_, MX_, os (FreeRTOS CMSIS), USBD_, Modbus_ (quy ước stack Modbus)

Build & Flash
bashmake -j4                    # build
make flash                  # flash qua SWD (STM32_Programmer_CLI)
make clean                  # clean
Dependency Graph tóm tắt
SynaptiX/apps
  → SynaptiX/services (Modbus, Shell, LoRa, Zigbee)
    → SynaptiX/board
      → HAL (Core/Src + Drivers/STM32H5xx_HAL_Driver)
        → CMSIS
SynaptiX/services/Modbus → SynaptiX/services/mbport → HAL UART (USART1)
SynaptiX/services/lora|zigbee → HAL SPI (SPI4)
USB → AL94_USB_Composite → HAL PCD + LL USB
FreeRTOS → CMSIS-RTOS2 → HAL TIM6 (timebase)

Quyết định thiết kế quan trọng

TIM6 làm HAL timebase thay SysTick — bắt buộc khi dùng FreeRTOS để tránh xung đột
SPI4 nguồn HSI (32 MHz) thay vì PLL — đảm bảo clock RF module không bị ảnh hưởng khi thay đổi PLL
3 CDC ACM trên USB composite — có thể 1 cho Modbus bridge, 1 cho shell, 1 cho RF monitor
GPDMA cho USART2 nhưng USART1 (RS485) dùng IRQ — Modbus yêu cầu timing chính xác nên IRQ-driven; USART2 dùng DMA cho throughput
heap_4 thay heap_1/2 — hỗ trợ free() + tái cấp phát, phù hợp với shell và dynamic protocol handling
SynaptiX/ tách hoàn toàn khỏi Core/ — cho phép regenerate code từ CubeMX mà không mất business logic
UFQFPN48 package — form factor nhỏ gọn cho board công nghiệp, giới hạn số chân dẫn đến chọn STM32H523CCU6

=====CẤU trúc của SynaptiX======

D:\Synaptix\SynaptiX\Checkout\rs485\rs485v2\SynaptiX\services>tree /F
Folder PATH listing
Volume serial number is 6F52-D360
D:.
│   logger.c
│   logger.h
│   
├───communicate
│       communicate.c
│       communicate.h
│       
├───lora
│       lora.c
│       lora.h
│       
├───mbport
│       port.h
│       portevent.c
│       portserial.c
│       porttimer.c
│       
├───Modbus
│   │   bsd.txt
│   │   Changelog.txt
│   │   gpl.txt
│   │   lgpl.txt
│   │   README.md
│   │   
│   ├───demo
│   │   └───STM32F0
│   │       │   STM32F051R8TX_FLASH.ld
│   │       │   TestJSON.ioc
│   │       │   TestJSON.launch
│   │       │   
│   │       ├───Core
│   │       │   ├───Inc
│   │       │   │       cJSON.h
│   │       │   │       cJSON_Utils.h
│   │       │   │       main.h
│   │       │   │       stm32f0xx_hal_conf.h
│   │       │   │       stm32f0xx_it.h
│   │       │   │       
│   │       │   ├───Src
│   │       │   │       cJSON.c
│   │       │   │       cJSON_Utils.c
│   │       │   │       main.c
│   │       │   │       stm32f0xx_hal_msp.c
│   │       │   │       stm32f0xx_it.c
│   │       │   │       syscalls.c
│   │       │   │       sysmem.c
│   │       │   │       system_stm32f0xx.c
│   │       │   │       
│   │       │   └───Startup
│   │       │           startup_stm32f051r8tx.s
│   │       │           
│   │       ├───Drivers
│   │       │   ├───CMSIS
│   │       │   │   ├───Device
│   │       │   │   │   └───ST
│   │       │   │   │       └───STM32F0xx
│   │       │   │   │           └───Include
│   │       │   │   │                   stm32f051x8.h
│   │       │   │   │                   stm32f0xx.h
│   │       │   │   │                   system_stm32f0xx.h
│   │       │   │   │                   
│   │       │   │   └───Include
│   │       │   │           cmsis_armcc.h
│   │       │   │           cmsis_armclang.h
│   │       │   │           cmsis_compiler.h
│   │       │   │           cmsis_gcc.h
│   │       │   │           cmsis_iccarm.h
│   │       │   │           cmsis_version.h
│   │       │   │           core_armv8mbl.h
│   │       │   │           core_armv8mml.h
│   │       │   │           core_cm0.h
│   │       │   │           core_cm0plus.h
│   │       │   │           core_cm1.h
│   │       │   │           core_cm23.h
│   │       │   │           core_cm3.h
│   │       │   │           core_cm33.h
│   │       │   │           core_cm4.h
│   │       │   │           core_cm7.h
│   │       │   │           core_sc000.h
│   │       │   │           core_sc300.h
│   │       │   │           mpu_armv7.h
│   │       │   │           mpu_armv8.h
│   │       │   │           tz_context.h
│   │       │   │           
│   │       │   └───STM32F0xx_HAL_Driver
│   │       │       ├───Inc
│   │       │       │   │   stm32f0xx_hal.h
│   │       │       │   │   stm32f0xx_hal_cortex.h
│   │       │       │   │   stm32f0xx_hal_def.h
│   │       │       │   │   stm32f0xx_hal_dma.h
│   │       │       │   │   stm32f0xx_hal_dma_ex.h
│   │       │       │   │   stm32f0xx_hal_exti.h
│   │       │       │   │   stm32f0xx_hal_flash.h
│   │       │       │   │   stm32f0xx_hal_flash_ex.h
│   │       │       │   │   stm32f0xx_hal_gpio.h
│   │       │       │   │   stm32f0xx_hal_gpio_ex.h
│   │       │       │   │   stm32f0xx_hal_i2c.h
│   │       │       │   │   stm32f0xx_hal_i2c_ex.h
│   │       │       │   │   stm32f0xx_hal_pwr.h
│   │       │       │   │   stm32f0xx_hal_pwr_ex.h
│   │       │       │   │   stm32f0xx_hal_rcc.h
│   │       │       │   │   stm32f0xx_hal_rcc_ex.h
│   │       │       │   │   stm32f0xx_hal_tim.h
│   │       │       │   │   stm32f0xx_hal_tim_ex.h
│   │       │       │   │   stm32f0xx_hal_uart.h
│   │       │       │   │   stm32f0xx_hal_uart_ex.h
│   │       │       │   │   
│   │       │       │   └───Legacy
│   │       │       │           stm32_hal_legacy.h
│   │       │       │           
│   │       │       └───Src
│   │       │               stm32f0xx_hal.c
│   │       │               stm32f0xx_hal_cortex.c
│   │       │               stm32f0xx_hal_dma.c
│   │       │               stm32f0xx_hal_exti.c
│   │       │               stm32f0xx_hal_flash.c
│   │       │               stm32f0xx_hal_flash_ex.c
│   │       │               stm32f0xx_hal_gpio.c
│   │       │               stm32f0xx_hal_i2c.c
│   │       │               stm32f0xx_hal_i2c_ex.c
│   │       │               stm32f0xx_hal_pwr.c
│   │       │               stm32f0xx_hal_pwr_ex.c
│   │       │               stm32f0xx_hal_rcc.c
│   │       │               stm32f0xx_hal_rcc_ex.c
│   │       │               stm32f0xx_hal_tim.c
│   │       │               stm32f0xx_hal_tim_ex.c
│   │       │               stm32f0xx_hal_uart.c
│   │       │               stm32f0xx_hal_uart_ex.c
│   │       │               
│   │       └───port
│   │               port.h
│   │               portevent.c
│   │               portserial.c
│   │               porttimer.c
│   │               user_mb_app.c
│   │               user_mb_app.h
│   │               
│   ├───doc
│   │       dox.css
│   │       doxygen.conf
│   │       dox_html_footer
│   │       dox_html_header
│   │       main.dox
│   │       memory.ods
│   │       porting.dox
│   │       tips.dox
│   │       TODO.txt
│   │       
│   ├───modbus
│   │   │   mb.c
│   │   │   
│   │   ├───ascii
│   │   │       mbascii.c
│   │   │       mbascii.h
│   │   │       
│   │   ├───functions
│   │   │       mbfunccoils.c
│   │   │       mbfuncdiag.c
│   │   │       mbfuncdisc.c
│   │   │       mbfuncholding.c
│   │   │       mbfuncinput.c
│   │   │       mbfuncother.c
│   │   │       mbutils.c
│   │   │       
│   │   ├───include
│   │   │       mb.h
│   │   │       mbconfig.h
│   │   │       mbframe.h
│   │   │       mbfunc.h
│   │   │       mbport.h
│   │   │       mbproto.h
│   │   │       mbutils.h
│   │   │       
│   │   ├───rtu
│   │   │       mbcrc.c
│   │   │       mbcrc.h
│   │   │       mbrtu.c
│   │   │       mbrtu.h
│   │   │       
│   │   └───tcp
│   │           mbtcp.c
│   │           mbtcp.h
│   │           
│   ├───port
│   │       port.h
│   │       portevent.c
│   │       portserial.c
│   │       porttimer.c
│   │       user_mb_app.c
│   │       user_mb_app.h
│   │       
│   └───tools
│           doxygen.exe
│           indent.sh
│           lint-arm.sh
│           lint-avr.sh
│           README.txt
│           
├───shell
│       cli_shell.c
│       cli_shell.h
│       
└───zigbee
        zigbee.c
        zigbee.h
        

D:\Synaptix\SynaptiX\Checkout\rs485\rs485v2\SynaptiX\services>