/**
  ******************************************************************************
  * @file       main.c
  * @author     embedfire
  * @version    V1.0
  * @date       2026
  * @brief      VL53L0X测距电机控制系统 - 满芯/卡芯检测与反钻控制
  ******************************************************************************
  * @attention
  *
  * 实验平台  ：野火 STM32F103C8T6-STM32开发板 
  * 论坛      ：http://www.firebbs.cn
  * 官网      ：https://embedfire.com/
  *
  ******************************************************************************
	*/

#include "main.h"
#include "dwt/bsp_dwt.h"          // 微秒级精确延时
#include "usart/bsp_usart.h"       // 串口调试
#include "vl53l0x/app_vl53l0x.h"   // VL53L0X测距传感器
#include "vl53l0x/bsp_i2c_vl53l0x.h" // VL53L0X的I2C通信
#include "board.h"
#include <stdbool.h>

// ========================== 系统参数配置 ==========================
#define SAMPLE_INTERVAL_MS     500     // 测距采样间隔：500ms
#define MOTOR_PULSE_INTERVAL_US 500     // 电机脉冲间隔：500us (1kHz频率)

// 距离阈值定义
#define DISTANCE_FAR_THRESH     400     // >400mm：远距离，电机正转
#define DISTANCE_NEAR_THRESH    400     // <400mm：近距离，进入停转+检测流程
#define FULL_CORE_THRESH_MM     50      // 满芯阈值：连续5次测距 < 50mm
#define STUCK_CORE_DIFF_THRESH   10     // 卡芯阈值：5个数据极差 ≤ 10mm

// 时序控制
#define MOTOR_PAUSE_5S_MS       5000    // 近距离停转5秒
#define STATUS_PRINT_INTERVAL_US 200000 // 状态打印间隔：200ms

// ========================== 数据结构定义 ==========================
// 距离数据缓冲区 - 存储最近5次测距值，用于满芯/卡芯判断
#define DATA_BUFFER_SIZE        5
static uint16_t g_distance_buffer[DATA_BUFFER_SIZE] = {0};
static uint8_t g_buffer_index = 0;
static uint8_t g_buffer_full = 0;      // 是否已收集满5个数据

// ========================== VL53L0X设备结构体 ==========================
static VL53L0X_Dev_t vl53l0x_dev;

// ========================== 电机控制变量 ==========================
static int32_t g_pulse_count = 0;       // 脉冲计数
static bool g_motor_direction = false;  // false=正转, true=反转
static bool g_motor_enabled = true;     // 电机使能

// 脉冲生成计时
static uint32_t g_last_pulse_tick_us = 0;

// ========================== 状态机变量 ==========================
// 近距离( <400mm )处理状态
typedef enum {
    NEAR_IDLE = 0,              // 空闲状态(距离>400mm)
    NEAR_PAUSE_5S,               // 5秒停转
    NEAR_RUNNING                 // 正常运行(正转+检测)
} NearState_t;
static NearState_t g_near_state = NEAR_IDLE;
static uint32_t g_near_pause_start_ms = 0;

// 异常状态标志 - 一旦触发就锁定，直到复位
static bool g_full_core_locked = false;     // 满芯锁定：连续5次<50mm
static bool g_stuck_core_locked = false;    // 卡芯锁定：5个数据极差≤10mm

// 满芯检测计数
static uint8_t g_full_core_count = 0;       // 连续<50mm的计数

// 状态打印计时
static uint32_t g_last_status_print_us = 0;

// ========================== 函数声明 ==========================
void SystemClock_Config(void);

// ========================== 主函数 ==========================
int main(void)
{
    // 基础初始化
    HAL_Init();
    SystemClock_Config();        // 配置系统时钟72MHz
    MX_USART1_UART_Init();       // 串口调试
    DWT_Init();                  // 微秒定时器
    
    // 初始化VL53L0X传感器
    VL53L0X_init(&vl53l0x_dev, 1); // 模式1：高精度模式
    
    board_init();
    DWT_DelayMs(2000);           // 等待系统稳定
    
    printf("\r\n========== 系统启动完成 ==========\r\n");
    printf("【工作模式】\r\n");
    printf("1. 距离 > %dmm : 电机正转\r\n", DISTANCE_FAR_THRESH);
    printf("2. 距离 < %dmm : 停转5秒 → 正转并检测\r\n", DISTANCE_NEAR_THRESH);
    printf("3. 【满芯检测】连续5次测距 < %dmm → 直接持续反转(锁定)\r\n", FULL_CORE_THRESH_MM);
    printf("4. 【卡芯检测】5个数据极差 ≤ %dmm → 直接持续反转(锁定)\r\n", STUCK_CORE_DIFF_THRESH);
    printf("==================================\r\n\r\n");
    
    // 初始状态：电机正转
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7=0: 正转
    g_motor_direction = false;
    g_last_pulse_tick_us = DWT_GetTickUs();

    // 测距数据
    VL53L0X_RangingMeasurementData_t measure_data;
    uint32_t last_measure_tick = 0;

    while (1)
    {
        uint32_t current_ms = HAL_GetTick();
        uint32_t current_us = DWT_GetTickUs();

        // ==================== 异常锁定处理(最高优先级) ====================
        if (g_full_core_locked || g_stuck_core_locked)
        {
            // 满芯或卡芯锁定后：持续反转
            g_motor_enabled = true;
            
            // 确保是反转方向
            if (g_motor_direction != true)
            {
                g_motor_direction = true;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);  // PA7=1: 反转
                printf("【%s】设置为持续反转模式\r\n", 
                       g_full_core_locked ? "满芯锁定" : "卡芯锁定");
            }
            
            // 定期打印状态(每200ms)
            if ((current_us - g_last_status_print_us) >= STATUS_PRINT_INTERVAL_US)
            {
                g_last_status_print_us = current_us;
                printf("【%s持续反转中】脉冲计数:%ld\r\n", 
                       g_full_core_locked ? "满芯" : "卡芯", g_pulse_count);
            }
            
            goto pulse_generation;  // 跳过测距和其他逻辑
        }

        // ==================== 定时测距(500ms间隔) ====================
        if ((current_ms - last_measure_tick) >= SAMPLE_INTERVAL_MS)
        {
            last_measure_tick = current_ms;
            
            if (VL53L0X_Task(&vl53l0x_dev, &measure_data) == VL53L0X_ERROR_NONE)
            {
                uint16_t dist = measure_data.RangeMilliMeter;
                printf("当前距离:%d mm\r\n", dist);

                // -------------------- 1. 满芯检测 --------------------
                if (dist < FULL_CORE_THRESH_MM)
                {
                    g_full_core_count++;
                    printf("满芯计数:%d/5 (<%dmm)\r\n", g_full_core_count, FULL_CORE_THRESH_MM);
                    
                    if (g_full_core_count >= DATA_BUFFER_SIZE)
                    {
                        g_full_core_locked = true;
                        g_full_core_count = 0;
                        printf("★★★ 满芯锁定! 连续5次<%dmm, 进入持续反转 ★★★\r\n", FULL_CORE_THRESH_MM);
                        continue;
                    }
                }
                else
                {
                    if (g_full_core_count > 0)
                    {
                        printf("满芯计数清零(距离≥%dmm)\r\n", FULL_CORE_THRESH_MM);
                    }
                    g_full_core_count = 0;
                }

                // -------------------- 2. 远距离处理(>400mm) --------------------
                if (dist > DISTANCE_FAR_THRESH)
                {
                    // 复位近距离状态机
                    g_near_state = NEAR_IDLE;
                    
                    // 强制正转
                    g_motor_enabled = true;
                    g_motor_direction = false;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7=0: 正转
                    
                    printf("【远距离>%dmm】电机正转\r\n", DISTANCE_FAR_THRESH);
                    continue;
                }

                // -------------------- 3. 近距离处理(<400mm) --------------------
                // 状态机: IDLE → 5秒停转 → 运行(正转+检测)
                switch (g_near_state)
                {
                    case NEAR_IDLE:
                        // 首次进入近距离: 启动5秒停转
                        g_near_state = NEAR_PAUSE_5S;
                        g_near_pause_start_ms = current_ms;
                        g_motor_enabled = false;
                        printf("【近距离<%dmm】停转5秒开始\r\n", DISTANCE_NEAR_THRESH);
                        break;
                        
                    case NEAR_PAUSE_5S:
                        // 5秒停转中
                        if ((current_ms - g_near_pause_start_ms) >= MOTOR_PAUSE_5S_MS)
                        {
                            // 5秒结束, 进入运行状态
                            g_near_state = NEAR_RUNNING;
                            g_motor_enabled = true;
                            g_motor_direction = false;
                            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7=0: 正转
                            printf("【5秒停转结束】恢复正转, 进入卡芯检测\r\n");
                        }
                        else
                        {
                            // 仍在停转中
                            g_motor_enabled = false;
                            uint32_t remaining = MOTOR_PAUSE_5S_MS - (current_ms - g_near_pause_start_ms);
                            printf("停转剩余:%ldms\r\n", remaining);
                        }
                        break;
                        
                    case NEAR_RUNNING:
                        // 正常运行: 正转并采集数据用于卡芯检测
                        g_motor_enabled = true;
                        
                        // 存储测距数据到缓冲区
                        g_distance_buffer[g_buffer_index] = dist;
                        printf("数据[%d]:%d mm\r\n", g_buffer_index, dist);
                        
                        g_buffer_index++;
                        if (g_buffer_index >= DATA_BUFFER_SIZE)
                        {
                            g_buffer_index = 0;
                            g_buffer_full = 1;
                        }
                        
                        // 缓冲区满时进行卡芯检测
                        if (g_buffer_full)
                        {
                            // 计算最大值和最小值
                            uint16_t max_val = g_distance_buffer[0];
                            uint16_t min_val = g_distance_buffer[0];
                            
                            for (int i = 1; i < DATA_BUFFER_SIZE; i++)
                            {
                                if (g_distance_buffer[i] > max_val) max_val = g_distance_buffer[i];
                                if (g_distance_buffer[i] < min_val) min_val = g_distance_buffer[i];
                            }
                            
                            uint16_t range = max_val - min_val;
                            printf("卡芯检测: 最大值=%d, 最小值=%d, 极差=%d\r\n", 
                                   max_val, min_val, range);
                            
                            // 极差 ≤ 10mm 触发卡芯锁定
                            if (range <= STUCK_CORE_DIFF_THRESH)
                            {
                                g_stuck_core_locked = true;
                                printf("★★★ 卡芯锁定! 5个数据极差≤%dmm, 进入持续反转 ★★★\r\n", 
                                       STUCK_CORE_DIFF_THRESH);
                                continue;
                            }
                            
                            // 未触发卡芯, 保持正转
                            if (g_motor_direction != false)
                            {
                                g_motor_direction = false;
                                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7=0: 正转
                                printf("【卡芯检测】极差>%dmm, 保持正转\r\n", STUCK_CORE_DIFF_THRESH);
                            }
                        }
                        break;
                }
            }
            else
            {
                printf("【测距错误】传感器读取失败\r\n");
            }
        }

        // ==================== 脉冲生成(独立于测距) ====================
        pulse_generation:
        if (g_motor_enabled)
        {
            if ((DWT_GetTickUs() - g_last_pulse_tick_us) >= MOTOR_PULSE_INTERVAL_US)
            {
                g_last_pulse_tick_us = DWT_GetTickUs();
                
                // 翻转PA6产生脉冲 - 只要电机使能就持续产生脉冲
                GPIOA->ODR ^= GPIO_PIN_6;
                g_pulse_count++;
                
                // 每2000个脉冲打印一次状态
                if (g_pulse_count % 2000 == 0)
                {
                    printf("【脉冲】方向:%s, 计数:%ld\r\n", 
                           g_motor_direction ? "反转" : "正转", g_pulse_count);
                }
            }
        }
        else
        {
            // 电机禁用时, 脉冲引脚置低
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
            g_last_pulse_tick_us = DWT_GetTickUs();
        }
    }
}

// ========================== 系统时钟配置 ==========================
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscinit = {0};
    RCC_ClkInitTypeDef clkinit = {0};

    // 启用HSE外部晶振, PLL倍频9倍 -> 8MHz * 9 = 72MHz
    oscinit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscinit.HSEState = RCC_HSE_ON;
    oscinit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscinit.PLL.PLLState = RCC_PLL_ON;
    oscinit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscinit.PLL.PLLMUL = RCC_PLL_MUL9;
    
    if (HAL_RCC_OscConfig(&oscinit) != HAL_OK)
        Error_Handler();

    // 配置系统时钟: SYSCLK=72MHz, HCLK=72MHz, PCLK2=72MHz, PCLK1=36MHz
    clkinit.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | 
                        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clkinit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkinit.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkinit.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinit.APB1CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&clkinit, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

// ========================== 错误处理 ==========================
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    printf("断言失败: 文件 %s 第 %ld 行\r\n", file, line);
}
#endif