/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 主程序文件
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BSP.h"        // 板级支持包
#include "SMD18.h"      // 测距传感器驱动
#include "stepper.h"    // 步进电机驱动
#include <stdio.h>      // 标准输入输出
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 系统运行模式枚举
typedef enum {
    MODE_IDLE = 0,      // 空闲模式
    MODE_AUTO,          // 自动控制模式
    MODE_MANUAL         // 手动模式
} SystemMode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 控制参数定义
#define CONTROL_INTERVAL    100     // 控制间隔(ms)
#define DISTANCE_TARGET     30      // 目标距离(cm)
#define DISTANCE_TOLERANCE  5       // 允许误差(cm)

// 调试输出开关
#define DEBUG_ENABLE        1       // 1:开启调试输出, 0:关闭
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#if DEBUG_ENABLE
    #define DEBUG_PRINT(...)  printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)
#endif
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 系统变量
static SystemMode_t system_mode = MODE_AUTO;    // 当前系统模式
static float current_distance = 0;               // 当前测量距离
static uint32_t last_control_time = 0;           // 上次控制时间
static uint8_t led_state = 0;                     // LED状态

// 统计变量
static uint32_t total_steps = 0;                  // 总步数计数
static uint32_t control_count = 0;                 // 控制次数计数
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Print_SystemInfo(void);               // 打印系统信息
static void Update_LED(void);                      // 更新LED指示
static void Process_DistanceControl(void);         // 处理距离控制
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief 打印系统信息
  * @param 无
  * @retval 无
  */
static void Print_SystemInfo(void)
{
    printf("\r\n========== 系统信息 ==========\r\n");
    printf("系统模式: %s\r\n", system_mode == MODE_AUTO ? "自动控制" : 
                                 (system_mode == MODE_MANUAL ? "手动控制" : "空闲模式"));
    printf("目标距离: %d cm\r\n", DISTANCE_TARGET);
    printf("控制间隔: %d ms\r\n", CONTROL_INTERVAL);
    printf("电机状态: %s\r\n", Stepper_GetState() == MOTOR_FORWARD ? "正转(远离)" :
                                 (Stepper_GetState() == MOTOR_BACKWARD ? "反转(靠近)" : "停止"));
    printf("当前距离: %.1f cm\r\n", current_distance);
    printf("总步数: %ld\r\n", total_steps);
    printf("==============================\r\n");
}

/**
  * @brief 更新LED指示
  * @param 无
  * @retval 无
  */
static void Update_LED(void)
{
    // LED闪烁指示系统状态
    led_state = !led_state;
    
    if(system_mode == MODE_AUTO)
    {
        // 自动模式：LED慢闪
        if(led_state)
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  // 亮
        }
        else
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);    // 灭
        }
    }
    else if(system_mode == MODE_MANUAL)
    {
        // 手动模式：LED常亮
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    }
    else
    {
        // 空闲模式：LED常灭
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    }
}

/**
  * @brief 处理距离控制
  * @param 无
  * @retval 无
  */
static void Process_DistanceControl(void)
{
    // 读取当前距离
    current_distance = SMD18_GetDistance();
    
    // 根据距离控制电机
    MotorState_t old_state = Stepper_GetState();
    Stepper_ControlByDistance(current_distance, DISTANCE_TARGET, DISTANCE_TOLERANCE);
    MotorState_t new_state = Stepper_GetState();
    
    // 如果电机状态改变，记录步数
    if(new_state != MOTOR_STOP)
    {
        total_steps += 5;  // Stepper_ControlByDistance每次发5个脉冲
    }
    
    // 状态变化时打印信息
    if(old_state != new_state)
    {
        DEBUG_PRINT("[状态变化] 电机: %s -> %s\r\n",
            old_state == MOTOR_FORWARD ? "正转" : (old_state == MOTOR_BACKWARD ? "反转" : "停止"),
            new_state == MOTOR_FORWARD ? "正转" : (new_state == MOTOR_BACKWARD ? "反转" : "停止"));
    }
    
    control_count++;
}
/* USER CODE END 0 */

/**
  * @brief 应用程序入口点
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  // 用户代码区域1 - 系统初始化前的代码
  /* USER CODE END 1 */

  /* MCU配置--------------------------------------------------------*/

  /* 复位所有外设，初始化Flash接口和Systick */
  HAL_Init();

  /* USER CODE BEGIN Init */
  // 用户初始化代码
  /* USER CODE END Init */

  /* 配置系统时钟 */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  // 系统初始化后的代码
  /* USER CODE END SysInit */

  /* 初始化所有配置的外设 */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  
  /* USER CODE BEGIN 2 */
  // 板级初始化
  BSP_init();
  
  // 初始化测距传感器
  SMD18_Init();
  DEBUG_PRINT("测距传感器初始化完成\r\n");
  
  // 初始化步进电机
  Stepper_Init();
  DEBUG_PRINT("步进电机初始化完成\r\n");
  
  // 设置电机速度（中速）
  Stepper_SetSpeed(SPEED_MEDIUM);
  
  // 使能电机
  Stepper_Enable();
  DEBUG_PRINT("电机已使能\r\n");
  
  // 打印系统信息
  Print_SystemInfo();
  /* USER CODE END 2 */

  /* 无限循环 */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    
    // 运行BSP主循环
    BSP_Loop();
    
    /* USER CODE BEGIN 3 */
    
    // 定时控制（每100ms执行一次）
    if(HAL_GetTick() - last_control_time >= CONTROL_INTERVAL)
    {
        // 根据当前模式执行不同操作
        switch(system_mode)
        {
            case MODE_AUTO:
                // 自动模式：根据距离控制电机
                Process_DistanceControl();
                
                // 每10次控制打印一次距离信息
                if(control_count % 10 == 0)
                {
                    DEBUG_PRINT("当前距离: %.1f cm, 电机状态: %d\r\n", 
                               current_distance, Stepper_GetState());
                }
                break;
                
            case MODE_MANUAL:
                // 手动模式：可以在这里添加手动控制代码
                // 例如通过串口命令控制
                break;
                
            case MODE_IDLE:
            default:
                // 空闲模式：不进行电机控制
                break;
        }
        
        // 更新LED指示
        Update_LED();
        
        // 更新控制时间
        last_control_time = HAL_GetTick();
    }
    
    // 简单的串口命令处理（可以通过串口切换模式）
    // 例如：输入 'a' 切换到自动模式，'m' 切换到手动模式，'i' 切换到空闲模式
    /* USER CODE END 3 */
  }
}

/**
  * @brief 系统时钟配置
  * @retval 无
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** 初始化RCC振荡器
  */
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

  /** 初始化CPU、AHB和APB总线时钟
  */
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
  * @brief  HAL延时回调函数（如果需要可以启用）
  * @param 无
  * @retval 无
  */
// void HAL_Delay_Callback(void)
// {
//     // 可以在延时期间执行一些任务
// }
/* USER CODE END 4 */

/**
  * @brief  错误处理函数
  * @param  无
  * @retval 无
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* 用户可以添加自己的错误处理代码 */
  printf("系统错误！\r\n");
  __disable_irq();
  while (1)
  {
    // 错误时LED快闪
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  报告断言错误发生的源文件和行号
  * @param  file: 源文件名指针
  * @param  line: 断言错误行号
  * @retval 无
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* 用户可以添加自己的实现来报告文件名和行号 */
  printf("参数错误: 文件 %s 第 %d 行\r\n", file, line);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */