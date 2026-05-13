#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

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
