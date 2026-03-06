# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MCU IAP (In-Application Programming) upgrade host tool built with Qt/C++. It communicates with MCU devices over serial port to perform firmware upgrades and voltage calibration.

## Build Commands

```bash
# Build with qmake + MinGW (from mcu_iap/ directory)
cd mcu_iap
qmake MCU_IAP.pro
mingw32-make

# Or open mcu_iap/MCU_IAP.pro in QtCreator and build with Ctrl+B
```

Requires Qt 5.6.2+ with modules: core, gui, widgets, serialport.

## Architecture

Three main components in `mcu_iap/`:

- **MainWindow** (`mainwindow.h/cpp/ui`) — GUI and upgrade flow control. Manages the IAP state machine, user interactions, configuration persistence (QSettings → `setting.ini`), and retry/timeout logic.
- **SerialPortHandler** (`serialporthandler.h/cpp`) — Serial port abstraction. Handles enumeration, open/close, async data reception, and delegates packet framing to DataProtocol.
- **DataProtocol** (`dataprotocol.h/cpp`) — Custom IAP protocol implementation. Packet format: `[Header 0xAC6D][Index][IndexXOR][Length][Data][CRC16]`. Implements CRC-CCITT (protocol frames) and CRC-MODBUS (calibration commands).

## IAP Upgrade Flow

1. Handshake (`0x10` GET_DEVICE_INFO)
2. Send upgrade request (`0x20`)
3. Send firmware info — packet size (`0x21`)
4. Stream BIN file in 2048-byte chunks (`0x22`)
5. CRC verification (`0x23`, `0x24`)
6. Reboot MCU (`0x11`)

## Key Constants (mainwindow.h)

- `D_IAP_BIN_PACKET_LEN = 2048` — firmware chunk size
- Command IDs: `0x10`–`0x24` for IAP protocol
- Retry count: 3, default timeout: 100ms

## Language

Code comments and UI text are in Chinese. Commit messages are also in Chinese.
