/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 新型钻具机构控制系统
  *                   - 激光雷达测距控制内管升降
  *                   - 内管到位400mm停止
  *                   - 到位后才开始卡芯/满芯判断
  *                   - 卡芯/满芯时反转上提到800mm
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
#include <string.h>
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 电机引脚定义
#define MOTOR_EN_PIN   GPIO_PIN_5      // 使能引脚
#define MOTOR_STP_PIN  GPIO_PIN_6      // 脉冲引脚
#define MOTOR_DIR_PIN  GPIO_PIN_7      // 方向引脚
#define MOTOR_PORT     GPIOA

// 钻具工作模式
#define MODE_IDLE       0    // 空闲
#define MODE_DOWN       1    // 正转下钻
#define MODE_UP         2    // 反转上提
#define MODE_PAUSE      3    // 暂停
#define MODE_WAIT       4    // 到位等待（判断卡芯/满芯）

// 距离阈值定义（单位：mm）
#define THRESHOLD_GROUND    800     // 地面上阈值（内管在顶部）
#define THRESHOLD_IN_PLACE  400     // 内管到位阈值（到此停止）
#define THRESHOLD_FULL      100     // 满芯阈值（小于100mm为满）
#define THRESHOLD_STUCK     10      // 卡芯判断差值（10mm以内为卡）

// 数组大小定义
#define DIST_ARRAY_SIZE     10      // 10个相邻测距的数组

// 时间定义（单位：ms）
#define MEASURE_INTERVAL    100     // 100ms测一次距离
#define PAUSE_TIME          3000    // 暂停3秒
#define PULSE_SPEED         1       // 1ms脉冲
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
// 电机控制变量
__IO int32_t pulse_count = 0;           // 脉冲计数
__IO uint8_t motor_status = MODE_IDLE;   // 电机当前状态
__IO uint32_t last_pulse_tick = 0;       // 上次脉冲时间

// 钻具控制变量
__IO uint8_t drill_mode = MODE_IDLE;     // 钻具工作模式
__IO uint32_t pause_start_time = 0;      // 暂停开始时间
__IO uint8_t pause_trigger_reason = 0;   // 暂停原因：1-卡芯 2-满芯

// 重要：标记当前是否在反转上提阶段
__IO uint8_t is_reversing = 0;           // 0-不是反转 1-正在反转上提

// 到位标记：表示已经到达400mm，开始卡芯/满芯判断
__IO uint8_t is_in_place = 0;            // 0-未到位 1-已到位

// 测距数组（用于卡芯和满芯判断）
__IO uint16_t dist_array[DIST_ARRAY_SIZE] = {0};  // 存储10个距离值
__IO uint8_t dist_array_index = 0;                 // 数组当前索引
__IO uint8_t dist_array_full = 0;                   // 数组是否已填满

// 传感器距离（从sdm18.c获取）
extern uint16_t gSDM18_Distance;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 函数声明
void Motor_GPIO_Init(void);
void Motor_Set_Direction(uint8_t dir);
void Motor_Generate_Pulse(void);
void Drill_Control(uint16_t dist);
void Update_Distance_Array(uint16_t dist);
uint8_t Check_Stuck(void);
uint8_t Check_Full(void);
void Print_Status(uint16_t dist);
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
  
  // 清空距离数组
  memset((void*)dist_array, 0, sizeof(dist_array));
  
  printf("\r\n====================================\r\n");
  printf("新型钻具机构控制系统启动\r\n");
  printf("阈值设置：到位=%dmm(停止) 满芯=<%dmm 卡芯差值<%dmm\r\n", 
         THRESHOLD_IN_PLACE, THRESHOLD_FULL, THRESHOLD_STUCK);
  printf("地面阈值：%dmm(反转停止)\r\n", THRESHOLD_GROUND);
  printf("测距间隔：%dms\r\n", MEASURE_INTERVAL);
  printf("====================================\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    BSP_Loop();  // 处理传感器数据（会更新gSDM18_Distance）
    
    // 产生电机脉冲
    Motor_Generate_Pulse();
    
    // 每100ms执行一次控制逻辑
    static uint32_t last_control = 0;
    if(HAL_GetTick() - last_control >= MEASURE_INTERVAL)
    {
        last_control = HAL_GetTick();
        
        // 获取当前距离值
        uint16_t current_dist = gSDM18_Distance;
        
        // 更新距离数组（用于卡芯和满芯判断）
        Update_Distance_Array(current_dist);
        
        // 执行钻具控制逻辑
        Drill_Control(current_dist);
        
        // 串口打印状态
        Print_Status(current_dist);
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
    
    // 初始状态：使能有效，脉冲低，方向默认
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_EN_PIN, GPIO_PIN_SET);   // 使能
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET); // 脉冲低
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // 方向默认
}

/**
  * @brief 设置电机方向
  * @param dir: MODE_DOWN-下钻(正转)  MODE_UP-上提(反转)
  */
void Motor_Set_Direction(uint8_t dir)
{
    if(dir == MODE_DOWN)
    {
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET);   // 正转下钻
    }
    else if(dir == MODE_UP)
    {
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // 反转上提
    }
}

/**
  * @brief 产生电机脉冲
  */
void Motor_Generate_Pulse(void)
{
    // 只有下钻或上提模式才发脉冲
    if(motor_status != MODE_DOWN && motor_status != MODE_UP)
        return;
    
    // 1ms产生一个脉冲
    if(HAL_GetTick() - last_pulse_tick >= 1)
    {
        last_pulse_tick = HAL_GetTick();
        HAL_GPIO_TogglePin(MOTOR_PORT, MOTOR_STP_PIN);
        pulse_count++;
    }
}

/**
  * @brief 更新距离数组（环形缓冲区）
  * @param dist: 当前距离值
  */
void Update_Distance_Array(uint16_t dist)
{
    // 将新距离存入数组当前位置
    dist_array[dist_array_index] = dist;
    
    // 更新索引（循环0-9）
    dist_array_index++;
    if(dist_array_index >= DIST_ARRAY_SIZE)
    {
        dist_array_index = 0;
        dist_array_full = 1;  // 标记数组已填满一次
    }
}

/**
  * @brief 检查是否卡芯
  * @retval 1-卡芯  0-正常
  */
uint8_t Check_Stuck(void)
{
    // 如果数组还没填满，无法判断
    if(!dist_array_full)
        return 0;
    
    // 找出数组中的最大值和最小值
    uint16_t max_dist = 0;
    uint16_t min_dist = 65535;
    
    for(int i = 0; i < DIST_ARRAY_SIZE; i++)
    {
        if(dist_array[i] > max_dist)
            max_dist = dist_array[i];
        if(dist_array[i] < min_dist)
            min_dist = dist_array[i];
    }
    
    // 计算差值
    uint16_t diff = max_dist - min_dist;
    
    // 如果差值小于阈值，说明距离变化很小 → 可能卡芯
    if(diff < THRESHOLD_STUCK)
    {
        return 1;
    }
    
    return 0;
}

/**
  * @brief 检查是否满芯
  * @retval 1-满芯  0-未满
  */
uint8_t Check_Full(void)
{
    // 如果数组还没填满，无法判断
    if(!dist_array_full)
        return 0;
    
    // 检查是否所有距离都小于满芯阈值
    for(int i = 0; i < DIST_ARRAY_SIZE; i++)
    {
        if(dist_array[i] >= THRESHOLD_FULL)
            return 0;  // 有一个大于等于阈值就不算满
    }
    
    return 1;
}

/**
  * @brief 钻具控制主逻辑
  * @param dist: 当前距离值(mm)
  * 
  * 逻辑说明：
  * 1. 距离>400mm：正转下钻
  * 2. 距离<=400mm：停止（内管到位），并标记is_in_place=1
  * 3. 到位后开始卡芯/满芯判断
  * 4. 卡芯/满芯时：暂停3秒 → 反转上提到800mm
  * 5. 反转阶段：必须到800mm才停止
  */
void Drill_Control(uint16_t dist)
{
    // 如果距离为0（还没收到有效数据），不动作
    if(dist == 0) return;
    
    /********** 第一步：处理暂停状态 **********/
    if(drill_mode == MODE_PAUSE)
    {
        // 检查是否到达暂停时间
        if(HAL_GetTick() - pause_start_time >= PAUSE_TIME)
        {
            // 暂停结束，开始反转上提
            drill_mode = MODE_UP;
            motor_status = MODE_UP;
            is_reversing = 1;  // 标记为反转阶段
            is_in_place = 0;    // 离开到位状态
            Motor_Set_Direction(MODE_UP);
            printf("⬆️ 暂停结束，反转上提（目标%dmm）\r\n", THRESHOLD_GROUND);
        }
        return;  // 暂停中不执行其他判断
    }
    
    /********** 第二步：检查是否在地面阈值（最高优先级） **********/
    if(dist >= THRESHOLD_GROUND)
    {
        // 到达地面阈值，无论什么状态都停止
        if(drill_mode != MODE_IDLE || is_reversing != 0)
        {
            drill_mode = MODE_IDLE;
            motor_status = MODE_IDLE;
            is_reversing = 0;  // 清除反转标记
            is_in_place = 0;    // 清除到位标记
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
            printf("🏁 到达地面阈值 %dmm，停止\r\n", THRESHOLD_GROUND);
            printf("----------------------------------------\r\n");
        }
        return;
    }
    
    /********** 第三步：如果正在反转阶段，继续反转直到地面 **********/
    if(is_reversing == 1)
    {
        // 反转阶段只关心是否到达地面阈值（上面已判断）
        // 没到地面就继续反转
        if(drill_mode != MODE_UP)
        {
            drill_mode = MODE_UP;
            motor_status = MODE_UP;
            Motor_Set_Direction(MODE_UP);
        }
        return;  // 直接返回，不执行到位判断
    }
    
    /********** 第四步：判断是否到位（<=400mm） **********/
    if(dist <= THRESHOLD_IN_PLACE)
    {
        // 内管到位，停止电机
        if(drill_mode != MODE_IDLE)
        {
            drill_mode = MODE_IDLE;
            motor_status = MODE_IDLE;
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
            printf("⏹️ 内管到位 %dmm，停止\r\n", dist);
        }
        
        // 标记为到位状态
        if(is_in_place == 0)
        {
            is_in_place = 1;
            printf("🔍 开始卡芯/满芯监测...\r\n");
        }
        
        /********** 第五步：到位后才进行卡芯/满芯判断 **********/
        if(is_in_place == 1)
        {
            // 检查满芯
            if(Check_Full())
            {
                drill_mode = MODE_PAUSE;
                motor_status = MODE_IDLE;
                pause_start_time = HAL_GetTick();
                pause_trigger_reason = 2;
                printf("⏸️ 满芯！暂停3秒后反转上提\r\n");
                return;
            }
            
            // 检查卡芯
            if(Check_Stuck())
            {
                drill_mode = MODE_PAUSE;
                motor_status = MODE_IDLE;
                pause_start_time = HAL_GetTick();
                pause_trigger_reason = 1;
                printf("⚠️ 卡芯！暂停3秒后反转上提\r\n");
                return;
            }
        }
        
        return;  // 到位且无异常，保持停止
    }
    
    /********** 第六步：距离>400mm，还没到位 **********/
    if(dist > THRESHOLD_IN_PLACE)
    {
        // 还没到位，正常下钻
        if(drill_mode != MODE_DOWN)
        {
            drill_mode = MODE_DOWN;
            motor_status = MODE_DOWN;
            is_in_place = 0;  // 清除到位标记
            Motor_Set_Direction(MODE_DOWN);
            printf("⬇️ 距离=%dmm > %dmm，正转下钻\r\n", dist, THRESHOLD_IN_PLACE);
        }
    }
}

/**
  * @brief 串口打印状态信息
  * @param dist: 当前距离值
  */
void Print_Status(uint16_t dist)
{
    static uint32_t last_print = 0;
    
    // 每100ms打印一次
    if(HAL_GetTick() - last_print >= MEASURE_INTERVAL)
    {
        last_print = HAL_GetTick();
        
        // 获取数组最大最小值和平均值
        uint16_t max_d = 0, min_d = 65535;
        uint32_t sum_d = 0;  // 用于计算平均值
        uint16_t avg_d = 0;
        
        for(int i = 0; i < DIST_ARRAY_SIZE; i++)
        {
            if(dist_array[i] > max_d) max_d = dist_array[i];
            if(dist_array[i] < min_d) min_d = dist_array[i];
            sum_d += dist_array[i];
        }
        
        // 计算平均值（如果数组已填满）
        if(dist_array_full)
        {
            avg_d = sum_d / DIST_ARRAY_SIZE;
        }
        else
        {
            avg_d = 0;  // 数组还没填满，平均值为0
        }
        
        // 模式字符串
        char *mode_str = "";
        switch(drill_mode)
        {
            case MODE_IDLE:   mode_str = "空闲"; break;
            case MODE_DOWN:   mode_str = "下钻"; break;
            case MODE_UP:     mode_str = "上提"; break;
            case MODE_PAUSE:  mode_str = "暂停"; break;
        }
        
        // 打印格式：距离 | 模式 | 到位标记 | 反转标记 | 平均值 | 差值
        printf("[%03d] dist=%4dmm | %s |到位=%d|rev=%d| avg=%3dmm| diff=%2dmm\r\n", 
               (HAL_GetTick()/1000)%1000,
               dist, 
               mode_str,
               is_in_place,
               is_reversing,
               avg_d,
               max_d - min_d);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
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