#include "syn6288.h"
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart2;

#define FRAME_HEAD    0xFD
#define CMD_SPEAK     0x01
#define CMD_VOLUME    0x03
#define MUSIC_NONE    0x00

void SYN6288_Init(void)
{
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

    uint8_t data_len = len + 3;
    uint8_t len_hi = (data_len >> 8) & 0xFF;
    uint8_t len_lo = data_len & 0xFF;

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
