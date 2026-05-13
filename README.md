# STM32F103 RFID Motor + TTS System

> 基于 STM32F103 + FreeRTOS 的 RFID 刷卡语音播报 + 直流电机控制系统

## Overview

STM32F103C8/CB FreeRTOS system with two tasks:
1. **Motor Control**: PA10 toggle switch (NC=RUN, NO=INIT) controls DC motor via H-bridge
2. **RFID + TTS**: RC522 reads card UID, looks up GB2312 name, SYN6288 voice broadcast

---

## Hardware Pin Assignments

| Peripheral | Signal | Pin | Mode |
|---|---|---|---|
| Toggle Switch | SW_IN | PA10 | GPIO Input, internal pull-up |
| H-Bridge Motor | IN1 | PB12 | GPIO Push-Pull Output |
| | IN2 | PB13 | GPIO Push-Pull Output |
| | PWM | PB0 | TIM3_CH3, AF Push-Pull |
| RC522 (SPI1) | SCK | PA5 | SPI1_SCK |
| | MISO | PA6 | SPI1_MISO |
| | MOSI | PA7 | SPI1_MOSI |
| | NSS | PA4 | GPIO Output (CS) |
| | RST | PA8 | GPIO Output (Reset) |
| SYN6288 (USART2) | TX | PA2 | USART2_TX |
| | RX | PA3 | USART2_RX |
| | BUSY | PA1 | GPIO Input |

No pin conflicts.

---

## Software Architecture

### FreeRTOS Tasks

| Task | Stack | Priority | Role |
|---|---|---|---|
| `Task_Motor` | 128 words | Normal | Read PA10, control motor direction/speed |
| `Task_RFID_TTS` | 256 words | Normal | RC522 card polling, UID lookup, TTS broadcast |

### Shared State (mutex protected)

```c
typedef enum {
    SYSTEM_MODE_RUN  = 0,  // PA10 low (NC): normal operation
    SYSTEM_MODE_INIT = 1,  // PA10 high (NO): stop + reinit
} SystemMode_t;

typedef struct {
    SystemMode_t mode;
    uint8_t      motor_direction;  // 0=forward, 1=reverse
    uint16_t     motor_speed;      // PWM duty 0~999
} SystemState_t;
```

---

## Configurable Parameters

| Variable | Default | Range | Description |
|---|---|---|---|
| `motor_direction` | 0 | 0/1 | 0=forward, 1=reverse |
| `motor_speed` | 500 | 0~999 | PWM duty cycle |
| `tts_volume` | 5 | 0~10 | SYN6288 volume level |

---

## UID-to-Name Mapping

Edit `Core/Src/uid_table.c` to add card UIDs and their GB2312-encoded Chinese names:

```c
const uid_entry_t uid_table[] = {
    // {UID (hex), GB2312 bytes, byte length}
    // Example: card "Zhang San"
    // {0x12345678, "\xBF\xA8\xC6\xAC\xD5\xC5\xC8\xFD", 8},
};
```

---

## Build & Flash

**Build:** VS Code with STM32Cube extension → `Ctrl+Shift+B` (build)

**Flash:** Connect ST-LINK, then:
- VS Code: `Ctrl+Shift+B` → select "Flash (ST-LINK)"
- CLI: `STM32_Programmer_CLI.exe -c port=SWD -w build/Release/test1.elf -v`

**Toolchain:** STM32Cube (st-arm-clang), CMake, FreeRTOS CMSIS-OS v1, STM32Cube HAL
