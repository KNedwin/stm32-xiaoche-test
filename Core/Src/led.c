#include "led.h"

void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = LED1_PIN;
    HAL_GPIO_Init(LED1_PORT, &gpio);

    gpio.Pin = LED2_PIN;
    HAL_GPIO_Init(LED2_PORT, &gpio);

    LED_Off();
}

void LED_On(void)
{
    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
}

void LED_Off(void)
{
    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
}
