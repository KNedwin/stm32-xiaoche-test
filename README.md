# STM32F103 RFID 刷卡语音播报 + 电机控制系统

> 基于 STM32F103 + FreeRTOS 的 UART RFID 读卡、SYN6288 语音播报、H 桥直流电机控制

## 项目概述

- **主控芯片**：STM32F103C8/CB（Cortex-M3，72MHz）
- **实时系统**：FreeRTOS（CMSIS-RTOS v1 封装）
- **时钟源**：HSE 外部 8MHz 晶振 → PLL 9 倍频 → 72MHz
- **构建工具**：STM32Cube VS Code 插件（st-arm-clang + CMake + Ninja）
- **HAL 版本**：STM32Cube FW_F1

### 核心功能

1. **电机控制**：PA11 拨动开关区分运行/初始化模式，H 桥驱动直流电机，支持 4 秒缓启动
2. **RFID 读卡 + 语音播报**：UART RFID 模块读取卡片数据块中的 GB2312 中文，SYN6288 语音合成播报
3. **LED 指示**：刷卡亮灯（PA8/PC13），卡片离开后延时熄灭

---

## 硬件引脚分配

| 外设 | 信号 | 引脚 | 模式 |
|------|------|------|------|
| 拨动开关 | SW_IN | PA11 | GPIO 输入，内部上拉 |
| H 桥电机 | IN1 | PB12 | GPIO 推挽输出 |
| | IN2 | PB13 | GPIO 推挽输出 |
| | PWM | PB0 | TIM3_CH3，复用推挽 |
| UART RFID | TX | PA9 | USART1_TX（115200bps） |
| | RX | PA10 | USART1_RX（115200bps） |
| SYN6288 语音 | TX | PA2 | USART2_TX（9600bps） |
| | RX | PA3 | USART2_RX |
| | BUSY | PA1 | GPIO 输入（播报状态检测） |
| LED 指示 | LED1 | PA8 | GPIO 推挽输出（低电平亮） |
| | LED2 | PC13 | GPIO 推挽输出（低电平亮） |

所有引脚无冲突。

---

## 软件架构

### FreeRTOS 任务

| 任务名 | 栈大小 | 优先级 | 功能 |
|--------|--------|--------|------|
| `Task_Motor` | 128 words | Normal | 读取 PA11，缓启动驱动电机，INIT 时停止 |
| `Task_RFID_TTS` | 256 words | Normal | RFID 状态机、跨区块中文读取、TTS 播报、LED 控制 |

### RFID 读卡状态机

```
NONE（空闲）
  │  中断检测到有卡
  ▼
EXIST（有卡）
  │  发送 ReadBlock(4)
  ▼
WAIT（等待响应）
  │  20ms×2 超时重发
  ▼
RESDATA（收到数据）
  │  有数据 → 拼接到 chinese_data，读下一块
  │  全零块 → 数据完整
  ▼
LEDLIGHT（播报完成）
  │  LED 亮 + SYN6288 播报
  │  轮询 ReadCard 检测卡片是否离开
  ▼
NONE（卡离开，LED 灭）
```

### 跨区块中文读取

- 优先读第 4 区块（官方 APP 写入位置）
- 第 4 区块无数据则回退读第 1 区块
- 逐块拼接，最多 60 字节（30 个 GB2312 汉字）
- 遇到全零区块表示数据结束

### RFID UART 私有协议

- **帧结构**：`[0x7F] [转义载荷]`，载荷中 `0x7F` 双写转义
- **校验**：载荷字节 XOR 异或
- **波特率**：9600 初始化 → 发送切换指令 → 115200 通信
- **指令码**：`0x10` 寻卡、`0x11` 读块、`0xAC` 设波特率
- **接收方式**：串口中断逐字节驱动状态机

### 共享状态（互斥锁保护）

```c
typedef enum {
    SYSTEM_MODE_RUN  = 0,  // 常闭 = 正常运行
    SYSTEM_MODE_INIT = 1,  // 常开 = 初始化设置
} SystemMode_t;

typedef struct {
    SystemMode_t mode;
    uint8_t      motor_direction;  // 0=正向（IN1高/IN2低）, 1=反向
    uint16_t     motor_speed;      // PWM 占空比 0~999
} SystemState_t;
```

---

## 可配置参数

| 参数 | 默认值 | 范围 | 所在文件 | 说明 |
|------|--------|------|----------|------|
| `motor_direction` | 0 | 0/1 | `system_state.c` | 电机转动方向 |
| `motor_speed` | 500 | 0~999 | `system_state.c` | PWM 占空比 |
| `motor_ramp_ms` | 4000 | — | `main.c:Task_Motor` | 缓启动时长（毫秒） |
| `tts_volume` | 5 | 0~10 | `syn6288.h` | 语音播报音量 |

---

## 构建与烧录

**编译**：VS Code + STM32Cube 插件 → `Ctrl+Shift+B`

**烧录**（需连接 ST-LINK）：
- VS Code 快捷键：`Ctrl+Shift+B` → 选择 "Flash (ST-LINK)"
- 命令行：`STM32_Programmer_CLI.exe -c port=SWD -w build/Release/test1.elf -v`

**工具链**：STM32Cube（st-arm-clang）、CMake、Ninja、FreeRTOS、STM32Cube HAL

---

## 项目文件结构

```
├── Core/
│   ├── Inc/
│   │   ├── main.h              # 主头文件
│   │   ├── system_state.h      # 共享状态 + 互斥锁 API
│   │   ├── motor.h             # H 桥电机驱动 API
│   │   ├── rfid_uart.h         # UART RFID 驱动 API
│   │   ├── syn6288.h           # SYN6288 TTS 驱动 API
│   │   ├── led.h               # LED 控制（PA8 + PC13）
│   │   └── uid_table.h         # UID 映射表（备用）
│   └── Src/
│       ├── main.c              # 主程序（HSE 时钟、引脚初始化、任务实现）
│       ├── system_state.c      # 互斥锁访问器
│       ├── motor.c             # PWM + GPIO 电机控制 + 缓启动
│       ├── rfid_uart.c         # 0x7F 帧协议 + 中断状态机 + 中文拼接
│       ├── syn6288.c           # UART 帧发送 + BUSY 检测
│       ├── led.c               # 双引脚 LED 控制
│       └── stm32f1xx_hal_msp.c # USART1/USART2/TIM3 MSP 配置
├── cmake/stm32cubemx/
│   └── CMakeLists.txt          # 构建配置
└── build/Release/
    └── test1.elf               # 编译产物
```
