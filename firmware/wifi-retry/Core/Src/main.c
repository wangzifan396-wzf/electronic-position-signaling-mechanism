/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 新型钻具机构控制系统 - 稳定生产版
  *                   - 激光雷达测距控制内管升降
  *                   - 内管到位400mm停止
  *                   - 到位后才开始卡芯/满芯判断
  *                   - 卡芯/满芯时反转上提到800mm
  *                   - WiFi无线数据传输（ESP8266）
  *                   - 支持远程参数修改
  *                   - 支持手动控制
  *                   - 自动重连机制
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

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

// 默认阈值定义（单位：mm）
#define DEFAULT_THRESHOLD_GROUND    800     // 默认地面阈值
#define DEFAULT_THRESHOLD_IN_PLACE  400     // 默认内管到位阈值
#define DEFAULT_THRESHOLD_FULL      100     // 满芯阈值
#define DEFAULT_THRESHOLD_STUCK     2       // 卡芯差值阈值

// 数组大小定义
#define DIST_ARRAY_SIZE     10      // 10个相邻测距的数组

// 时间定义（单位：ms）
#define MEASURE_INTERVAL        100     // 100ms测一次距离
#define PAUSE_TIME_IN_PLACE     5000    // 内管到位暂停时间（5秒）
#define PAUSE_TIME_CORE         3000    // 卡芯/满芯暂停时间（3秒）
#define DEFAULT_MOTOR_SPEED     100    // 默认电机速度
#define WIFI_RECONNECT_INTERVAL 30000   // WiFi重连间隔（30秒）

// ==================== WiFi配置（请修改为你的网络） ====================
#define WIFI_UART_HANDLE      &huart3      // USART3连接WiFi
#define WIFI_RST_PIN          GPIO_PIN_9   // WiFi复位引脚（PB9）
#define WIFI_RST_PORT         GPIOB
#define WIFI_IO_PIN           GPIO_PIN_8   // IO引脚（PB8），悬空

// WiFi连接参数（请修改成你自己的）
#include "wifi_config.h"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 电机控制变量
__IO int32_t pulse_count = 0;
__IO uint8_t motor_status = MODE_IDLE;

// 可修改的参数
__IO uint16_t threshold_ground = DEFAULT_THRESHOLD_GROUND;
__IO uint16_t threshold_in_place = DEFAULT_THRESHOLD_IN_PLACE;
__IO uint16_t threshold_full = DEFAULT_THRESHOLD_FULL;
__IO uint16_t threshold_stuck = DEFAULT_THRESHOLD_STUCK;
__IO uint16_t motor_speed = DEFAULT_MOTOR_SPEED;

// 手动控制标志
__IO uint8_t manual_control = 0;
__IO uint8_t manual_cmd = 0;

// 钻具控制变量
__IO uint8_t drill_mode = MODE_IDLE;
__IO uint32_t pause_start_time = 0;
__IO uint8_t is_reversing = 0;
__IO uint8_t is_in_place = 0;

// 测距数组
__IO uint16_t dist_array[DIST_ARRAY_SIZE] = {0};
__IO uint8_t dist_array_index = 0;
__IO uint8_t dist_array_full = 0;

// 传感器距离
extern uint16_t gSDM18_Distance;

// WiFi状态
__IO uint8_t wifi_connected = 0;
__IO uint32_t last_wifi_check = 0;

// 指令接收缓冲区
uint8_t cmd_buffer[256];
uint8_t cmd_index = 0;
uint8_t rx_byte = 0;
volatile uint8_t cmd_ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 电机控制
void Motor_GPIO_Init(void);
void Motor_Set_Direction(uint8_t dir);
void Motor_Generate_Pulse(void);
void Drill_Control(uint16_t dist);
void Update_Distance_Array(uint16_t dist);
uint8_t Check_Stuck(void);
uint8_t Check_Full(void);
void Print_Status(uint16_t dist);

// WiFi功能
void WiFi_GPIO_Init(void);
void WiFi_Reset(void);
uint8_t WiFi_SendAT(const char* cmd, const char* expected, uint32_t timeout);
void WiFi_Connect(void);
void WiFi_SendData(uint8_t* data, uint16_t len);
void WiFi_SendStatus(uint16_t dist, uint8_t mode, uint8_t in_place, uint8_t reversing, 
                      uint16_t avg, uint16_t diff);
void WiFi_CheckAndReconnect(void);
void Parse_Command(char* cmd);
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  
  /* USER CODE BEGIN 2 */
  // 系统初始化
  BSP_init();
  Motor_GPIO_Init();
  WiFi_GPIO_Init();
  
  // 清空距离数组
  memset((void*)dist_array, 0, sizeof(dist_array));
  
  // 打印启动信息
  printf("\r\n\r\n");
  printf("========================================\r\n");
  printf("    新型钻具机构控制系统 - 稳定版      \r\n");
  printf("========================================\r\n");
  printf("系统启动中...\r\n");
  
  // 尝试连接WiFi
  WiFi_Reset();
  HAL_Delay(1000);
  WiFi_Connect();
  last_wifi_check = HAL_GetTick();
  
  if(wifi_connected)
  {
      printf("✓ WiFi连接成功！\r\n");
      printf("  服务器: %s:%s\r\n", SERVER_IP, SERVER_PORT);
      // 启动指令接收
      HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
  }
  else
  {
      printf("✗ WiFi连接失败，系统将独立运行\r\n");
      printf("  如需WiFi请检查网络设置后按复位键\r\n");
  }
  
  // 显示当前配置
  printf("\r\n当前配置：\r\n");
  printf("  内管到位阈值: %dmm\r\n", threshold_in_place);
  printf("  地面阈值: %dmm\r\n", threshold_ground);
  printf("  满芯阈值: %dmm\r\n", threshold_full);
  printf("  卡芯差值: %dmm\r\n", threshold_stuck);
  printf("  电机速度: %d\r\n", motor_speed);
  printf("========================================\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    BSP_Loop();  // 处理传感器数据
    Motor_Generate_Pulse();
    
    // 处理接收到的指令
    if(cmd_ready)
    {
        cmd_ready = 0;
        Parse_Command((char*)cmd_buffer);
    }
    
    // WiFi自动重连（每30秒检查一次）
    WiFi_CheckAndReconnect();
    
    // 每100ms执行控制逻辑
    static uint32_t last_control = 0;
    if(HAL_GetTick() - last_control >= MEASURE_INTERVAL)
    {
        last_control = HAL_GetTick();
        uint16_t current_dist = gSDM18_Distance;
        Update_Distance_Array(current_dist);
        Drill_Control(current_dist);
        Print_Status(current_dist);
    }
    
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

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

// ==================== 电机控制函数 ====================

void Motor_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = MOTOR_EN_PIN | MOTOR_STP_PIN | MOTOR_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_EN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
}

void Motor_Set_Direction(uint8_t dir)
{
    if(dir == MODE_DOWN)
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET);
    else if(dir == MODE_UP)
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
}

void Motor_Generate_Pulse(void)
{
    if(motor_status != MODE_DOWN && motor_status != MODE_UP)
        return;
    
    for(volatile int i = 0; i < motor_speed; i++)
        __NOP();
    
    HAL_GPIO_TogglePin(MOTOR_PORT, MOTOR_STP_PIN);
    pulse_count++;
}

// ==================== 距离检测函数 ====================

void Update_Distance_Array(uint16_t dist)
{
    dist_array[dist_array_index] = dist;
    dist_array_index++;
    if(dist_array_index >= DIST_ARRAY_SIZE)
    {
        dist_array_index = 0;
        dist_array_full = 1;
    }
}

uint8_t Check_Stuck(void)
{
    if(!dist_array_full) return 0;
    
    uint16_t max_dist = 0, min_dist = 65535;
    for(int i = 0; i < DIST_ARRAY_SIZE; i++)
    {
        if(dist_array[i] > max_dist) max_dist = dist_array[i];
        if(dist_array[i] < min_dist) min_dist = dist_array[i];
    }
    
    return ((max_dist - min_dist) < threshold_stuck) ? 1 : 0;
}

uint8_t Check_Full(void)
{
    if(!dist_array_full) return 0;
    
    for(int i = 0; i < DIST_ARRAY_SIZE; i++)
    {
        if(dist_array[i] >= threshold_full) return 0;
    }
    return 1;
}

// ==================== 钻具控制主逻辑 ====================

void Drill_Control(uint16_t dist)
{
    if(dist == 0) return;
    
    // 手动模式
    if(manual_control == 1)
    {
        if(manual_cmd == 1 && motor_status != MODE_DOWN)
        {
            motor_status = MODE_DOWN;
            drill_mode = MODE_DOWN;
            is_reversing = 0;
            is_in_place = 0;
            Motor_Set_Direction(MODE_DOWN);
            printf("手动: 下钻\r\n");
            WiFi_SendData((uint8_t*)"!!! MANUAL: DRILLING START !!!\r\n", 35);
        }
        else if(manual_cmd == 2 && motor_status != MODE_UP)
        {
            motor_status = MODE_UP;
            drill_mode = MODE_UP;
            is_reversing = 1;
            is_in_place = 0;
            Motor_Set_Direction(MODE_UP);
            printf("手动: 上提\r\n");
            WiFi_SendData((uint8_t*)"!!! MANUAL: REVERSING START !!!\r\n", 35);
        }
        else if(manual_cmd == 3 && motor_status != MODE_IDLE)
        {
            motor_status = MODE_IDLE;
            drill_mode = MODE_IDLE;
            is_reversing = 0;
            is_in_place = 0;
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
            printf("手动: 急停\r\n");
            WiFi_SendData((uint8_t*)"!!! EMERGENCY STOP !!!\r\n", 25);
        }
        return;
    }
    
    // 自动模式
    if(drill_mode == MODE_PAUSE)
    {
        if(HAL_GetTick() - pause_start_time >= PAUSE_TIME_CORE)
        {
            drill_mode = MODE_UP;
            motor_status = MODE_UP;
            is_reversing = 1;
            is_in_place = 0;
            Motor_Set_Direction(MODE_UP);
            printf("⬆️ 暂停结束，反转上提（目标%dmm）\r\n", threshold_ground);
            WiFi_SendData((uint8_t*)"!!! ACTION: REVERSING START !!!\r\n", 35);
        }
        return;
    }
    
    if(dist >= threshold_ground)
    {
        if(drill_mode != MODE_IDLE || is_reversing != 0)
        {
            drill_mode = MODE_IDLE;
            motor_status = MODE_IDLE;
            is_reversing = 0;
            is_in_place = 0;
            printf("🏁 到达地面阈值 %dmm，停止\r\n", threshold_ground);
            WiFi_SendData((uint8_t*)"!!! STATUS: AT GROUND !!!\r\n", 29);
        }
        return;
    }
    
    if(is_reversing == 1)
    {
        if(drill_mode != MODE_UP)
        {
            drill_mode = MODE_UP;
            motor_status = MODE_UP;
            Motor_Set_Direction(MODE_UP);
        }
        return;
    }
    
    if(dist <= threshold_in_place)
    {
        if(drill_mode != MODE_IDLE)
        {
            drill_mode = MODE_IDLE;
            motor_status = MODE_IDLE;
            printf("⏹️ 内管到位 %dmm，停止\r\n", dist);
            char alert[64];
            sprintf(alert, "*** STATUS: IN_PLACE (dist=%dmm) ***\r\n", dist);
            WiFi_SendData((uint8_t*)alert, strlen(alert));
        }
        
        if(is_in_place == 0)
        {
            is_in_place = 1;
            printf("🔍 开始卡芯/满芯监测...\r\n");
            uint32_t pause_start = HAL_GetTick();
            while(HAL_GetTick() - pause_start < PAUSE_TIME_IN_PLACE)
                HAL_Delay(10);
        }
        
        if(is_in_place == 1)
        {
            if(Check_Full())
            {
                drill_mode = MODE_PAUSE;
                motor_status = MODE_IDLE;
                pause_start_time = HAL_GetTick();
                printf("⏸️ 满芯！暂停3秒后反转上提\r\n");
                WiFi_SendData((uint8_t*)"!!! ALERT: CORE FULL !!!\r\n", 28);
                return;
            }
            if(Check_Stuck())
            {
                drill_mode = MODE_PAUSE;
                motor_status = MODE_IDLE;
                pause_start_time = HAL_GetTick();
                printf("⚠️ 卡芯！暂停3秒后反转上提\r\n");
                WiFi_SendData((uint8_t*)"!!! ALERT: CORE STUCK !!!\r\n", 28);
                return;
            }
        }
        return;
    }
    
    if(dist > threshold_in_place && drill_mode != MODE_DOWN)
    {
        drill_mode = MODE_DOWN;
        motor_status = MODE_DOWN;
        is_in_place = 0;
        Motor_Set_Direction(MODE_DOWN);
        printf("⬇️ 距离=%dmm > %dmm，正转下钻\r\n", dist, threshold_in_place);
    }
}

// ==================== 指令解析 ====================

void Parse_Command(char* cmd)
{
    if(strlen(cmd) > 100) return;
    printf("收到指令: %s\r\n", cmd);
    
    if(strstr(cmd, "SET_IN_PLACE|") != NULL)
    {
        uint16_t val = atoi(cmd + 13);
        if(val >= 100 && val <= 1000)
        {
            threshold_in_place = val;
            printf("内管到位阈值已修改为: %dmm\r\n", threshold_in_place);
            WiFi_SendData((uint8_t*)"OK|IN_PLACE_CHANGED\r\n", 23);
        }
        else WiFi_SendData((uint8_t*)"ERROR|INVALID_VALUE\r\n", 22);
        return;
    }
    
    if(strstr(cmd, "SET_GROUND|") != NULL)
    {
        uint16_t val = atoi(cmd + 11);
        if(val >= 500 && val <= 1500)
        {
            threshold_ground = val;
            printf("地面阈值已修改为: %dmm\r\n", threshold_ground);
            WiFi_SendData((uint8_t*)"OK|GROUND_CHANGED\r\n", 22);
        }
        else WiFi_SendData((uint8_t*)"ERROR|INVALID_VALUE\r\n", 22);
        return;
    }
    
    if(strstr(cmd, "SET_FULL|") != NULL)
    {
        uint16_t val = atoi(cmd + 9);
        if(val >= 20 && val <= 200)
        {
            threshold_full = val;
            printf("满芯阈值已修改为: %dmm\r\n", threshold_full);
            WiFi_SendData((uint8_t*)"OK|FULL_CHANGED\r\n", 20);
        }
        else WiFi_SendData((uint8_t*)"ERROR|INVALID_VALUE\r\n", 22);
        return;
    }
    
    if(strstr(cmd, "SET_STUCK|") != NULL)
    {
        uint16_t val = atoi(cmd + 10);
        if(val >= 1 && val <= 50)
        {
            threshold_stuck = val;
            printf("卡芯差值阈值已修改为: %dmm\r\n", threshold_stuck);
            WiFi_SendData((uint8_t*)"OK|STUCK_CHANGED\r\n", 21);
        }
        else WiFi_SendData((uint8_t*)"ERROR|INVALID_VALUE\r\n", 22);
        return;
    }
    
    if(strstr(cmd, "SET_SPEED|") != NULL)
    {
        uint16_t val = atoi(cmd + 10);
        if(val >= 100 && val <= 5000)
        {
            motor_speed = val;
            printf("电机速度已修改: 延时=%d\r\n", motor_speed);
            WiFi_SendData((uint8_t*)"OK|SPEED_CHANGED\r\n", 21);
        }
        else WiFi_SendData((uint8_t*)"ERROR|INVALID_VALUE\r\n", 22);
        return;
    }
    
    if(strstr(cmd, "GET_PARAMS") != NULL)
    {
        char buffer[128];
        sprintf(buffer, "PARAMS|IN_PLACE=%d|GROUND=%d|FULL=%d|STUCK=%d|SPEED=%d\r\n",
                threshold_in_place, threshold_ground, threshold_full, threshold_stuck, motor_speed);
        WiFi_SendData((uint8_t*)buffer, strlen(buffer));
        return;
    }
    
    if(strstr(cmd, "CMD_DOWN") != NULL)
    {
        manual_control = 1;
        manual_cmd = 1;
        printf("手动下钻模式\r\n");
        WiFi_SendData((uint8_t*)"OK|MANUAL_DOWN\r\n", 18);
        return;
    }
    
    if(strstr(cmd, "CMD_UP") != NULL)
    {
        manual_control = 1;
        manual_cmd = 2;
        printf("手动上提模式\r\n");
        WiFi_SendData((uint8_t*)"OK|MANUAL_UP\r\n", 16);
        return;
    }
    
    if(strstr(cmd, "CMD_STOP") != NULL)
    {
        manual_control = 1;
        manual_cmd = 3;
        printf("急停！\r\n");
        WiFi_SendData((uint8_t*)"OK|EMERGENCY_STOP\r\n", 21);
        return;
    }
    
    if(strstr(cmd, "CMD_RESET") != NULL)
    {
        manual_control = 0;
        manual_cmd = 0;
        motor_status = MODE_IDLE;
        drill_mode = MODE_IDLE;
        is_reversing = 0;
        is_in_place = 0;
        HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_STP_PIN, GPIO_PIN_RESET);
        printf("复位到自动模式\r\n");
        WiFi_SendData((uint8_t*)"OK|RESET_TO_AUTO\r\n", 20);
        return;
    }
    
    WiFi_SendData((uint8_t*)"ERROR|UNKNOWN_CMD\r\n", 20);
}

// ==================== WiFi模块函数 ====================

void WiFi_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = WIFI_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(WIFI_RST_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = WIFI_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(WIFI_RST_PORT, &GPIO_InitStruct);
}

void WiFi_Reset(void)
{
    HAL_GPIO_WritePin(WIFI_RST_PORT, WIFI_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(WIFI_RST_PORT, WIFI_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(2000);
}

uint8_t WiFi_SendAT(const char* cmd, const char* expected, uint32_t timeout)
{
    uint8_t buffer[128];
    uint32_t start = HAL_GetTick();
    uint16_t index = 0;
    
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)cmd, strlen(cmd), 100);
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)"\r\n", 2, 100);
    
    while(HAL_GetTick() - start < timeout)
    {
        if(HAL_UART_Receive(WIFI_UART_HANDLE, &buffer[index], 1, 50) == HAL_OK)
        {
            if(index < sizeof(buffer)-1)
            {
                index++;
                buffer[index] = 0;
                if(strstr((char*)buffer, expected) != NULL)
                    return 1;
            }
        }
    }
    return 0;
}

void WiFi_Connect(void)
{
    char cmd[128];
    
    printf("WiFi连接中...\r\n");
    
    // 退出透传模式
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)"+++", 3, 100);
    HAL_Delay(500);
    
    // 测试AT
    if(!WiFi_SendAT("AT", "OK", 2000))
    {
        printf("WiFi模块无响应\r\n");
        wifi_connected = 0;
        return;
    }
    
    // 设置STA模式
    WiFi_SendAT("AT+CWMODE=1", "OK", 2000);
    
    // 连接WiFi
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    if(!WiFi_SendAT(cmd, "OK", 10000))
    {
        printf("WiFi连接失败\r\n");
        wifi_connected = 0;
        return;
    }
    printf("WiFi已连接\r\n");
    HAL_Delay(2000);
    
    // 连接TCP服务器
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", SERVER_IP, SERVER_PORT);
    if(!WiFi_SendAT(cmd, "CONNECT", 5000))
    {
        printf("TCP连接失败\r\n");
        wifi_connected = 0;
        return;
    }
    printf("TCP已连接\r\n");
    HAL_Delay(500);
    
    // 开启透传
    WiFi_SendAT("AT+CIPMODE=1", "OK", 2000);
    WiFi_SendAT("AT+CIPSEND", ">", 2000);
    
    wifi_connected = 1;
    printf("✓ WiFi透传模式已开启\r\n");
}

void WiFi_CheckAndReconnect(void)
{
    if(HAL_GetTick() - last_wifi_check >= WIFI_RECONNECT_INTERVAL)
    {
        last_wifi_check = HAL_GetTick();
        
        if(!wifi_connected)
        {
            printf("WiFi断开，尝试重连...\r\n");
            WiFi_Connect();
            if(wifi_connected)
                HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
        }
    }
}

void WiFi_SendData(uint8_t* data, uint16_t len)
{
    if(wifi_connected)
        HAL_UART_Transmit(WIFI_UART_HANDLE, data, len, 100);
}

void WiFi_SendStatus(uint16_t dist, uint8_t mode, uint8_t in_place, uint8_t reversing, 
                      uint16_t avg, uint16_t diff)
{
    char buffer[256];
    char *mode_str = "", *status_str = "";
    
    switch(mode)
    {
        case MODE_IDLE:   mode_str = "IDLE"; break;
        case MODE_DOWN:   mode_str = "DOWN"; break;
        case MODE_UP:     mode_str = "UP"; break;
        case MODE_PAUSE:  mode_str = "PAUSE"; break;
        default:          mode_str = "UNKNOWN"; break;
    }
    
    if(in_place) status_str = "IN_PLACE";
    else if(reversing) status_str = "REVERSING";
    else if(mode == MODE_DOWN) status_str = "DRILLING";
    else if(mode == MODE_IDLE) status_str = "WAITING";
    else if(mode == MODE_PAUSE) status_str = "PAUSING";
    else status_str = "NORMAL";
    
    sprintf(buffer, "DATA|%d|%d|%s|%s|%d|%d|%d|%d|%d|%d|%d\r\n",
            (int)(HAL_GetTick()/1000), dist, mode_str, status_str, 
            in_place, reversing, avg, diff,
            threshold_in_place, threshold_ground, motor_speed);
    
    WiFi_SendData((uint8_t*)buffer, strlen(buffer));
}

void Print_Status(uint16_t dist)
{
    static uint32_t last_print = 0;
    
    if(HAL_GetTick() - last_print >= MEASURE_INTERVAL)
    {
        last_print = HAL_GetTick();
        
        uint16_t max_d = 0, min_d = 65535;
        uint32_t sum_d = 0;
        
        for(int i = 0; i < DIST_ARRAY_SIZE; i++)
        {
            if(dist_array[i] > max_d) max_d = dist_array[i];
            if(dist_array[i] < min_d) min_d = dist_array[i];
            sum_d += dist_array[i];
        }
        
        uint16_t avg_d = dist_array_full ? (sum_d / DIST_ARRAY_SIZE) : 0;
        
        char *mode_str = "";
        switch(drill_mode)
        {
            case MODE_IDLE:   mode_str = "空闲"; break;
            case MODE_DOWN:   mode_str = "下钻"; break;
            case MODE_UP:     mode_str = "上提"; break;
            case MODE_PAUSE:  mode_str = "暂停"; break;
        }
        
        printf("[%03d] dist=%4dmm | %s |到位=%d|rev=%d| avg=%3dmm| diff=%2dmm\r\n", 
               (HAL_GetTick()/1000)%1000, dist, mode_str, is_in_place, 
               is_reversing, avg_d, max_d - min_d);
        
        WiFi_SendStatus(dist, drill_mode, is_in_place, is_reversing, avg_d, max_d - min_d);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)
    {
        if(rx_byte == '\n' || cmd_index >= 255)
        {
            if(cmd_index > 0)
            {
                cmd_buffer[cmd_index] = 0;
                cmd_ready = 1;
                cmd_index = 0;
            }
        }
        else if(rx_byte != '\r')
        {
            cmd_buffer[cmd_index++] = rx_byte;
        }
        HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
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