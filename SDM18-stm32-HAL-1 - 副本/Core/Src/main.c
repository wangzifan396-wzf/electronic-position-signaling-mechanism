/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 电机引脚定义（PA5, PA6, PA7）
#define MOTOR_EN_PIN   GPIO_PIN_5
#define MOTOR_STP_PIN  GPIO_PIN_6
#define MOTOR_DIR_PIN  GPIO_PIN_7
#define MOTOR_PORT     GPIOA

// 距离阈值（单位：mm）
#define DISTANCE_NEAR   800   // 小于800mm正转
#define DISTANCE_FAR    1500  // 大于1500mm反转

// 电机状态
#define MOTOR_STOP      0
#define MOTOR_FORWARD   1
#define MOTOR_BACKWARD  2
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
// 电机控制变量
__IO int32_t pulse_count = 0;           // 脉冲计数
__IO uint8_t motor_status = MOTOR_STOP;  // 电机状态
__IO uint32_t last_pulse_tick = 0;       // 上次脉冲时间

// 引用外部的距离变量（从sdm18.c中来的）
extern uint16_t gSDM18_Distance;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 电机控制函数声明 - 添加在这里！
void Motor_GPIO_Init(void);
void Motor_Set_Direction(uint8_t dir);
void Motor_Update_By_Distance(uint16_t dist);
void Motor_Generate_Pulse(void);
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  
  /* USER CODE BEGIN 2 */
  BSP_init();  // 传感器初始化
  
  // 电机GPIO初始化
  Motor_GPIO_Init();
  
  printf("System Ready! Distance will control motor\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    BSP_Loop();              // 处理传感器数据
    Motor_Generate_Pulse();   // 产生电机脉冲
    
    // 每50ms更新一次电机状态
    static uint32_t last_update = 0;
    if(HAL_GetTick() - last_update > 50)
    {
        last_update = HAL_GetTick();
        Motor_Update_By_Distance(gSDM18_Distance);
    }
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
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
/**
  * @brief 电机GPIO初始化
  */
void Motor_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = MOTOR_EN_PIN | MOTOR_STP_PIN | MOTOR_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_EN_PIN, GPIO_PIN_SET);   // 使能
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET); // 脉冲低
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // 方向默认
    
    printf("Motor GPIO Init OK\r\n");
}

/**
  * @brief 设置电机方向
  */
void Motor_Set_Direction(uint8_t dir)
{
    if(dir == MOTOR_FORWARD)
    {
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET);
        printf("Direction: Forward\r\n");
    }
    else if(dir == MOTOR_BACKWARD)
    {
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
        printf("Direction: Backward\r\n");
    }
}

/**
  * @brief 根据距离更新电机状态
  */
void Motor_Update_By_Distance(uint16_t dist)
{
    static uint8_t last_status = MOTOR_STOP;
    uint8_t new_status = last_status;
    
    if(dist == 0) return;
    
    switch(last_status)
    {
        case MOTOR_STOP:
            if(dist < DISTANCE_NEAR)
                new_status = MOTOR_FORWARD;
            else if(dist > DISTANCE_FAR)
                new_status = MOTOR_BACKWARD;
            break;
            
        case MOTOR_FORWARD:
            if(dist > DISTANCE_NEAR + 200)
                new_status = MOTOR_STOP;
            break;
            
        case MOTOR_BACKWARD:
            if(dist < DISTANCE_FAR - 200)
                new_status = MOTOR_STOP;
            break;
    }
    
    if(new_status != last_status)
    {
        last_status = new_status;
        motor_status = new_status;
        
        if(motor_status == MOTOR_STOP)
        {
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
            pulse_count = 0;
            printf("Motor Stop (dist=%dmm)\r\n", dist);
        }
        else
        {
            Motor_Set_Direction(motor_status);
            pulse_count = 0;
            if(motor_status == MOTOR_FORWARD)
                printf("Motor Forward (dist=%dmm)\r\n", dist);
            else
                printf("Motor Backward (dist=%dmm)\r\n", dist);
        }
    }
}

/**
  * @brief 产生脉冲
  */
void Motor_Generate_Pulse(void)
{
    if(motor_status == MOTOR_STOP)
        return;
    
    if(HAL_GetTick() - last_pulse_tick > 2)
    {
        last_pulse_tick = HAL_GetTick();
        HAL_GPIO_TogglePin(MOTOR_PORT, MOTOR_STP_PIN);
        pulse_count++;
    }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif