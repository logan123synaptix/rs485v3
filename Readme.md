# SynaptiX Industrial IO Remote

## Features

### MCU

* STM32H5 Series MCU
* ARM Cortex-M33
* Hardware Security Features
* TrustZone Support
* High Performance Embedded Flash

### USB Device

Powered by TinyUSB.

Supported USB classes:

* CDC ACM (Virtual COM Port)
* CDC ECM (USB Ethernet for Linux/macOS)
* RNDIS (USB Ethernet for Windows)

When connected to a PC, the device appears as:

```text
USB Composite Device
├── Virtual COM Port
├── ECM Network Adapter
└── RNDIS Network Adapter
```

---

## Network Services

### Embedded Web Server

Powered by Mongoose.

Features:

* Device configuration
* Network configuration
* Digital I/O monitoring
* Firmware information
* Device diagnostics
* REST API
* Static web UI

Access:

```text
http://192.168.7.1
```

---

## Communication Interfaces

### RS485

Industrial communication interface.

Supported protocols:

* Modbus RTU Master
* Modbus RTU Slave
* Custom Serial Protocol

### Wireless

Supported wireless modules:

* Zigbee
* LoRa

Applications:

* Sensor Networks
* Remote Monitoring
* Smart Agriculture
* Building Automation

---

## Digital Inputs

### 4x Digital Inputs

Features:

* Opto-isolated inputs
* 24V industrial level
* Status monitoring
* Event detection

| Channel | Description     |
| ------- | --------------- |
| DI1     | Digital Input 1 |
| DI2     | Digital Input 2 |
| DI3     | Digital Input 3 |
| DI4     | Digital Input 4 |

---

## Digital Outputs

### 4x Digital Outputs

Features:

* Industrial output drivers
* Relay control
* Alarm control
* Automation control

| Channel | Description      |
| ------- | ---------------- |
| DO1     | Digital Output 1 |
| DO2     | Digital Output 2 |
| DO3     | Digital Output 3 |
| DO4     | Digital Output 4 |

---

## Power Supply

### Industrial Power Input

```text
Input Voltage : 24VDC
```

Features:

* Reverse polarity protection
* Over-current protection
* EMI filtering
* Industrial power design

---

## System Architecture

```text
                     +----------------+
                     |    Web UI      |
                     |   Mongoose     |
                     +--------+-------+
                              |
                     +--------+-------+
                     |   Application  |
                     +--------+-------+
                              |
        +-----------+---------+---------+-----------+
        |           |                   |           |
        |           |                   |           |
      Modbus      DI/DO           Zigbee/LoRa     Shell
      Service    Service            Service      Service
        |           |                   |           |
        +-----------+---------+---------+-----------+
                              |
                       FreeRTOS Kernel
                              |
                      STM32H5 Platform
```

---

## USB Networking

```text
Host PC
    |
    +-------------------- USB --------------------+
                                                   |
                                            TinyUSB Stack
                                                   |
                                     +-------------+-------------+
                                     |                           |
                                   ECM                        RNDIS
                                     |                           |
                                     +-------------+-------------+
                                                   |
                                                Mongoose
                                                   |
                                              Web Server
```

---

## Web Interface

Available pages:

### Dashboard

* Device Information
* Firmware Version
* System Status
* Network Status

### Digital I/O

* DI Status
* DO Control
* Event Monitoring

### Modbus

* RTU Configuration
* Baudrate Settings
* Device Address

### Wireless

* Zigbee Configuration
* LoRa Configuration

### Network

* IP Address
* Gateway
* DNS
* DHCP Settings

---

## Command Shell

Interactive shell available through:

* USB CDC ACM
* UART Console

Example:

```bash
> help

system info
network status
modbus status
lora status
zigbee status
di read
do set
reboot
factory-reset
```

---

## Modbus RTU

Supported Functions:

```text
01 Read Coils
02 Read Inputs
03 Read Holding Registers
04 Read Input Registers
05 Write Single Coil
06 Write Single Register
15 Write Multiple Coils
16 Write Multiple Registers
```

---

## Typical Applications

### Remote I/O Controller

```text
PLC
 |
RS485 Modbus
 |
STM32H5 Gateway
 |---- DI
 |---- DO
```

### LoRa Gateway

```text
LoRa Sensors
      |
      v
STM32H5 Gateway
      |
 USB ECM/RNDIS
      |
 Web Dashboard
```

### Building Automation

```text
Sensors
   |
 RS485
   |
STM32H5 Gateway
   |
 Ethernet over USB
   |
 Management PC
```

---

## Project Structure

```text
.
├── Core/
├── Drivers/
├── Middleware/
│   ├── tinyusb/
│   ├── mongoose/
│   └── freertos/
│
├── App/
│   ├── webserver/
│   ├── shell/
│   ├── modbus/
│   ├── zigbee/
│   ├── lora/
│   ├── dio/
│   └── network/
│
├── BSP/
├── docs/
│   └── images/
└── README.md
```

---

## Software Components

| Component     | Description                |
| ------------- | -------------------------- |
| STM32 HAL     | Hardware Abstraction Layer |
| FreeRTOS      | Real-Time Operating System |
| TinyUSB       | USB Device Stack           |
| Mongoose      | Embedded Web Server        |
| Modbus RTU    | Industrial Protocol        |
| Shell Service | Command Line Interface     |

---

## Future Enhancements

* MQTT Client
* HTTPS Support
* OTA Firmware Update
* Modbus TCP
* BACnet
* SNMP
* Secure Boot
* TLS Web Interface

---

## License

MIT License
