# STM32F103 RFID Motor + TTS System Design

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

### Synchronization

- `SemaphoreHandle_t state_mutex`: guards all access to `SystemState_t`
- Only `Task_Motor` reads PA10; `Task_RFID_TTS` reads `mode` via mutex
- Mode transitions published by `Task_Motor`, consumed by both tasks

---

## Task Logic

### Task_Motor (100ms cycle)

```
LOOP:
  Read PA10 (with debounce: 3 samples, 30ms apart)
  Take mutex
  IF PA10 == LOW (RUN):
    mode = RUN
    Set IN1/IN2 per motor_direction
    Start TIM3_CH3 PWM at motor_speed
  ELSE (INIT):
    mode = INIT
    IN1 = LOW, IN2 = LOW
    Stop TIM3_CH3 PWM
  Release mutex
  Delay 100ms
```

### Task_RFID_TTS (200ms cycle)

```
LOOP:
  Take mutex, read mode, release mutex

  IF mode == INIT:
    RC522: pull RST low, delay 10ms, pull RST high, reinit registers
    SYN6288: send init frame (volume config)
    Delay 500ms
    CONTINUE

  IF mode == RUN:
    RC522: Request → Anticoll → Select → Read UID (retry 3x on SPI fail)
    Lookup UID in uid_table[]
    IF match found:
      Wait for SYN6288 BUSY = low (timeout 5s, force reinit)
      Send SYN6288 frame: [0xFD][len_hi][len_lo][GB2312_text][XOR checksum]
    Delay 200ms
```

---

## Data Structures

### UID-to-Name Mapping Table

```c
typedef struct {
    uint32_t uid;
    const char *name_gb2312;
    uint8_t  name_len;          // bytes in GB2312
} uid_entry_t;

const uid_entry_t uid_table[] = {
    {0x12345678, "\xBF\xA8\xC6\xAC\xD5\xC5\xC8\xFD", 8},
    // Extend at compile time
};
#define UID_TABLE_SIZE  (sizeof(uid_table) / sizeof(uid_table[0]))
```

### SYN6288 Frame Format

```
[0xFD] [len_hi] [len_lo] [GB2312_data...] [checksum]
checksum = XOR of len_hi, len_lo, and all GB2312 bytes
```

---

## Configurable Parameters

| Variable | Default | Range | Description |
|---|---|---|---|
| `motor_direction` | 0 | 0/1 | 0=forward, 1=reverse |
| `motor_speed` | 500 | 0~999 | PWM duty cycle |
| `tts_volume` | 5 | 0~10 | SYN6288 volume level |

---

## Error Handling

- **SPI communication failure (RC522)**: retry 3 times, treat as no card present
- **SYN6288 BUSY timeout (5s)**: force reinitialize SYN6288
- **HAL peripheral error**: enter `Error_Handler()` (system halt)
- **Toggle switch debounce**: 3 consistent reads at 30ms intervals before state change
- **Unknown UID**: silently ignore, no TTS broadcast

---

## Build & Toolchain

- Build system: CMake (existing)
- MCU: STM32F103C8/CB
- HAL: STM32Cube HAL (existing)
- RTOS: FreeRTOS CMSIS-OS v1 API (existing)
- Compiler: arm-none-eabi-gcc (existing CMakePresets)
