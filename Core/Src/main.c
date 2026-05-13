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

extern UART_HandleTypeDef huart1;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t byte;
        HAL_UART_Receive_IT(&huart1, &byte, 1);
        RFID_UART_RxCallback(byte);
    }
}

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

  extern UART_HandleTypeDef huart1;
  extern UART_HandleTypeDef huart2;

  /* Toggle switch: PA11 input with pull-up */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef gpio_switch = {0};
  gpio_switch.Pin = GPIO_PIN_11;
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

  /* SYN6288 BUSY(PA1) */
  GPIO_InitTypeDef gpio_busy = {0};
  gpio_busy.Pin = GPIO_PIN_1;
  gpio_busy.Mode = GPIO_MODE_INPUT;
  gpio_busy.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio_busy);

  /* USART2 init (SYN6288) */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  HAL_UART_Init(&huart2);

  /* Initialize drivers */
  LED_Init();
  Motor_Init();
  RFID_UART_Init();
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

static uint8_t DebounceSwitch(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == GPIO_PIN_RESET) {
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
                Motor_SoftStart(spd, 4000);
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
    uint32_t card_wait_ms = 0;
    uint8_t  resend_count = 0;
    uint8_t  block_data[16];
    uint8_t  led_close_count = 0;

    for (;;) {
        SystemMode_t mode = SystemState_GetMode();

        if (mode == SYSTEM_MODE_INIT) {
            LED_Off();
            RFID_SetCardFlag(CARD_FLAG_NONE);
            RFID_SetChineseBlockNum(0);
            card_wait_ms = 0;
            resend_count = 0;
            osDelay(500);
            continue;
        }

        uint8_t flag = RFID_GetCardFlag();

        switch (flag) {

        case CARD_FLAG_NONE:
            LED_Off();
            RFID_ClearChineseData();
            RFID_SetChineseBlockNum(0);
            card_wait_ms = 0;
            resend_count = 0;
            osDelay(10);
            break;

        case CARD_FLAG_EXIST:
            RFID_SetChineseBlockNum(0);
            RFID_SendReadBlock(4);
            RFID_SetCardFlag(CARD_FLAG_WAIT);
            card_wait_ms = 0;
            resend_count = 0;
            osDelay(1);
            break;

        case CARD_FLAG_WAIT:
            card_wait_ms++;
            if (card_wait_ms >= 20) {
                card_wait_ms = 0;
                resend_count++;
                if (resend_count > 2) {
                    RFID_SetCardFlag(CARD_FLAG_NONE);
                } else {
                    uint8_t blk = 4;
                    int8_t bn = (int8_t)RFID_GetChineseBlockNum();
                    if (bn < 0) blk = 1;
                    else if (bn > 0) blk = 4 + bn;
                    RFID_SendReadBlock(blk);
                }
            }
            osDelay(1);
            break;

        case CARD_FLAG_RESDATA: {
            RFID_GetBlockData(block_data);
            int8_t bn = (int8_t)RFID_GetChineseBlockNum();

            if (bn >= 0 && block_data[0] != 0x00) {
                /* Block has data — append and continue reading */
                RFID_AppendChineseData(block_data, 16);
                RFID_SetChineseBlockNum(bn + 1);
                RFID_SendReadBlock(4 + bn + 1);
                RFID_SetCardFlag(CARD_FLAG_WAIT);
                card_wait_ms = 0;
                resend_count = 0;
            } else if (bn == 0 && block_data[0] == 0x00) {
                /* Block 4 empty — fallback to Block 1 */
                RFID_SetChineseBlockNum(-3);
                RFID_SendReadBlock(1);
                RFID_SetCardFlag(CARD_FLAG_WAIT);
                card_wait_ms = 0;
                resend_count = 0;
            } else {
                /* Data complete or Block 1 also empty */
                uint8_t *data = RFID_GetChineseData();
                if (data[0] != 0x00) {
                    LED_On();
                    uint32_t tmo = 5000;
                    while (SYN6288_IsBusy() && tmo > 0) { osDelay(10); tmo -= 10; }
                    if (tmo == 0) SYN6288_Init();

                    uint8_t dlen = (bn > 0) ? (uint8_t)(bn * 16) : 16;
                    SYN6288_Speak(data, dlen);

                    RFID_SetCardFlag(CARD_FLAG_LEDLIGHT);
                    led_close_count = 10;
                } else {
                    RFID_SetCardFlag(CARD_FLAG_NONE);
                }
            }
            osDelay(1);
            break;
        }

        case CARD_FLAG_LEDLIGHT:
            LED_On();
            RFID_SendReadCard();
            if (RFID_GetCardFlag() == CARD_FLAG_LEDLIGHT) {
                led_close_count--;
                if (led_close_count == 0) {
                    RFID_SetCardFlag(CARD_FLAG_NONE);
                }
            }
            osDelay(10);
            break;

        default:
            osDelay(10);
            break;
        }
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
