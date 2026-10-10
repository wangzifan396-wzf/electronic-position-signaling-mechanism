/**
  ******************************************************************************
  * @file       main.c
  * @author     embedfire
  * @version    V1.0
  * @date       2026
  * @brief      SMD18测距电机控制系统 - 满芯/卡芯检测与反钻控制
  ******************************************************************************
  */

#include "main.h"
#include "dwt/bsp_dwt.h"          // 微秒级精确延时
#include "usart/bsp_usart.h"       // 串口调试
#include "board.h"
#include "SMD18.h"                 // 替换为SMD18传感器
#include "usart2.h"                // SMD18的串口
#include <stdbool.h>

// ========================== 系统参数配置 ==========================
#define SAMPLE_INTERVAL_MS     100     // 测距采样间隔：100ms
#define MOTOR_PULSE_INTERVAL_US 500     // 电机脉冲间隔：500us (1kHz频率)
#define BUTTON_DEBOUNCE_MS      50      // 按键消抖时间：50ms

// 距离阈值定义
#define DISTANCE_FAR_THRESH     400     // >400mm：远距离，电机正转（伸出）
#define DISTANCE_NEAR_THRESH    400     // <400mm：近距离，内管到位，停止正转
#define FULL_CORE_THRESH_MM     50      // 满芯阈值：连续10次测距 < 50mm
#define STUCK_CORE_DIFF_THRESH   2      // 卡芯阈值：10个数据极差 ≤ 2mm

// 时序控制
#define STATUS_PRINT_INTERVAL_US 200000 // 状态打印间隔：200ms

// ========================== 数据结构定义 ==========================
// 距离数据缓冲区 - 存储最近10次测距值
#define DATA_BUFFER_SIZE        5
static uint16_t g_distance_buffer[DATA_BUFFER_SIZE] = {0};
static uint8_t g_buffer_index = 0;
static uint8_t g_buffer_full = 0;

// ========================== 电机控制变量 ==========================
static int32_t g_pulse_count = 0;       // 脉冲计数
static bool g_motor_direction = false;  // false=正转, true=反转
static bool g_motor_enabled = false;    // 电机使能
static uint32_t g_last_pulse_tick_us = 0;

// ========================== 系统运行状态 ==========================
typedef enum {
    SYS_STOPPED = 0,
    SYS_RUNNING
} SystemState_t;
static SystemState_t g_system_state = SYS_STOPPED;

// ========================== 状态机变量 ==========================
typedef enum {
    NEAR_IDLE = 0,
    NEAR_MONITORING
} NearState_t;
static NearState_t g_near_state = NEAR_IDLE;

static bool g_full_core_locked = false;
static bool g_stuck_core_locked = false;
static uint8_t g_full_core_count = 0;
static uint32_t g_last_status_print_us = 0;

// 按键状态变量
static uint8_t g_key1_last_state = 1;
static uint8_t g_key2_last_state = 1;
static uint32_t g_key1_press_time = 0;
static uint32_t g_key2_press_time = 0;
static uint8_t g_key1_processed = 0;
static uint8_t g_key2_processed = 0;

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
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    printf("【按键初始化完成】KEY1(PA0)=开始, KEY2(PC13)=暂停\r\n");
}

void System_Start(void)
{
    if (g_system_state == SYS_STOPPED)
    {
        g_system_state = SYS_RUNNING;
        printf("\r\n★★★ 系统启动 ★★★\r\n");
    }
}

void System_Stop(void)
{
    if (g_system_state == SYS_RUNNING)
    {
        g_system_state = SYS_STOPPED;
        g_motor_enabled = false;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
        printf("\r\n★★★ 系统暂停 ★★★\r\n");
    }
}

void Check_Buttons(void)
{
    uint32_t current_ms = HAL_GetTick();
    
    uint8_t key1_current = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    uint8_t key2_current = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    
    // KEY1检测
    if (key1_current == 0 && g_key1_last_state == 1)
    {
        g_key1_press_time = current_ms;
        g_key1_processed = 0;
    }
    
    if (key1_current == 1 && g_key1_last_state == 0 && g_key1_processed == 0)
    {
        if ((current_ms - g_key1_press_time) >= BUTTON_DEBOUNCE_MS)
        {
            if (g_system_state == SYS_STOPPED)
            {
                System_Start();
            }
            else
            {
                printf("【按键提示】系统已在运行中\r\n");
            }
            g_key1_processed = 1;
        }
    }
    
    // KEY2检测
    if (key2_current == 0 && g_key2_last_state == 1)
    {
        g_key2_press_time = current_ms;
        g_key2_processed = 0;
    }
    
    if (key2_current == 1 && g_key2_last_state == 0 && g_key2_processed == 0)
    {
        if ((current_ms - g_key2_press_time) >= BUTTON_DEBOUNCE_MS)
        {
            if (g_system_state == SYS_RUNNING)
            {
                System_Stop();
            }
            else
            {
                printf("【按键提示】系统已处于暂停状态\r\n");
            }
            g_key2_processed = 1;
        }
    }
    
    g_key1_last_state = key1_current;
    g_key2_last_state = key2_current;
}

// ========================== 主函数 ==========================
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();       // 串口调试
    DWT_Init();
    KEY_GPIO_Config();
    MX_USART2_UART_Init();  // 初始化SMD18的串口
    // 初始化SMD18传感器
    SMD18_Init();
    
    board_init();
    DWT_DelayMs(2000);
    
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

    uint32_t last_measure_tick = 0;

    while (1)
    {
        uint32_t current_ms = HAL_GetTick();
        uint32_t current_us = DWT_GetTickUs();

        Check_Buttons();

        if (g_system_state == SYS_STOPPED)
        {
            g_motor_enabled = false;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
            
            static uint32_t last_prompt_ms = 0;
            if ((current_ms - last_prompt_ms) > 2000)
            {
                last_prompt_ms = current_ms;
                printf("【系统暂停】请按KEY1开始运行\r\n");
            }
            
            goto pulse_generation;
        }

        if (g_full_core_locked || g_stuck_core_locked)
        {
            g_motor_enabled = true;
            
            if (g_motor_direction != true)
            {
                g_motor_direction = true;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
                printf("【%s】设置为持续反转模式\r\n", 
                       g_full_core_locked ? "满芯锁定" : "卡芯锁定");
            }
            
            if ((current_us - g_last_status_print_us) >= STATUS_PRINT_INTERVAL_US)
            {
                g_last_status_print_us = current_us;
                printf("【%s持续反转中】脉冲计数:%ld\r\n", 
                       g_full_core_locked ? "满芯" : "卡芯", g_pulse_count);
            }
            
            goto pulse_generation;
        }

        // ==================== 定时测距(100ms间隔) ====================
        if ((current_ms - last_measure_tick) >= SAMPLE_INTERVAL_MS)
        {
            last_measure_tick = current_ms;
            
            // 使用SMD18读取距离
            uint16_t dist = SMD18_GetDistance_MM();
            if (dist > 0)  // 有效距离
            {
                printf("当前距离:%d mm\r\n", dist);

                // 远距离处理
                if (dist > DISTANCE_FAR_THRESH)
                {
                    g_near_state = NEAR_IDLE;
                    g_motor_enabled = true;
                    g_motor_direction = false;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
                    
                    printf("【远距离>%dmm】电机正转伸出\r\n", DISTANCE_FAR_THRESH);
                    continue;
                }
                // 近距离处理
                else
                {
                    g_motor_enabled = false;
                    g_near_state = NEAR_MONITORING;
                    
                    printf("【内管到位】距离<%dmm，电机停转，进入监测状态\r\n", DISTANCE_NEAR_THRESH);
                    
                    // 满芯检测
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
                    
                    // 卡芯检测
                    g_distance_buffer[g_buffer_index] = dist;
                    printf("数据[%d]:%d mm\r\n", g_buffer_index, dist);
                    
                    g_buffer_index++;
                    if (g_buffer_index >= DATA_BUFFER_SIZE)
                    {
                        g_buffer_index = 0;
                        g_buffer_full = 1;
                    }
                    
                    if (g_buffer_full)
                    {
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
                        
                        if (range <= STUCK_CORE_DIFF_THRESH)
                        {
                            g_stuck_core_locked = true;
                            printf("★★★ 卡芯锁定! %d个数据极差≤%dmm, 进入持续反转 ★★★\r\n", 
                                   DATA_BUFFER_SIZE, STUCK_CORE_DIFF_THRESH);
                            continue;
                        }
                        
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
                
                if (g_pulse_count % 2000 == 0)
                {
                    printf("【脉冲】方向:%s, 计数:%ld\r\n", 
                           g_motor_direction ? "反转" : "正转", g_pulse_count);
                }
            }
        }
        else
        {
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

    oscinit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscinit.HSEState = RCC_HSE_ON;
    oscinit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscinit.PLL.PLLState = RCC_PLL_ON;
    oscinit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscinit.PLL.PLLMUL = RCC_PLL_MUL9;
    
    if (HAL_RCC_OscConfig(&oscinit) != HAL_OK)
        Error_Handler();

    clkinit.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | 
                        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clkinit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkinit.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkinit.APB2CLKDivider = RCC_HCLK_DIV1;
    clkinit.APB1CLKDivider = RCC_HCLK_DIV2;
    
    if (HAL_RCC_ClockConfig(&clkinit, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

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