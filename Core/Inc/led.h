#ifndef __LED_H
#define __LED_H

#include "stm32f1xx_hal.h"

#define LED1_PIN    GPIO_PIN_8
#define LED1_PORT   GPIOA
#define LED2_PIN    GPIO_PIN_13
#define LED2_PORT   GPIOC

void LED_Init(void);
void LED_On(void);
void LED_Off(void);

#endif
