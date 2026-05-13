# STM32F103 RFID Motor + TTS System

> 基于 STM32F103 + FreeRTOS 的 UART RFID 刷卡语音播报 + 直流电机控制系统

## Overview

STM32F103C8/CB FreeRTOS system with two tasks:
1. **Motor Control**: PA11 toggle switch (NC=RUN, NO=INIT) controls DC motor via H-bridge with 4-second soft-start ramp
2. **RFID + TTS**: UART RFID module reads card block data (GB2312 Chinese text), SYN6288 voice broadcast, LED indicator with delayed off

**Clock:** HSE external 8MHz crystal → PLL 9x → 72MHz SYSCLK

---

## Hardware Pin Assignments

| Peripheral | Signal | Pin | Mode |
|---|---|---|---|
| Toggle Switch | SW_IN | PA11 | GPIO Input, internal pull-up |
| H-Bridge Motor | IN1 | PB12 | GPIO Push-Pull Output |
| | IN2 | PB13 | GPIO Push-Pull Output |
| | PWM | PB0 | TIM3_CH3, AF Push-Pull |
| UART RFID (USART1) | TX | PA9 | USART1_TX (115200bps) |
| | RX | PA10 | USART1_RX (115200bps) |
| SYN6288 (USART2) | TX | PA2 | USART2_TX (9600bps) |
| | RX | PA3 | USART2_RX |
| | BUSY | PA1 | GPIO Input |
| LED Indicator | LED1 | PA8 | GPIO Push-Pull Output (active-low) |
| | LED2 | PC13 | GPIO Push-Pull Output (active-low) |

No pin conflicts.

---

## Software Architecture

### FreeRTOS Tasks

| Task | Stack | Priority | Role |
|---|---|---|---|
| `Task_Motor` | 128 words | Normal | Read PA11, soft-start motor, stop on INIT |
| `Task_RFID_TTS` | 256 words | Normal | UART RFID state machine, card block reading, TTS broadcast, LED control |

### RFID Card Read State Machine

```
NONE ──(card detected)──▶ EXIST ──(send ReadBlock)──▶ WAIT
  ▲                          ▲                            │
  │                          │                  (response received)
  │                          │                            ▼
  │                          ◀──(next block)────── RESDATA
  │                                                   │
  │                                        (data complete)
  │                                                   ▼
  ◀────────(card left, LED off)────────────── LEDLIGHT
                                                  LED On + TTS broadcast
```

- Block 4 is read first (official APP data location)
- Falls back to Block 1 if Block 4 is empty
- Cross-block Chinese data assembly (up to 60 bytes / 30 GB2312 characters)
- Brand filter: cards starting with "一点" are silently ignored

### RFID UART Protocol

Private binary protocol over USART1:
- Frame: `[0x7F] [escaped payload]` with XOR checksum
- Baud rate auto-switch: 9600 → 115200 on init
- Commands: `0x10` ReadCard, `0x11` ReadBlock, `0xAC` SetBaud
- Interrupt-driven single-byte receive state machine

### Shared State (mutex protected)

```c
typedef enum {
    SYSTEM_MODE_RUN  = 0,  // PA11 low (NC): normal operation
    SYSTEM_MODE_INIT = 1,  // PA11 high (NO): stop + reinit
} SystemMode_t;

typedef struct {
    SystemMode_t mode;
    uint8_t      motor_direction;  // 0=forward, 1=reverse
    uint16_t     motor_speed;      // PWM duty 0~999
} SystemState_t;
```

---

## Configurable Parameters

| Variable | Default | Range | File | Description |
|---|---|---|---|---|
| `motor_direction` | 0 | 0/1 | `system_state.c` | 0=forward, 1=reverse |
| `motor_speed` | 500 | 0~999 | `system_state.c` | PWM duty cycle |
| `motor_ramp_ms` | 4000 | — | `main.c:Task_Motor` | Soft-start duration |
| `tts_volume` | 5 | 0~10 | `syn6288.h` | SYN6288 volume level |

---

## Build & Flash

**Build:** VS Code with STM32Cube extension → `Ctrl+Shift+B` (build)

**Flash:** Connect ST-LINK, then:
- VS Code: `Ctrl+Shift+B` → select "Flash (ST-LINK)"
- CLI: `STM32_Programmer_CLI.exe -c port=SWD -w build/Release/test1.elf -v`

**Toolchain:** STM32Cube (st-arm-clang), CMake, FreeRTOS CMSIS-OS v1, STM32Cube HAL
