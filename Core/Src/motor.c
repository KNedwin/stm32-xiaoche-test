#include "motor.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

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

/* direction: 0=正向(IN1高/IN2低), 1=反向(IN1低/IN2高) */
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

/* speed: PWM占空比 0~999 (对应0%~100%), 超出自动限制为999 */
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

void Motor_SoftStart(uint16_t target_speed, uint32_t ramp_ms)
{
    if (target_speed > 999) target_speed = 999;
    if (ramp_ms < 50) ramp_ms = 50;

    uint32_t steps = ramp_ms / 50;
    uint16_t current = 0;

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    for (uint32_t s = 0; s < steps; s++) {
        current += target_speed / steps;
        if (current > target_speed) current = target_speed;
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, current);
        osDelay(50);
    }
}
