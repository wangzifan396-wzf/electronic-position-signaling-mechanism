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
#define SAMPLE_INTERVAL_MS     100     // 测距采样间隔：100ms
#define MOTOR_PULSE_INTERVAL_US 500     // 电机脉冲间隔：500us (1kHz频率)
#define BUTTON_DEBOUNCE_MS      50      // 按键消抖时间：50ms

// 距离阈值定义
#define DISTANCE_FAR_THRESH     400     // >400mm：远距离，电机正转（伸出）
#define DISTANCE_NEAR_THRESH    400     // <400mm：近距离，内管到位，停止正转
#define FULL_CORE_THRESH_MM     50      // 满芯阈值：连续10次测距 < 50mm
#define STUCK_CORE_DIFF_THRESH   2      // 卡芯阈值：10个数据极差 ≤ 2mm（改为<2mm）

// 时序控制
#define STATUS_PRINT_INTERVAL_US 200000 // 状态打印间隔：200ms

// ========================== 数据结构定义 ==========================
// 距离数据缓冲区 - 存储最近10次测距值，用于满芯/卡芯判断
#define DATA_BUFFER_SIZE        5      // 改为10个数据
static uint16_t g_distance_buffer[DATA_BUFFER_SIZE] = {0};
static uint8_t g_buffer_index = 0;
static uint8_t g_buffer_full = 0;      // 是否已收集满10个数据

// ========================== VL53L0X设备结构体 ==========================
static VL53L0X_Dev_t vl53l0x_dev;

// ========================== 电机控制变量 ==========================
static int32_t g_pulse_count = 0;       // 脉冲计数
static bool g_motor_direction = false;  // false=正转, true=反转
static bool g_motor_enabled = false;    // 电机使能（初始为暂停状态）

// 脉冲生成计时
static uint32_t g_last_pulse_tick_us = 0;

// ========================== 系统运行状态 ==========================
typedef enum {
    SYS_STOPPED = 0,    // 系统停止（暂停）
    SYS_RUNNING         // 系统运行
} SystemState_t;
static SystemState_t g_system_state = SYS_STOPPED;  // 初始为停止状态

// ========================== 状态机变量 ==========================
// 近距离( <400mm )处理状态 - 内管到位后的状态
typedef enum {
    NEAR_IDLE = 0,              // 空闲状态(距离>400mm)
    NEAR_MONITORING             // 监测状态(距离<400mm, 电机停转, 只检测满芯/卡芯)
} NearState_t;
static NearState_t g_near_state = NEAR_IDLE;

// 异常状态标志 - 一旦触发就锁定，直到复位
static bool g_full_core_locked = false;     // 满芯锁定：连续10次<50mm
static bool g_stuck_core_locked = false;    // 卡芯锁定：10个数据极差≤2mm

// 满芯检测计数
static uint8_t g_full_core_count = 0;       // 连续<50mm的计数

// 状态打印计时
static uint32_t g_last_status_print_us = 0;

// 按键状态变量 - 用于检测按键边沿（按下和释放）
static uint8_t g_key1_last_state = 1;  // 上次KEY1状态：1=释放，0=按下
static uint8_t g_key2_last_state = 1;  // 上次KEY2状态：1=释放，0=按下
static uint32_t g_key1_press_time = 0; // KEY1按下时刻
static uint32_t g_key2_press_time = 0; // KEY2按下时刻
static uint8_t g_key1_processed = 0;   // KEY1按下已处理标志
static uint8_t g_key2_processed = 0;   // KEY2按下已处理标志

// ========================== 函数声明 ==========================
void SystemClock_Config(void);
void KEY_GPIO_Config(void);
void Check_Buttons(void);
void System_Start(void);
void System_Stop(void);

// ========================== 按键初始化 ==========================
void KEY_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOA和GPIOC时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // 配置KEY1 (PA0) - 上拉输入，低电平有效
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 配置KEY2 (PC13) - 上拉输入，低电平有效
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    printf("【按键初始化完成】KEY1(PA0)=开始, KEY2(PC13)=暂停\r\n");
}

// ========================== 系统启动函数 ==========================
void System_Start(void)
{
    if (g_system_state == SYS_STOPPED)
    {
        g_system_state = SYS_RUNNING;
        printf("\r\n★★★ 系统启动 ★★★\r\n");
    }
}

// ========================== 系统暂停函数 ==========================
void System_Stop(void)
{
    if (g_system_state == SYS_RUNNING)
    {
        g_system_state = SYS_STOPPED;
        g_motor_enabled = false;  // 禁用电机
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);  // 脉冲引脚置低
        printf("\r\n★★★ 系统暂停 ★★★\r\n");
    }
}

// ========================== 按键检测函数 ==========================
void Check_Buttons(void)
{
    uint32_t current_ms = HAL_GetTick();
    
    // 读取当前按键状态（0=按下，1=释放）
    uint8_t key1_current = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    uint8_t key2_current = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    
    // ========== KEY1检测（开始按钮）==========
    // 检测按下瞬间（上次是释放，这次是按下）
    if (key1_current == 0 && g_key1_last_state == 1)
    {
        g_key1_press_time = current_ms;  // 记录按下时刻
        g_key1_processed = 0;             // 重置处理标志
    }
    
    // 检测释放瞬间（上次是按下，这次是释放）- 只有在按下已处理标志为0时才处理
    if (key1_current == 1 && g_key1_last_state == 0 && g_key1_processed == 0)
    {
        // 检查按下持续时间是否大于消抖时间（有效按键）
        if ((current_ms - g_key1_press_time) >= BUTTON_DEBOUNCE_MS)
        {
            // 只有在系统停止时才能启动
            if (g_system_state == SYS_STOPPED)
            {
                System_Start();
            }
            else
            {
                printf("【按键提示】系统已在运行中\r\n");
            }
            g_key1_processed = 1;  // 标记已处理
        }
    }
    
    // 如果按下时间很短就释放（小于消抖时间），不处理
    if (key1_current == 1 && g_key1_last_state == 0 && g_key1_processed == 0)
    {
        if ((current_ms - g_key1_press_time) < BUTTON_DEBOUNCE_MS)
        {
            g_key1_processed = 1;  // 标记为已处理（忽略）
        }
    }
    
    // ========== KEY2检测（暂停按钮）==========
    // 检测按下瞬间（上次是释放，这次是按下）
    if (key2_current == 0 && g_key2_last_state == 1)
    {
        g_key2_press_time = current_ms;  // 记录按下时刻
        g_key2_processed = 0;             // 重置处理标志
    }
    
    // 检测释放瞬间（上次是按下，这次是释放）- 只有在按下已处理标志为0时才处理
    if (key2_current == 1 && g_key2_last_state == 0 && g_key2_processed == 0)
    {
        // 检查按下持续时间是否大于消抖时间（有效按键）
        if ((current_ms - g_key2_press_time) >= BUTTON_DEBOUNCE_MS)
        {
            // 只有在系统运行时才能暂停
            if (g_system_state == SYS_RUNNING)
            {
                System_Stop();
            }
            else
            {
                printf("【按键提示】系统已处于暂停状态\r\n");
            }
            g_key2_processed = 1;  // 标记已处理
        }
    }
    
    // 如果按下时间很短就释放（小于消抖时间），不处理
    if (key2_current == 1 && g_key2_last_state == 0 && g_key2_processed == 0)
    {
        if ((current_ms - g_key2_press_time) < BUTTON_DEBOUNCE_MS)
        {
            g_key2_processed = 1;  // 标记为已处理（忽略）
        }
    }
    
    // 更新上次状态
    g_key1_last_state = key1_current;
    g_key2_last_state = key2_current;
}

// ========================== 主函数 ==========================
int main(void)
{
    // 基础初始化
    HAL_Init();
    SystemClock_Config();        // 配置系统时钟72MHz
    MX_USART1_UART_Init();       // 串口调试
    DWT_Init();                  // 微秒定时器
    KEY_GPIO_Config();           // 按键初始化
    
    // 初始化VL53L0X传感器 - 使用默认模式（模式0）
    VL53L0X_init(&vl53l0x_dev, 0); // 模式0：默认模式（高精度模式改为0）
    
    board_init();
    DWT_DelayMs(2000);           // 等待系统稳定
    
    printf("\r\n========== 系统启动完成 ==========\r\n");
    printf("【按键功能】\r\n");
    printf("  KEY1(PA0): 开始运行\r\n");
    printf("  KEY2(PC13): 暂停运行\r\n");
    printf("\r\n【工作模式】\r\n");
    printf("  测距间隔: %dms\r\n", SAMPLE_INTERVAL_MS);
    printf("  缓冲区大小: %d个数据\r\n", DATA_BUFFER_SIZE);
    printf("1. 距离 > %dmm : 电机正转（内管伸出）\r\n", DISTANCE_FAR_THRESH);
    printf("2. 距离 < %dmm : 内管到位，电机停转，进入监测状态\r\n", DISTANCE_NEAR_THRESH);
    printf("   - 监测满芯：连续%d次测距 < %dmm → 持续反转(锁定)\r\n", DATA_BUFFER_SIZE, FULL_CORE_THRESH_MM);
    printf("   - 监测卡芯：%d个数据极差 ≤ %dmm → 持续反转(锁定)\r\n", DATA_BUFFER_SIZE, STUCK_CORE_DIFF_THRESH);
    printf("==================================\r\n");
    printf("\r\n系统初始为暂停状态，请按KEY1开始运行\r\n\r\n");

    // 测距数据
    VL53L0X_RangingMeasurementData_t measure_data;
    uint32_t last_measure_tick = 0;

    while (1)
    {
        uint32_t current_ms = HAL_GetTick();
        uint32_t current_us = DWT_GetTickUs();

        // ==================== 按键检测 ====================
        Check_Buttons();

        // ==================== 系统暂停处理 ====================
        if (g_system_state == SYS_STOPPED)
        {
            // 系统暂停时，电机禁用
            g_motor_enabled = false;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);  // 脉冲引脚置低
            
            // 每2秒打印一次提示（避免刷屏）
            static uint32_t last_prompt_ms = 0;
            if ((current_ms - last_prompt_ms) > 2000)
            {
                last_prompt_ms = current_ms;
                printf("【系统暂停】请按KEY1开始运行\r\n");
            }
            
            // 跳过测距逻辑，直接进入脉冲生成
            goto pulse_generation;
        }

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

        // ==================== 定时测距(100ms间隔) ====================
        if ((current_ms - last_measure_tick) >= SAMPLE_INTERVAL_MS)
        {
            last_measure_tick = current_ms;
            
            if (VL53L0X_Task(&vl53l0x_dev, &measure_data) == VL53L0X_ERROR_NONE)
            {
                uint16_t dist = measure_data.RangeMilliMeter;
                printf("当前距离:%d mm\r\n", dist);

                // -------------------- 1. 远距离处理(>400mm) --------------------
                if (dist > DISTANCE_FAR_THRESH)
                {
                    // 内管未到位，电机正转伸出
                    g_near_state = NEAR_IDLE;
                    g_motor_enabled = true;
                    g_motor_direction = false;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // PA7=0: 正转
                    
                    printf("【远距离>%dmm】电机正转伸出\r\n", DISTANCE_FAR_THRESH);
                    continue;
                }

                // -------------------- 2. 近距离处理(<400mm) --------------------
                // 内管到位，电机停转，只进行满芯/卡芯检测
                else
                {
                    // 内管到位，电机停转
                    g_motor_enabled = false;
                    g_near_state = NEAR_MONITORING;
                    
                    printf("【内管到位】距离<%dmm，电机停转，进入监测状态\r\n", DISTANCE_NEAR_THRESH);
                    
                    // -------------------- 满芯检测 --------------------
                    if (dist < FULL_CORE_THRESH_MM)
                    {
                        g_full_core_count++;
                        printf("满芯计数:%d/%d (<%dmm)\r\n", g_full_core_count, DATA_BUFFER_SIZE, FULL_CORE_THRESH_MM);
                        
                        if (g_full_core_count >= DATA_BUFFER_SIZE)
                        {
                            g_full_core_locked = true;
                            g_full_core_count = 0;
                            printf("★★★ 满芯锁定! 连续%d次<%dmm, 进入持续反转 ★★★\r\n", 
                                   DATA_BUFFER_SIZE, FULL_CORE_THRESH_MM);
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
                    
                    // -------------------- 卡芯检测 --------------------
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
                        
                        // 极差 ≤ 2mm 触发卡芯锁定（改为<2mm，即≤2mm）
                        if (range <= STUCK_CORE_DIFF_THRESH)
                        {
                            g_stuck_core_locked = true;
                            printf("★★★ 卡芯锁定! %d个数据极差≤%dmm, 进入持续反转 ★★★\r\n", 
                                   DATA_BUFFER_SIZE, STUCK_CORE_DIFF_THRESH);
                            continue;
                        }
                        
                        // 未触发卡芯，保持停转
                        printf("【卡芯检测】极差>%dmm, 继续监测\r\n", STUCK_CORE_DIFF_THRESH);
                    }
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
                
                // 翻转PA6产生脉冲
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