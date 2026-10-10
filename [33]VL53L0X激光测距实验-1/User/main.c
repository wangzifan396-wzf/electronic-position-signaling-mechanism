/**
  ******************************************************************************
  * @file       main.c
  * @author     embedfire
  * @version    V1.0
  * @date       2026
  * @brief      DWT应用与VL53L0X测距电机控制
  ******************************************************************************
  * @attention
  *
  * 实验平台  ：野火 STM32F103C8T6-STM32开发板 
  * 论坛      ：http://www.firebbs.cn
  * 官网      ：https://embedfire.com/
  * 淘宝      ：https://yehuosm.tmall.com/
  *
  ******************************************************************************
	*/

#include "main.h"
#include "led/bsp_led.h"
#include "dwt/bsp_dwt.h"  // 确保包含DWT头文件，解决函数声明警告
#include "usart/bsp_usart.h"
#include "i2c/bsp_i2c.h"
#include "vl53l0x/bsp_i2c_vl53l0x.h"
#include "vl53l0x/app_vl53l0x.h"
#include "board.h"
#include <stdbool.h>
#include "stm32f1xx_hal_gpio.h"

// 距离阈值宏定义
#define MAX_DISTANCE_DATA     5        // 5个数据用于判断
#define DISTANCE_THRESH_400   400      // 400mm阈值
#define FULL_CORE_THRESH      50       // 满芯阈值：50mm（连续5次测距<50mm）
#define MOTOR_PAUSE_DURATION  5000     // 停转时长：5秒（5000ms）- 仅用于<400mm时的停转
#define MOTOR_PULSE_INTERVAL  500      // 电机脉冲间隔（500us = 1kHz频率）

// 反钻效果相关宏定义
#define REVERSE_DRILL_HOLD_TIME 100000  // 反钻保持时间：100ms
#define REVERSE_DRILL_PAUSE_TIME 50000  // 反钻暂停时间：50ms

// 定义存储距离数据的数组和索引
uint16_t distance_array[MAX_DISTANCE_DATA] = {0};  // 存储距离数据
uint8_t data_index = 0;  // 当前数据索引
uint8_t data_ready = 0;  // 标记是否已收集到足够的5个数据

// 电机控制相关变量
__IO int32_t pulse_count = 0;     // 脉冲计数
__IO bool motor_direction = false; // 电机方向：false=正转，true=反转
__IO bool motor_enabled = true;   // 电机使能标志

// 状态管理变量
static uint8_t motor_pause_state = 0;       // <400mm时的5秒停转状态
static uint8_t pause_complete = 0;          // 5秒停转完成标志
static uint32_t stop_motor_tick = 0;        // 电机停转计时（5秒）
static uint32_t last_pulse_tick = 0;        // 电机脉冲计时（非阻塞）

// 反钻效果相关变量（用于卡芯和满芯的反钻控制）
static uint32_t reverse_drill_tick = 0;      // 反钻计时
static uint8_t reverse_drill_phase = 0;      // 反钻相位：0=反转，1=暂停
static uint32_t reverse_drill_phase_tick = 0; // 相位计时

// 满芯检测相关变量
static uint8_t full_core_detected = 0;        // 满芯检测标志：1=已检测到满芯
static uint8_t full_core_count = 0;           // 连续<50mm的计数
static uint8_t full_core_reverse_active = 0;  // 满芯反钻激活标志（删除停转状态变量）

// 卡芯检测相关变量
static uint8_t stuck_core_detected = 0;        // 卡芯检测标志：1=已检测到卡芯
static uint8_t stuck_core_reverse_active = 0;  // 卡芯反钻激活标志（删除停转状态变量）

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();                        // 初始化 HAL 库 
    SystemClock_Config();              // 配置系统时钟，设置为 72MHz
    LED_GPIO_Config();                 // 配置 LED 所在的 GPIO 端口和引脚
    MX_USART1_UART_Init();             // 初始化串口，用于打印调试信息
    DWT_Init();                        // 启动 DWT 计数器，用于精确测量程序运行时间
    VL53L0X_init(&vl53l0x_dev, 1);     // 初始化 VL53L0X 设备，模式设为 1（高精度）

    // 定义存储单次测量数据的变量
    VL53L0X_RangingMeasurementData_t measure_data;
    

    board_init();
    DWT_DelayMs(2000);
    
    printf("系统启动完成\r\n");
    printf("电机控制逻辑：\r\n");
    printf("  - 距离>400mm：电机强制正转\r\n");
    printf("  - 距离<400mm：电机先停转5秒，再正转\r\n");
    printf("  - 【满芯检测】连续5次测距 < 50mm：电机直接进入持续反钻，直到复位\r\n");
    printf("  - 【卡芯检测】连续5个测距数据的最大值与最小值之差 ≤ 10mm：电机直接进入持续反钻，直到复位\r\n");
    
    // 初始状态：电机正转
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // Dir低电平：正转
    motor_direction = false;
    last_pulse_tick = DWT_GetTickUs(); // 初始化脉冲计时

    while (1)
    {
        uint32_t current_tick = HAL_GetTick();
        uint32_t current_us = DWT_GetTickUs();  // 获取微秒级时间用于反钻控制

        // ===================== 满芯检测和处理逻辑（最高优先级） =====================
        if (full_core_detected)
        {
            // 满芯状态：直接进入持续反钻，忽略其他所有判断
            full_core_reverse_active = 1;
            
            // 持续反钻阶段
            if (full_core_reverse_active)
            {
                motor_enabled = true;
                
                // 反钻相位控制（只反转，有暂停）
                switch (reverse_drill_phase)
                {
                    case 0: // 反转相位
                        // 设置反转方向
                        if (motor_direction != true)
                        {
                            motor_direction = true;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
                            printf("【满芯反钻】设置反转方向\r\n");
                        }
                        
                        // 检查反转时间是否到达
                        if ((current_us - reverse_drill_phase_tick) >= REVERSE_DRILL_HOLD_TIME)
                        {
                            reverse_drill_phase = 1;  // 切换到暂停
                            reverse_drill_phase_tick = current_us;
                            // 暂停期间关闭脉冲
                            motor_enabled = false;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
                        }
                        break;
                        
                    case 1: // 暂停相位
                        // 检查暂停时间是否到达
                        if ((current_us - reverse_drill_phase_tick) >= REVERSE_DRILL_PAUSE_TIME)
                        {
                            reverse_drill_phase = 0;  // 切回反转
                            reverse_drill_phase_tick = current_us;
                            motor_enabled = true;  // 重新使能电机
                        }
                        break;
                }
                
                // 红灯闪烁表示满芯反钻状态
                R_LED_TOGGLE();
                
                // 每200ms打印一次状态（降低频率）
                if ((current_us - reverse_drill_tick) >= 200000)
                {
                    reverse_drill_tick = current_us;
                    printf("【满芯反钻】持续反转中，脉冲计数：%ld\r\n", pulse_count);
                }
            }
            
            // 跳过其他所有逻辑，只进行脉冲生成
            goto pulse_generation;
        }

        // ===================== 卡芯检测和处理逻辑（次高优先级） =====================
        if (stuck_core_detected)
        {
            // 卡芯状态：直接进入持续反钻，忽略其他所有判断
            stuck_core_reverse_active = 1;
            
            // 持续反钻阶段
            if (stuck_core_reverse_active)
            {
                motor_enabled = true;
                
                // 反钻相位控制（只反转，有暂停）
                switch (reverse_drill_phase)
                {
                    case 0: // 反转相位
                        // 设置反转方向
                        if (motor_direction != true)
                        {
                            motor_direction = true;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
                            printf("【卡芯反钻】设置反转方向\r\n");
                        }
                        
                        // 检查反转时间是否到达
                        if ((current_us - reverse_drill_phase_tick) >= REVERSE_DRILL_HOLD_TIME)
                        {
                            reverse_drill_phase = 1;  // 切换到暂停
                            reverse_drill_phase_tick = current_us;
                            // 暂停期间关闭脉冲
                            motor_enabled = false;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
                        }
                        break;
                        
                    case 1: // 暂停相位
                        // 检查暂停时间是否到达
                        if ((current_us - reverse_drill_phase_tick) >= REVERSE_DRILL_PAUSE_TIME)
                        {
                            reverse_drill_phase = 0;  // 切回反转
                            reverse_drill_phase_tick = current_us;
                            motor_enabled = true;  // 重新使能电机
                        }
                        break;
                }
                
                // 红灯闪烁表示卡芯反钻状态
                R_LED_TOGGLE();
                
                // 每200ms打印一次状态（降低频率）
                if ((current_us - reverse_drill_tick) >= 200000)
                {
                    reverse_drill_tick = current_us;
                    printf("【卡芯反钻】持续反转中，脉冲计数：%ld\r\n", pulse_count);
                }
            }
            
            // 跳过其他所有逻辑，只进行脉冲生成
            goto pulse_generation;
        }

        // ===================== 正常测距和判断逻辑 =====================
        static uint32_t last_measure_tick = 0;
        if (current_tick - last_measure_tick >= 500)  
        {
            last_measure_tick = current_tick;
            
            // 进行单次距离测量，并获取数据
            if (VL53L0X_Task(&vl53l0x_dev, &measure_data) == VL53L0X_ERROR_NONE)
            {
                uint16_t current_distance = measure_data.RangeMilliMeter;
                printf("当前测距值: %d mm\r\n", current_distance);

                // 满芯检测：连续5次测距 < 50mm
                if (current_distance < FULL_CORE_THRESH)
                {
                    full_core_count++;
                    printf("满芯计数：%d/5 (当前值: %d < 50mm)\r\n", full_core_count, current_distance);
                    
                    if (full_core_count >= MAX_DISTANCE_DATA)
                    {
                        full_core_detected = 1;  // 触发满芯状态
                        full_core_reverse_active = 1;  // 直接激活反钻
                        full_core_count = 0;      // 重置计数
                        printf("【满芯检测】连续5次 < 50mm，直接进入持续反钻（无停转）\r\n");
                        continue;  // 直接进入满芯处理
                    }
                }
                else
                {
                    if (full_core_count > 0)
                    {
                        printf("满芯计数重置：当前值 %d >= 50mm\r\n", current_distance);
                    }
                    full_core_count = 0;  // 有测距>=50mm，重置计数
                }

                // 如果已经满芯，跳过其他逻辑
                if (full_core_detected)
                {
                    continue;
                }

                // 1. 距离>400mm：电机强制一直正转
                if (current_distance > DISTANCE_THRESH_400)
                {
                    // 重置停转相关状态
                    motor_pause_state = 0;
                    pause_complete = 0;
                    // 强制正转状态
                    motor_enabled = true;
                    motor_direction = false;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); 
                    printf("【距离>400mm】电机强制正转\r\n");
                    G_LED_ON_ONLY(); 
                    continue;
                }

                // 2. 距离<400mm：先停转5秒，再正转，之后进入卡芯检测
                else
                {
                    // 进入5秒停转状态（首次触发时初始化计时）
                    if (!motor_pause_state)
                    {
                        motor_pause_state = 1;    
                        stop_motor_tick = current_tick; 
                        motor_enabled = false;        
                        printf("【距离<400mm】电机开始停转5秒\r\n");
                        B_LED_ON_ONLY(); 
                    }
                    // 停转计时中：判断5秒是否完成
                    else if (motor_pause_state && !pause_complete)
                    {
                        uint32_t pause_elapsed = current_tick - stop_motor_tick;
                        if (pause_elapsed >= MOTOR_PAUSE_DURATION)
                        {
                            pause_complete = 1;   
                            motor_pause_state = 0;
                            // 停转完成后：持续维持电机使能为true
                            motor_enabled = true;
                            motor_direction = false;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); 
                            printf("【停转完成】5秒停转结束，电机恢复正转，进入卡芯检测\r\n");
                            G_LED_ON_ONLY(); 
                        }
                        else
                        {
                            motor_enabled = false;
                            printf("【停转中】剩余时间：%dms\r\n", MOTOR_PAUSE_DURATION - pause_elapsed);
                            continue;
                        }
                    }

                    // ===================== 卡芯检测逻辑 =====================
                    // 停转完成后，持续维持电机使能
                    motor_enabled = true;
                    
                    // 存储最新测量数据
                    distance_array[data_index] = current_distance;
                    printf("测距数据[%d]: %d mm\r\n", data_index, current_distance);
                    
                    data_index++;
                    if (data_index >= MAX_DISTANCE_DATA)
                    {
                        data_index = 0;
                        data_ready = 1;
                    }

                    // 数据足够时进行卡芯判断
                    if (data_ready)
                    {
                        // 计算5个数据的最大值和最小值
                        uint16_t max_value = distance_array[0];
                        uint16_t min_value = distance_array[0];
                        
                        for (uint8_t i = 1; i < MAX_DISTANCE_DATA; i++)
                        {
                            if (distance_array[i] > max_value)
                            {
                                max_value = distance_array[i];
                            }
                            if (distance_array[i] < min_value)
                            {
                                min_value = distance_array[i];
                            }
                        }
                        
                        // 计算最大值与最小值的差
                        uint16_t range_diff = max_value - min_value;
                        
                        printf("卡芯检测：最大值=%d, 最小值=%d, 差值=%d\r\n", max_value, min_value, range_diff);
                        
                        // 卡芯检测：最大值与最小值之差 ≤ 10mm 时触发卡芯状态
                        if (range_diff <= 10)
                        {
                            stuck_core_detected = 1;  // 触发卡芯状态
                            stuck_core_reverse_active = 1;  // 直接激活反钻
                            printf("【卡芯检测】最大值与最小值差 ≤ 10mm，直接进入持续反钻（无停转）\r\n");
                            R_LED_ON_ONLY();  
                            continue;
                        }
                        else
                        {
                            // 未触发卡芯，保持正转
                            if (motor_direction != false)
                            {
                                motor_direction = false;
                                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  
                                printf("【卡芯检测】差值>10mm，电机保持正转\r\n");
                                G_LED_ON_ONLY();  
                            }
                        }
                    }
                }
            }
            else
            {
                // 测量失败：维持电机当前状态
                printf("【测距错误】测量失败，电机保持当前状态\r\n");
                B_LED_ON_ONLY();  
            }
        }

pulse_generation:
        // ===================== 脉冲生成代码 =====================
        if (motor_enabled)
        {
            // 非阻塞式脉冲间隔判断（500us）
            if (DWT_GetTickUs() - last_pulse_tick >= MOTOR_PULSE_INTERVAL)
            {
                last_pulse_tick = DWT_GetTickUs();
                
                // 翻转PA6产生脉冲
                GPIOA->ODR ^= GPIO_PIN_6;
                
                // 计数
                pulse_count++;
                
                // 每2000个脉冲闪烁一次LED
                if (pulse_count % 2000 == 0)
                {
                    if (motor_direction)
                        R_LED_TOGGLE();  
                    else
                        G_LED_TOGGLE();  
                    printf("【电机状态】%s，脉冲计数：%d\r\n", motor_direction ? "反转" : "正转", pulse_count);
                }
            }
        }
        else
        {
            // 电机禁用时，确保脉冲引脚置低
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
            last_pulse_tick = DWT_GetTickUs(); // 重置脉冲计时
        }
    }
}


void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef clkinitstruct = {0};
  RCC_OscInitTypeDef oscinitstruct = {0};
  

  oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
  oscinitstruct.HSEState        = RCC_HSE_ON;
  oscinitstruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
  oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
  oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
  oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&oscinitstruct)!= HAL_OK)
  {
    while(1);
  }

  clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
  clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  if (HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    while(1);
  }
}


void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}
#endif USE_FULL_ASSERT 
// 注意：文件最后按Enter加空行，解决#1-D警告