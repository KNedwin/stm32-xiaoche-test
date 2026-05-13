# STM32 RFID UART + Motor Soft-Start + HSE Clock Upgrade

## Overview

在三项修改现有代码库，参考成品套件（RFID 开源系统 4.0）：

1. **HSI → HSE**：内部时钟切换为外部 8MHz 晶振 + PLL 72MHz
2. **RFID 模块替换**：RC522 (SPI) → UART 私有协议 RFID 模块 + LED 刷卡亮灯延时熄灭
3. **电机缓启动**：PWM 线性斜坡 4 秒加速

---

## 引脚分配变更

| 引脚 | 变更前 | 变更后 | 说明 |
|------|--------|--------|------|
| PA10 | 拨动开关 | USART1_RX | RFID 模块接收 (115200bps) |
| PA9 | 空闲 | USART1_TX | RFID 模块发送 (115200bps) |
| PA11 | 空闲 | 拨动开关 | 替代 PA10 (内部上拉) |
| PA8 | RC522_RST | LED1 | 刷卡亮灯 (推挽输出) |
| PC13 | 空闲 | LED2 | 刷卡亮灯 (推挽输出) |
| PD0 | 未用 | HSE_IN | 外部 8MHz 晶振 |
| PD1 | 未用 | HSE_OUT | 外部 8MHz 晶振 |
| PA4/PA5/PA6/PA7 | RC522 SPI1 | 释放 | 不再需要 |
| PB12/PB13 | 电机 IN1/IN2 | 不变 | — |
| PB0 | 电机 PWM | 不变 | — |
| PA2/PA3 | SYN6288 TX/RX | 不变 | — |
| PA1 | SYN6288 BUSY | 不变 | — |

---

## 文件变更清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `Core/Inc/rfid_uart.h` | UART RFID 头文件 |
| 新增 | `Core/Src/rfid_uart.c` | RFID 私有协议驱动 (0x7F 帧 + 转义 + XOR) |
| 新增 | `Core/Inc/led.h` | LED 控制头文件 |
| 新增 | `Core/Src/led.c` | LED 双引脚控制 |
| 删除 | `Core/Inc/rc522.h` | 不再使用 |
| 删除 | `Core/Src/rc522.c` | 不再使用 |
| 修改 | `Core/Src/main.c` | HSE 时钟、新引脚初始化、新任务逻辑 |
| 修改 | `Core/Inc/main.h` | 更新头文件引用 |
| 修改 | `Core/Src/motor.c` | 新增 Motor_SoftStart() |
| 修改 | `Core/Inc/motor.h` | 新增 Motor_SoftStart() 声明 |
| 修改 | `Core/Src/stm32f1xx_hal_msp.c` | 移除 SPI1 MSP，添加 USART1 MSP |
| 修改 | `cmake/stm32cubemx/CMakeLists.txt` | 替换 rc522.c → rfid_uart.c + led.c |
| 修改 | `Core/Inc/stm32f1xx_hal_conf.h` | 禁用 HAL_SPI_MODULE_ENABLED，启用 HAL_USART_MODULE_ENABLED |

---

## 时钟配置 (HSE + PLL)

`SystemClock_Config()` 修改：

```c
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
RCC_OscInitStruct.HSEState = RCC_HSE_ON;
RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 72MHz
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;     // 36MHz
RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;     // 72MHz
```

TIM3 时钟 = APB1×2 = 72MHz。Prescaler 72-1 = 1MHz，Period 1000-1 = 1kHz PWM，**参数无需修改**。

---

## RFID UART 模块 (rfid_uart.c/h)

### 通信协议

参考成品套件 RFID 模块私有协议：

- **物理层**：USART1, 初始 9600bps → `SetBound115200()` 切换 → 115200bps
- **帧格式**：`[0x7F] [载荷 (转义后)]`，载荷中 `0x7F` 双写转义
- **校验**：载荷字节 XOR 异或

### 核心指令

| 指令码 | 功能 | 载荷 |
|--------|------|------|
| `0x10` | ReadCard 读卡号 | `{len=3, addr=0, cmd=0x10, chk}` |
| `0x11` | ReadBlock 读块 | `{len=4, addr=0, cmd=0x11, block, chk}` |
| `0xAC` | SetBound 波特率切换 | `{len=5, addr=0, cmd=0xAC, 0x00, baud_param, chk}` |

### 中断接收状态机

```
UartReceiveCommand(byte):
  等待 0x7F 帧头
  → len = 载荷长度
  → 逐字节写入 ReceiveBuffer
  → len 归零 → 校验 → 返回结果码

返回值: 0=等待中, 1=帧收完, 2=读卡成功, 3=错误/无卡
```

### 块数据读取与中文拼接

```
chinese_data[60]  缓冲区，最多 30 个 GB2312 汉字
chinese_block_num 当前区块偏移

读取流程:
  chinese_block_num = 0
  → ReadBlock(4)  读取第 4 区块
  → 有数据 → 追加 → chinese_block_num++
  → 继续读 Block 5, 6...
  → 遇到全零块 → 数据完整 → 播报
  → 若 Block 4 无数据 → chinese_block_num = -3
  → 回退读 Block 1 → 直接播报
```

### 驱动接口

```c
void    RFID_UART_Init(void);
void    RFID_UART_RxCallback(uint8_t byte);
uint8_t RFID_GetCardFlag(void);
void    RFID_SetCardFlag(uint8_t flag);
uint32_t RFID_GetCardUID(void);
void    RFID_ReadBlockRaw(uint8_t block, uint8_t *buf16);
void    RFID_SendReadCard(void);
```

---

## LED 模块 (led.c/h)

```c
void LED_Init(void);   // PA8 + PC13, 初始灭
void LED_On(void);     // PA8=0, PC13=0
void LED_Off(void);    // PA8=1, PC13=1
```

---

## 电机缓启动

`Motor_SoftStart(target_speed, ramp_ms)`：

```
每 50ms 递增一次
increment = target_speed / (ramp_ms / 50)
从 0 线性升至 target_speed
```

在 `Task_Motor` RUN 模式启动时调用：`Motor_SoftStart(motor_speed, 4000)`。

---

## Task_RFID_TTS 状态机

```
              ┌─────────┐
              │  NONE   │ 空闲，无卡
              └────┬────┘
                   │ RFID 中断检测到卡
                   ▼
              ┌─────────┐
              │  EXIST  │ 发送 ReadBlock(4+chinese_block_num)
              └────┬────┘ 立即转入 WAIT
                   │
                   ▼
              ┌─────────┐
              │  WAIT   │ 等待响应 (20ms × 2 超时重发)
              └────┬────┘
       收到响应 │  超时 → NONE
                   ▼
              ┌──────────┐
              │ RESDATA  │ 处理块数据
              └────┬─────┘
       数据完整 │  数据不全 → EXIST (继续读下一块)
                   ▼
              ┌──────────┐
              │ LEDLIGHT │ LED_On() + SYN6288_Speak()
              └────┬─────┘ ReadCard() 轮询卡在位
      卡离开 │  递减 led_close_counts
                   ▼
              ┌─────────┐
              │  NONE   │ LED_Off()
              └─────────┘
```

### "一点" 品牌过滤

若 `chinese_data` 前 4 字节为 `{0xD2, 0xBB, 0xB5, 0xE3}`（"一点"），不播报不亮灯。

---

## 可调参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `motor_direction` | 0 | 0=正向, 1=反向 |
| `motor_speed` | 500 | PWM 占空比 0~999 |
| `motor_ramp_ms` | 4000 | 缓启动时长 |
| `tts_volume` | 5 | SYN6288 音量 0~10 |
| `led_close_delay_ms` | 500 | LED 延时关闭 |

---

## 错误处理

- RFID 帧校验失败 → 忽略，等待重发
- RFID 超时 (2×20ms) → 返回 NONE
- SYN6288 BUSY 超时 (5s) → 强制重新初始化
- 拨动开关消抖 → 3 次采样取多数 (30ms 间隔)
