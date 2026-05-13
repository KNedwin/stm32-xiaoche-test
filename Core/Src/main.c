/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */

static uint8_t DebounceSwitch(void);
void Task_Motor(void const *argument);
void Task_RFID_TTS(void const *argument);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
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

  /* USER CODE BEGIN RTOS_MUTEX */
  SystemState_Init();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(motorTask, Task_Motor, osPriorityNormal, 0, 128);
  osThreadCreate(osThread(motorTask), NULL);

  osThreadDef(rfidTtsTask, Task_RFID_TTS, osPriorityNormal, 0, 256);
  osThreadCreate(osThread(rfidTtsTask), NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

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
    return (count >= 2) ? 0 : 1;
}

void Task_Motor(void const *argument)
{
    (void)argument;
    SystemMode_t last_mode = SYSTEM_MODE_RUN;

    for (;;) {
        uint8_t sw = DebounceSwitch();

        if (sw == 0) {
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

        uint32_t uid = 0;
        if (RC522_CheckCard(&uid)) {
            if (uid != current_uid) {
                current_uid = uid;
                const uid_entry_t *entry = UIDTable_Lookup(uid);
                if (entry != NULL) {
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

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
