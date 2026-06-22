# Build Guide

This document describes how to build, flash and debug the STM32H5 Industrial Gateway firmware.

---

# Prerequisites

## Supported Host Systems

* Ubuntu 22.04+
* Debian 12+
* Linux Mint 21+
* Windows 10/11 (WSL2 recommended)

---

# Required Tools

| Tool                | Version |
| ------------------- | ------- |
| CMake               | 3.22+   |
| Ninja               | 1.10+   |
| ARM GNU Toolchain   | 13+     |
| Git                 | Latest  |
| OpenOCD             | 0.12+   |
| STM32CubeProgrammer | Latest  |

Verify installation:

```bash
cmake --version
ninja --version
arm-none-eabi-gcc --version
git --version
```

---

# Install Dependencies

## Ubuntu

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip
```

---

## ARM GNU Toolchain

Download and install:

```text
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
```

Verify:

```bash
arm-none-eabi-gcc --version
```

---

# Clone Repository

Clone project and submodules:

```bash
git clone --recursive <https://github.com/SynaptiXCompany/RF_IO_RS485_V3.git>

cd stm32h5-industrial-IO
```

If already cloned:

```bash
git submodule update --init --recursive
```

---

# Project Structure

```text
.
├── App/
├── BSP/
├── Core/
├── Drivers/
├── Middleware/
│   ├── tinyusb/
│   ├── mongoose/
│   └── freertos/
│
├── cmake/
├── linker/
├── scripts/
└── CMakeLists.txt
```

---

# Configure Build

## Debug Build

```bash
cmake \
  -B build/debug \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug
```

---

## Release Build

```bash
cmake \
  -B build/release \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

---

# Build Firmware

## Debug

```bash
cmake --build build/Debug
```

---

## Release

```bash
cmake --build build/Release
```

---

# Build Outputs

Generated files:

```text
build/Release/

├── RF_IO_V3.elf
├── RF_IO_V3.bin
├── RF_IO_V3.hex
├── RF_IO_V3.map
└── compile_commands.json
```

---

# Clean Build

```bash
rm -rf build
```

or

```bash
cmake --build build/release --target clean
```

---

# Flash Firmware

## Using STM32CubeProgrammer CLI

```bash
STM32_Programmer_CLI \
-c port=SWD \
-w build/Release/RF_IO_V3.bin 0x08000000 \
-v \
-rst
```

---

## Using OpenOCD

Start OpenOCD:

```bash
openocd \
-f interface/stlink.cfg \
-f target/stm32h5x.cfg
```

Program firmware:

```bash
arm-none-eabi-gdb build/Release/RF_IO_V3.elf
```

Inside GDB:

```gdb
target extended-remote localhost:3333

monitor reset halt

load

monitor reset run

quit
```

---

# Debugging

## VS Code

Required extensions:

* C/C++
* Cortex-Debug
* CMake Tools

Launch:

```bash
F5
```

---

## GDB

```bash
arm-none-eabi-gdb build/Debug/RF_IO_V3.elf
```

Useful commands:

```gdb
break main
continue
next
step
info threads
```

---

# Memory Usage

View memory consumption:

```bash
arm-none-eabi-size build/Release/RF_IO_V3.elf
```

Example:

```text
text    data     bss     dec
520000   4096   78000  602096
```

---

# USB Verification

Connect device to host.

Linux:

```bash
lsusb
```

Expected:

```text
STM32H5 Industrial Gateway
```

Check ACM:

```bash
ls /dev/ttyACM*
```

Check ECM:

```bash
ip link
```

---

# Web Server Verification

Ping device:

```bash
ping 192.168.7.1
```

Open browser:

```text
http://192.168.7.1
```

Expected:

* Dashboard page
* I/O status page
* Modbus configuration page

---

# Production Build

Recommended flags:

```bash
cmake \
  -B build/release \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
```

Compiler optimizations:

```text
-O2
-flto
-ffunction-sections
-fdata-sections
```

Linker options:

```text
-Wl,--gc-sections
```

---

# Firmware Version

Display firmware version:

```bash
strings firmware.bin | grep VERSION
```

Example:

```text
FW_VERSION=1.0.0
```

---

# Troubleshooting

## Missing Toolchain

Error:

```text
arm-none-eabi-gcc: command not found
```

Solution:

```bash
export PATH=<toolchain>/bin:$PATH
```

---

## Missing Submodules

Error:

```text
tinyusb not found
```

Solution:

```bash
git submodule update --init --recursive
```

---

## ST-LINK Not Detected

Check:

```bash
lsusb
```

Install udev rules:

```bash
sudo cp scripts/49-stlinkv2.rules /etc/udev/rules.d/

sudo udevadm control --reload-rules

sudo udevadm trigger
```

---

# Continuous Integration

Example CI build:

```bash
cmake \
  -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel
```

---

# Release Checklist

* Build successful
* No compiler warnings
* Memory usage verified
* USB ACM verified
* USB ECM verified
* USB RNDIS verified
* Web UI verified
* Modbus RTU verified
* DI/DO verified
* Zigbee/LoRa verified
* Firmware version updated
* Release binary generated

---

# Generated Artifacts

```text
RF_IO_V3.elf
RF_IO_V3.bin
RF_IO_V3.hex
RF_IO_V3.map
```
