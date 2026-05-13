#ifndef __SYN6288_H
#define __SYN6288_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define SYN6288_USART      USART2
#define SYN6288_BUSY_PIN   GPIO_PIN_1
#define SYN6288_BUSY_PORT  GPIOA

void SYN6288_Init(void);
void SYN6288_SetVolume(uint8_t vol);
void SYN6288_Speak(const uint8_t *text, uint8_t len);
uint8_t SYN6288_IsBusy(void);

#endif
