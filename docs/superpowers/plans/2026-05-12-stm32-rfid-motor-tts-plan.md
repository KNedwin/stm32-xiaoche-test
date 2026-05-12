# STM32 RFID Motor + TTS Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a dual-task FreeRTOS system on STM32F103 that controls a DC motor via H-bridge when PA10 is closed, and reads RC522 card UIDs to broadcast GB2312 names through SYN6288 TTS.

**Architecture:** Two FreeRTOS tasks sharing mutex-protected state. Task_Motor reads PA10 toggle switch and drives motor direction/PWM. Task_RFID_TTS polls RC522 via SPI1, looks up UIDs in a hardcoded table, and sends GB2312 text to SYN6288 via USART2. PA10 open = INIT mode (stop motor, reinit peripherals).

**Tech Stack:** STM32Cube HAL, FreeRTOS (CMSIS-OS v1), arm-none-eabi-gcc, CMake

---

## File Map

| File | Action | Purpose |
|------|--------|---------|
| `Core/Inc/system_state.h` | Create | Shared state type, mutex handle, accessor APIs |
| `Core/Src/system_state.c` | Create | Mutex creation, mode/direction/speed getters/setters |
| `Core/Inc/motor.h` | Create | Motor driver API + pin definitions |
| `Core/Src/motor.c` | Create | H-bridge control via GPIO + TIM3_CH3 PWM |
| `Core/Inc/rc522.h` | Create | RC522 driver API + SPI pin defs |
| `Core/Src/rc522.c` | Create | SPI1 RC522 register ops, ISO14443-A state machine |
| `Core/Inc/syn6288.h` | Create | SYN6288 driver API + USART2 pin defs |
| `Core/Src/syn6288.c` | Create | UART frame send, volume control, BUSY check |
| `Core/Inc/uid_table.h` | Create | UID entry struct + table declaration |
| `Core/Src/uid_table.c` | Create | Hardcoded UID-to-GB2312-name mapping |
| `Core/Src/stm32f1xx_hal_msp.c` | Modify | Add SPI1, USART2, TIM3, GPIO MSP init |
| `Core/Inc/main.h` | Modify | Add shared includes |
| `Core/Src/main.c` | Modify | Init peripherals, create mutex + tasks |
| `cmake/stm32cubemx/CMakeLists.txt` | Modify | Add new sources + HAL drivers (SPI, UART, TIM) |

---

### Task 1: Initialize Git Repository

- [ ] **Step 1: Init git repo**

```bash
cd "c:/Users/KN/Desktop/test1" && git init
```

Expected: `Initialized empty Git repository in ...`

- [ ] **Step 2: Create .gitignore**

Write `C:/Users/KN/Desktop/test1/.gitignore`:

```
build/
.mxproject
.vs/
*.o
*.d
*.elf
*.hex
*.bin
*.map
```

- [ ] **Step 3: Initial commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add -A && git commit -m "$(cat <<'EOF'
Initial commit: STM32F103 FreeRTOS project template

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 2: Add Required HAL Drivers to Build

**Files:**
- Modify: `cmake/stm32cubemx/CMakeLists.txt`

The STM32_Drivers_Src list currently lacks SPI, UART, and TIM drivers. We need to add them.

- [ ] **Step 1: Add SPI, UART, and TIM HAL driver sources**

In `cmake/stm32cubemx/CMakeLists.txt`, find the `STM32_Drivers_Src` block (lines 35-48) and add these entries before the closing `)`:

Edit the file, changing the STM32_Drivers_Src list from:
```
set(STM32_Drivers_Src
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
)
```
to:
```
set(STM32_Drivers_Src
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
)
```

- [ ] **Step 2: Add new application sources to MX_Application_Src**

In the same file, modify the `MX_Application_Src` block (lines 24-32), adding new sources before the closing `)`:

```
set(MX_Application_Src
    ...
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32f103xb.s
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/motor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/rc522.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syn6288.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/uid_table.c
)
```

- [ ] **Step 3: Verify build configuration parses**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake -S . -B build -G "Unix Makefiles" --preset Debug 2>&1 | tail -5
```

Expected: CMake configures without errors (may show toolchain warnings if not fully set up, but no cmake errors).

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add cmake/stm32cubemx/CMakeLists.txt && git commit -m "$(cat <<'EOF'
build: add SPI, UART, TIM HAL drivers and new app sources

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 3: System State Module

**Files:**
- Create: `Core/Inc/system_state.h`
- Create: `Core/Src/system_state.c`

- [ ] **Step 1: Create system_state.h**

Write `C:/Users/KN/Desktop/test1/Core/Inc/system_state.h`:

```c
#ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#include "cmsis_os.h"

typedef enum {
    SYSTEM_MODE_RUN  = 0,
    SYSTEM_MODE_INIT = 1,
} SystemMode_t;

typedef struct {
    SystemMode_t mode;
    uint8_t      motor_direction;
    uint16_t     motor_speed;
} SystemState_t;

extern SystemState_t system_state;
extern osMutexId state_mutex_handle;

void SystemState_Init(void);
void SystemState_SetMode(SystemMode_t mode);
SystemMode_t SystemState_GetMode(void);
void SystemState_SetMotorDirection(uint8_t direction);
uint8_t SystemState_GetMotorDirection(void);
void SystemState_SetMotorSpeed(uint16_t speed);
uint16_t SystemState_GetMotorSpeed(void);

#endif
```

- [ ] **Step 2: Create system_state.c**

Write `C:/Users/KN/Desktop/test1/Core/Src/system_state.c`:

```c
#include "system_state.h"

SystemState_t system_state = {
    .mode = SYSTEM_MODE_RUN,
    .motor_direction = 0,
    .motor_speed = 500,
};

osMutexId state_mutex_handle;
osMutexDef(state_mutex);

void SystemState_Init(void)
{
    state_mutex_handle = osMutexCreate(osMutex(state_mutex));
}

void SystemState_SetMode(SystemMode_t mode)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.mode = mode;
    osMutexRelease(state_mutex_handle);
}

SystemMode_t SystemState_GetMode(void)
{
    SystemMode_t m;
    osMutexWait(state_mutex_handle, osWaitForever);
    m = system_state.mode;
    osMutexRelease(state_mutex_handle);
    return m;
}

void SystemState_SetMotorDirection(uint8_t direction)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.motor_direction = direction;
    osMutexRelease(state_mutex_handle);
}

uint8_t SystemState_GetMotorDirection(void)
{
    uint8_t d;
    osMutexWait(state_mutex_handle, osWaitForever);
    d = system_state.motor_direction;
    osMutexRelease(state_mutex_handle);
    return d;
}

void SystemState_SetMotorSpeed(uint16_t speed)
{
    osMutexWait(state_mutex_handle, osWaitForever);
    system_state.motor_speed = speed;
    osMutexRelease(state_mutex_handle);
}

uint16_t SystemState_GetMotorSpeed(void)
{
    uint16_t s;
    osMutexWait(state_mutex_handle, osWaitForever);
    s = system_state.motor_speed;
    osMutexRelease(state_mutex_handle);
    return s;
}
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -20
```

Expected: system_state.c compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/system_state.h Core/Src/system_state.c && git commit -m "$(cat <<'EOF'
feat: add system state module with mutex-protected accessors

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 4: Motor Driver (H-Bridge)

**Files:**
- Create: `Core/Inc/motor.h`
- Create: `Core/Src/motor.c`

- [ ] **Step 1: Create motor.h**

Write `C:/Users/KN/Desktop/test1/Core/Inc/motor.h`:

```c
#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

#define MOTOR_IN1_PIN    GPIO_PIN_12
#define MOTOR_IN1_PORT   GPIOB
#define MOTOR_IN2_PIN    GPIO_PIN_13
#define MOTOR_IN2_PORT   GPIOB
#define MOTOR_PWM_PIN    GPIO_PIN_0
#define MOTOR_PWM_PORT   GPIOB

void Motor_Init(void);
void Motor_SetDirection(uint8_t direction);
void Motor_SetSpeed(uint16_t speed);
void Motor_Start(void);
void Motor_Stop(void);

#endif
```

- [ ] **Step 2: Create motor.c**

Write `C:/Users/KN/Desktop/test1/Core/Src/motor.c`:

```c
#include "motor.h"
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef htim3;

void Motor_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    TIM_OC_InitTypeDef sConfig = {0};
    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.Pulse = 0;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfig, TIM_CHANNEL_3);

    HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
}

void Motor_SetDirection(uint8_t direction)
{
    if (direction == 0) {
        HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN, GPIO_PIN_SET);
    }
}

void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999) speed = 999;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, speed);
}

void Motor_Start(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

void Motor_Stop(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_GPIO_WritePin(MOTOR_IN1_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN2_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
}
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -20
```

Expected: motor.c compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/motor.h Core/Src/motor.c && git commit -m "$(cat <<'EOF'
feat: add H-bridge motor driver with direction and PWM speed control

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 5: RC522 RFID Driver

**Files:**
- Create: `Core/Inc/rc522.h`
- Create: `Core/Src/rc522.c`

- [ ] **Step 1: Create rc522.h**

Write `C:/Users/KN/Desktop/test1/Core/Inc/rc522.h`:

```c
#ifndef __RC522_H
#define __RC522_H

#include <stdint.h>

#define RC522_SPI         SPI1
#define RC522_NSS_PIN     GPIO_PIN_4
#define RC522_NSS_PORT    GPIOA
#define RC522_RST_PIN     GPIO_PIN_8
#define RC522_RST_PORT    GPIOA

void RC522_Init(void);
void RC522_HardReset(void);
uint8_t RC522_CheckCard(uint32_t *uid_out);

#endif
```

- [ ] **Step 2: Create rc522.c**

Write `C:/Users/KN/Desktop/test1/Core/Src/rc522.c`:

```c
#include "rc522.h"
#include "stm32f1xx_hal.h"

/* RC522 register addresses */
#define REG_COMMAND      0x01
#define REG_COMIEN       0x02
#define REG_DIVIEN       0x03
#define REG_COMIRQ       0x04
#define REG_DIVIRQ       0x05
#define REG_ERROR        0x06
#define REG_FIFODATA     0x09
#define REG_FIFOLEVEL    0x0A
#define REG_CONTROL      0x0C
#define REG_BITFRAMING   0x0D
#define REG_MODE         0x11
#define REG_TXMODE       0x12
#define REG_RXMODE       0x13
#define REG_TXCONTROL    0x14
#define REG_TXAUTO       0x15
#define REG_COLL         0x0E
#define REG_TMODE        0x2A
#define REG_TPRESCALER   0x2B
#define REG_TRELOADH     0x2C
#define REG_TRELOADL     0x2D
#define REG_VERSION      0x37

/* Commands */
#define PCD_IDLE         0x00
#define PCD_CALCCRC      0x03
#define PCD_TRANSCEIVE   0x0C
#define PCD_SOFTRESET    0x0F

/* PICC commands */
#define PICC_REQIDL      0x26
#define PICC_ANTICOLL    0x93
#define PICC_SELECTTAG   0x93

extern SPI_HandleTypeDef hspi1;

static void RC522_WriteReg(uint8_t addr, uint8_t val)
{
    uint8_t buf[2];
    buf[0] = (addr << 1) & 0x7E;
    buf[1] = val;
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_SET);
}

static uint8_t RC522_ReadReg(uint8_t addr)
{
    uint8_t tx[2], rx[2];
    tx[0] = ((addr << 1) & 0x7E) | 0x80;
    tx[1] = 0x00;
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_PIN_SET);
    return rx[1];
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t val = RC522_ReadReg(reg);
    RC522_WriteReg(reg, val | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t val = RC522_ReadReg(reg);
    RC522_WriteReg(reg, val & ~mask);
}

static void RC522_FlushFIFO(void)
{
    RC522_WriteReg(REG_FIFOLEVEL, 0x80);
}

static uint8_t RC522_ToCard(uint8_t cmd, uint8_t *send, uint8_t send_len,
                            uint8_t *recv, uint8_t *recv_len)
{
    uint8_t irq = 0x00;
    uint8_t n;

    if (cmd == PCD_TRANSCEIVE) {
        irq = 0x30; /* TxIRq | RxIRq */
    }

    RC522_WriteReg(REG_COMIEN, 0x7F & ~0x20);
    RC522_WriteReg(REG_COMIRQ, 0x7F);
    RC522_FlushFIFO();
    RC522_SetBitMask(REG_FIFOLEVEL, 0x80);

    RC522_WriteReg(REG_COMMAND, PCD_IDLE);
    for (uint8_t i = 0; i < send_len; i++) {
        RC522_WriteReg(REG_FIFODATA, send[i]);
    }
    RC522_WriteReg(REG_COMMAND, cmd);
    if (cmd == PCD_TRANSCEIVE) {
        RC522_SetBitMask(REG_BITFRAMING, 0x80);
    }

    uint16_t timeout = 2000;
    do {
        n = RC522_ReadReg(REG_COMIRQ);
        timeout--;
    } while ((timeout > 0) && !(n & irq) && !(n & 0x01));

    RC522_ClearBitMask(REG_BITFRAMING, 0x80);

    if (timeout == 0 || (RC522_ReadReg(REG_ERROR) & 0x1B)) {
        return 0;
    }

    if (n & irq) {
        uint8_t status = RC522_ReadReg(REG_ERROR);
        if (status & 0x1B) return 0;
    }

    if (n & 0x01) return 0; /* TimerIRq */

    uint8_t fifo_bytes = RC522_ReadReg(REG_FIFOLEVEL);
    if (*recv_len < fifo_bytes) fifo_bytes = *recv_len;
    for (uint8_t i = 0; i < fifo_bytes; i++) {
        recv[i] = RC522_ReadReg(REG_FIFODATA);
    }
    *recv_len = fifo_bytes;
    return 1;
}

void RC522_Init(void)
{
    RC522_HardReset();

    RC522_WriteReg(REG_TMODE, 0x8D);
    RC522_WriteReg(REG_TPRESCALER, 0x3E);
    RC522_WriteReg(REG_TRELOADL, 30);
    RC522_WriteReg(REG_TRELOADH, 0);

    RC522_WriteReg(REG_TXAUTO, 0x40);
    RC522_WriteReg(REG_MODE, 0x3D);

    RC522_ClearBitMask(REG_TXCONTROL, 0x03);

    RC522_WriteReg(REG_RXMODE, 0x00);
    RC522_WriteReg(REG_TXMODE, 0x00);
}

void RC522_HardReset(void)
{
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    RC522_WriteReg(REG_COMMAND, PCD_SOFTRESET);
    HAL_Delay(10);

    while (RC522_ReadReg(REG_COMMAND) & 0x10) { /* wait for PowerDown bit clear */ }
}

uint8_t RC522_CheckCard(uint32_t *uid_out)
{
    uint8_t recv[16];
    uint8_t recv_len;
    uint8_t send[2];

    /* REQA */
    send[0] = PICC_REQIDL;
    send[1] = 0x00;
    recv_len = 2;
    RC522_ClearBitMask(REG_COLL, 0x80);
    if (!RC522_ToCard(PCD_TRANSCEIVE, send, 1, recv, &recv_len)) {
        return 0;
    }
    if (recv_len != 2) return 0;

    /* ANTICOLL */
    send[0] = PICC_ANTICOLL;
    send[1] = 0x20;
    recv_len = 8;
    RC522_ClearBitMask(REG_COLL, 0x80);
    if (!RC522_ToCard(PCD_TRANSCEIVE, send, 2, recv, &recv_len)) {
        return 0;
    }
    if (recv_len != 5) return 0;

    /* Verify BCC */
    uint8_t bcc = recv[0] ^ recv[1] ^ recv[2] ^ recv[3];
    if (bcc != recv[4]) return 0;

    *uid_out = ((uint32_t)recv[0] << 24)
             | ((uint32_t)recv[1] << 16)
             | ((uint32_t)recv[2] << 8)
             |  (uint32_t)recv[3];

    /* SELECT */
    send[0] = PICC_SELECTTAG;
    send[1] = 0x70;
    for (uint8_t i = 0; i < 5; i++) send[i + 2] = recv[i];
    /* CRC */
    uint8_t crc_send[9];
    for (uint8_t i = 0; i < 7; i++) crc_send[i] = send[i];
    recv_len = 8;
    RC522_ClearBitMask(REG_COLL, 0x80);
    RC522_ToCard(PCD_TRANSCEIVE, crc_send, 7, recv, &recv_len);

    return 1;
}
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -30
```

Expected: rc522.c compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/rc522.h Core/Src/rc522.c && git commit -m "$(cat <<'EOF'
feat: add RC522 RFID driver for ISO14443-A card UID reading

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 6: SYN6288 TTS Driver

**Files:**
- Create: `Core/Inc/syn6288.h`
- Create: `Core/Src/syn6288.c`

- [ ] **Step 1: Create syn6288.h**

Write `C:/Users/KN/Desktop/test1/Core/Inc/syn6288.h`:

```c
#ifndef __SYN6288_H
#define __SYN6288_H

#include <stdint.h>

#define SYN6288_USART      USART2
#define SYN6288_BUSY_PIN   GPIO_PIN_1
#define SYN6288_BUSY_PORT  GPIOA

void SYN6288_Init(void);
void SYN6288_SetVolume(uint8_t vol);
void SYN6288_Speak(const uint8_t *text, uint8_t len);
uint8_t SYN6288_IsBusy(void);

#endif
```

- [ ] **Step 2: Create syn6288.c**

Write `C:/Users/KN/Desktop/test1/Core/Src/syn6288.c`:

```c
#include "syn6288.h"
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart2;

#define FRAME_HEAD    0xFD
#define CMD_SPEAK     0x01
#define CMD_VOLUME    0x03
#define MUSIC_NONE    0x00

void SYN6288_Init(void)
{
    /* Send initial config: background music off */
    uint8_t init_frame[] = {FRAME_HEAD, 0x00, 0x03, CMD_SPEAK, MUSIC_NONE, 0x00, 0x00};
    uint8_t checksum = 0;
    for (uint8_t i = 1; i < 5; i++) checksum ^= init_frame[i];
    init_frame[5] = checksum;
    HAL_UART_Transmit(&huart2, init_frame, sizeof(init_frame), HAL_MAX_DELAY);
    HAL_Delay(200);

    SYN6288_SetVolume(5);
}

void SYN6288_SetVolume(uint8_t vol)
{
    if (vol > 10) vol = 10;
    uint8_t frame[] = {FRAME_HEAD, 0x00, 0x03, CMD_VOLUME, vol, 0x00};
    uint8_t checksum = 0;
    for (uint8_t i = 1; i < 5; i++) checksum ^= frame[i];
    frame[5] = checksum;
    HAL_UART_Transmit(&huart2, frame, sizeof(frame), HAL_MAX_DELAY);
    HAL_Delay(50);
}

void SYN6288_Speak(const uint8_t *text, uint8_t len)
{
    if (len == 0 || len > 200) return;

    uint8_t data_len = len + 3; /* cmd(1) + music(1) + text(len) + 1 reserved */
    uint8_t len_hi = (data_len >> 8) & 0xFF;
    uint8_t len_lo = data_len & 0xFF;

    /* Build frame */
    uint8_t frame[256];
    uint8_t idx = 0;
    frame[idx++] = FRAME_HEAD;
    frame[idx++] = len_hi;
    frame[idx++] = len_lo;
    frame[idx++] = CMD_SPEAK;
    frame[idx++] = MUSIC_NONE;

    for (uint8_t i = 0; i < len; i++) {
        frame[idx++] = text[i];
    }

    uint8_t checksum = 0;
    for (uint8_t i = 1; i < idx; i++) checksum ^= frame[i];
    frame[idx++] = checksum;

    HAL_UART_Transmit(&huart2, frame, idx, HAL_MAX_DELAY);
}

uint8_t SYN6288_IsBusy(void)
{
    return (HAL_GPIO_ReadPin(SYN6288_BUSY_PORT, SYN6288_BUSY_PIN) == GPIO_PIN_SET) ? 1 : 0;
}
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -20
```

Expected: syn6288.c compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/syn6288.h Core/Src/syn6288.c && git commit -m "$(cat <<'EOF'
feat: add SYN6288 TTS driver with GB2312 frame building

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 7: UID-to-Name Mapping Table

**Files:**
- Create: `Core/Inc/uid_table.h`
- Create: `Core/Src/uid_table.c`

- [ ] **Step 1: Create uid_table.h**

Write `C:/Users/KN/Desktop/test1/Core/Inc/uid_table.h`:

```c
#ifndef __UID_TABLE_H
#define __UID_TABLE_H

#include <stdint.h>

typedef struct {
    uint32_t uid;
    const uint8_t *name_gb2312;
    uint8_t  name_len;
} uid_entry_t;

extern const uid_entry_t uid_table[];
extern const uint8_t uid_table_size;

const uid_entry_t *UIDTable_Lookup(uint32_t uid);

#endif
```

- [ ] **Step 2: Create uid_table.c**

Write `C:/Users/KN/Desktop/test1/Core/Src/uid_table.c`:

```c
#include "uid_table.h"

/*
 * GB2312-encoded name table.
 * Add entries here. Example encoding:
 *   卡片张三 = {0xBF, 0xA8, 0xC6, 0xAC, 0xD5, 0xC5, 0xC8, 0xFD}
 */
const uid_entry_t uid_table[] = {
    /* {UID (hex), GB2312 bytes, byte length} */
};

const uint8_t uid_table_size = sizeof(uid_table) / sizeof(uid_table[0]);

const uid_entry_t *UIDTable_Lookup(uint32_t uid)
{
    for (uint8_t i = 0; i < uid_table_size; i++) {
        if (uid_table[i].uid == uid) {
            return &uid_table[i];
        }
    }
    return NULL;
}
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -20
```

Expected: uid_table.c compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/uid_table.h Core/Src/uid_table.c && git commit -m "$(cat <<'EOF'
feat: add UID-to-GB2312-name lookup table

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 8: MSP Configuration (Peripheral Pin Init)

**Files:**
- Modify: `Core/Src/stm32f1xx_hal_msp.c`

We need to add `HAL_SPI_MspInit`, `HAL_UART_MspInit`, `HAL_TIM_PWM_MspInit`, and GPIO init for the switch, motor IN1/IN2, RC522 NSS/RST, SYN6288 BUSY.

Read the existing file first to find USER CODE sections.

- [ ] **Step 1: Read existing MSP file**

Read `C:/Users/KN/Desktop/test1/Core/Src/stm32f1xx_hal_msp.c`.

- [ ] **Step 2: Add MSP implementations**

Find the `/* USER CODE BEGIN 0 */` section and add peripheral handle declarations after it:

```c
/* USER CODE BEGIN 0 */

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* USER CODE END 0 */
```

Then find the `/* USER CODE BEGIN 1 */` section (near end of file) and add these functions:

```c
/* USER CODE BEGIN 1 */

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {0};
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;

        gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7; /* SCK, MOSI */
        HAL_GPIO_Init(GPIOA, &gpio);

        gpio.Pin = GPIO_PIN_6; /* MISO */
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {0};
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;

        gpio.Pin = GPIO_PIN_2; /* TX */
        HAL_GPIO_Init(GPIOA, &gpio);

        gpio.Pin = GPIO_PIN_3; /* RX */
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = GPIO_PIN_0; /* CH3 */
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
}

/* USER CODE END 1 */
```

- [ ] **Step 3: Verify compilation**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -30
```

Expected: Compiles without errors.

- [ ] **Step 4: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Src/stm32f1xx_hal_msp.c && git commit -m "$(cat <<'EOF'
feat: add SPI1, USART2, TIM3 MSP initialization

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 9: Main Integration — Wire Everything Together

**Files:**
- Modify: `Core/Inc/main.h`
- Modify: `Core/Src/main.c`

- [ ] **Step 1: Add includes to main.h**

Read `C:/Users/KN/Desktop/test1/Core/Inc/main.h`, find `/* USER CODE BEGIN Includes */` and add:

```c
/* USER CODE BEGIN Includes */

#include "system_state.h"
#include "motor.h"
#include "rc522.h"
#include "syn6288.h"
#include "uid_table.h"

/* USER CODE END Includes */
```

- [ ] **Step 2: Add peripheral init to main.c — USER CODE BEGIN 2**

Read `C:/Users/KN/Desktop/test1/Core/Src/main.c`, find `/* USER CODE BEGIN 2 */` and add:

```c
  /* USER CODE BEGIN 2 */

  extern SPI_HandleTypeDef hspi1;
  extern UART_HandleTypeDef huart2;

  /* Switch GPIO: PA10 input with pull-up */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef gpio_switch = {0};
  gpio_switch.Pin = GPIO_PIN_10;
  gpio_switch.Mode = GPIO_MODE_INPUT;
  gpio_switch.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpio_switch);

  /* Motor GPIO: PB12(IN1), PB13(IN2) */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef gpio_motor = {0};
  gpio_motor.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  gpio_motor.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_motor.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gpio_motor);

  /* RC522 NSS(PA4) and RST(PA8) */
  GPIO_InitTypeDef gpio_rc522 = {0};
  gpio_rc522.Pin = GPIO_PIN_4 | GPIO_PIN_8;
  gpio_rc522.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_rc522.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &gpio_rc522);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

  /* SYN6288 BUSY(PA1) */
  GPIO_InitTypeDef gpio_busy = {0};
  gpio_busy.Pin = GPIO_PIN_1;
  gpio_busy.Mode = GPIO_MODE_INPUT;
  gpio_busy.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio_busy);

  /* SPI1 init */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  HAL_SPI_Init(&hspi1);

  /* USART2 init */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  HAL_UART_Init(&huart2);

  /* Initialize drivers */
  Motor_Init();
  RC522_Init();
  SYN6288_Init();

  /* USER CODE END 2 */
```

- [ ] **Step 3: Add mutex creation in USER CODE BEGIN RTOS_MUTEX**

Find `/* USER CODE BEGIN RTOS_MUTEX */` and add:

```c
  /* USER CODE BEGIN RTOS_MUTEX */
  SystemState_Init();
  /* USER CODE END RTOS_MUTEX */
```

- [ ] **Step 4: Replace default task with Motor and RFID_TTS tasks**

Find `/* USER CODE BEGIN RTOS_THREADS */` and add:

```c
  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(motorTask, Task_Motor, osPriorityNormal, 0, 128);
  osThreadCreate(osThread(motorTask), NULL);

  osThreadDef(rfidTtsTask, Task_RFID_TTS, osPriorityNormal, 0, 256);
  osThreadCreate(osThread(rfidTtsTask), NULL);
  /* USER CODE END RTOS_THREADS */
```

- [ ] **Step 5: Add task function implementations in USER CODE BEGIN 4**

Find `/* USER CODE BEGIN 4 */` and add before it:

```c
/* USER CODE BEGIN 4 */

static uint8_t DebounceSwitch(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_RESET) {
            count++;
        }
        osDelay(30);
    }
    return (count >= 2) ? 0 : 1; /* 0=low(RUN), 1=high(INIT) */
}

void Task_Motor(void const *argument)
{
    (void)argument;
    SystemMode_t last_mode = SYSTEM_MODE_RUN;

    for (;;) {
        uint8_t sw = DebounceSwitch();

        if (sw == 0) {
            /* RUN mode: motor on */
            SystemState_SetMode(SYSTEM_MODE_RUN);
            if (last_mode != SYSTEM_MODE_RUN) {
                uint8_t dir = SystemState_GetMotorDirection();
                uint16_t spd = SystemState_GetMotorSpeed();
                Motor_SetDirection(dir);
                Motor_SetSpeed(spd);
                Motor_Start();
                last_mode = SYSTEM_MODE_RUN;
            }
        } else {
            /* INIT mode: motor off, reinit */
            SystemState_SetMode(SYSTEM_MODE_INIT);
            if (last_mode != SYSTEM_MODE_INIT) {
                Motor_Stop();
                last_mode = SYSTEM_MODE_INIT;
            }
        }
        osDelay(100);
    }
}

void Task_RFID_TTS(void const *argument)
{
    (void)argument;
    uint32_t current_uid = 0;

    for (;;) {
        SystemMode_t mode = SystemState_GetMode();

        if (mode == SYSTEM_MODE_INIT) {
            RC522_HardReset();
            RC522_Init();
            SYN6288_Init();
            current_uid = 0;
            osDelay(500);
            continue;
        }

        /* RUN mode: poll for card */
        uint32_t uid = 0;
        if (RC522_CheckCard(&uid)) {
            if (uid != current_uid) {
                current_uid = uid;
                const uid_entry_t *entry = UIDTable_Lookup(uid);
                if (entry != NULL) {
                    /* Wait for TTS ready */
                    uint32_t busy_timeout = 5000;
                    while (SYN6288_IsBusy() && busy_timeout > 0) {
                        osDelay(10);
                        busy_timeout -= 10;
                    }
                    if (busy_timeout == 0) {
                        SYN6288_Init();
                    }
                    SYN6288_Speak(entry->name_gb2312, entry->name_len);
                }
            }
        } else {
            current_uid = 0;
        }
        osDelay(200);
    }
}

/* USER CODE END 4 */
```

- [ ] **Step 6: Verify full build**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1 | tail -30
```

Expected: Full build succeeds without errors or warnings.

- [ ] **Step 7: Commit**

```bash
cd "c:/Users/KN/Desktop/test1" && git add Core/Inc/main.h Core/Src/main.c && git commit -m "$(cat <<'EOF'
feat: integrate motor and RFID-TTS tasks with shared state

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

---

### Task 10: Forward Declaration Fix (if needed)

The `hspi1` and `huart2` handles are declared in `stm32f1xx_hal_msp.c` but used in `rc522.c` and `syn6288.c`. Verify the build passes and if the linker complains about missing symbols, add extern declarations.

- [ ] **Step 1: Build and check for errors**

```bash
cd "c:/Users/KN/Desktop/test1" && cmake --build build 2>&1
```

Expected: No build errors.

- [ ] **Step 2: If linker errors about hspi1/huart2, fix and recommit**

If errors occur, ensure `rc522.c` has `extern SPI_HandleTypeDef hspi1;` (already in the provided code) and `syn6288.c` has `extern UART_HandleTypeDef huart2;` (already in the provided code).

- [ ] **Step 3: Commit any fixes**

```bash
cd "c:/Users/KN/Desktop/test1" && git add -A && git commit -m "$(cat <<'EOF'
fix: resolve symbol declarations for SPI and UART handles

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```
