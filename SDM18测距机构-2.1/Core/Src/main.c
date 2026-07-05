/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 新型钻具机构控制系统
  *                   - 激光雷达测距控制内管升降
  *                   - 内管到位400mm停止
  *                   - 到位后才开始卡芯/满芯判断
  *                   - 卡芯/满芯时反转上提到800mm
  *                   - WiFi无线数据传输（ESP8266）
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

// 距离阈值定义（单位：mm）
#define THRESHOLD_GROUND    800     // 地面上阈值（内管在顶部）
#define THRESHOLD_IN_PLACE  400     // 内管到位阈值（到此停止）
#define THRESHOLD_FULL      100     // 满芯阈值（小于100mm为满）
#define THRESHOLD_STUCK     2       // 卡芯判断差值（2mm以内为卡）

// 数组大小定义
#define DIST_ARRAY_SIZE     10      // 10个相邻测距的数组

// 时间定义（单位：ms）
#define MEASURE_INTERVAL        100     // 100ms测一次距离
#define PAUSE_TIME_IN_PLACE     5000    // 内管到位暂停时间（5秒）
#define PAUSE_TIME_CORE         3000    // 卡芯/满芯暂停时间（3秒）

// ==================== WiFi配置 ====================
// 接线：PB10->RXD, PB11->TXD, PB9->RST, PB8->IO(悬空)
#define WIFI_UART_HANDLE      &huart3      // USART3连接WiFi
#define WIFI_RST_PIN          GPIO_PIN_9   // WiFi复位引脚（PB9）
#define WIFI_RST_PORT         GPIOB
#define WIFI_IO_PIN           GPIO_PIN_8   // IO引脚（PB8），悬空即可

// WiFi连接参数（已配置好）
#define WIFI_SSID             "632"                // WiFi名称
#define WIFI_PASSWORD         "2022031004632"      // WiFi密码
#define SERVER_IP             "192.168.1.105"      // 电脑IP
#define SERVER_PORT           "8080"               // 端口号
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

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

// WiFi连接标志
__IO uint8_t wifi_connected = 0;
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

// WiFi相关函数声明
void WiFi_GPIO_Init(void);
void WiFi_Reset(void);
uint8_t WiFi_SendAT(const char* cmd, const char* expected, uint32_t timeout);
void WiFi_Connect(void);
void WiFi_SendData(uint8_t* data, uint16_t len);
void WiFi_SendStatus(uint16_t dist, uint8_t mode, uint8_t in_place, uint8_t reversing, 
                      uint16_t avg, uint16_t diff);
uint8_t WiFi_CheckConnection(void);
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
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  BSP_init();  // 传感器初始化
  
  // 电机GPIO初始化
  Motor_GPIO_Init();
  
  // WiFi模块引脚初始化
  WiFi_GPIO_Init();
  
  // 复位WiFi模块
  WiFi_Reset();
  HAL_Delay(1000);
  
  // 配置WiFi连接
  WiFi_Connect();
  
  // 清空距离数组
  memset((void*)dist_array, 0, sizeof(dist_array));
  
  printf("\r\n====================================\r\n");
  printf("新型钻具机构控制系统启动\r\n");
  printf("WiFi已启动，数据将无线传输\r\n");
  printf("WiFi名称: %s\r\n", WIFI_SSID);
  printf("电脑IP: %s:%s\r\n", SERVER_IP, SERVER_PORT);
  printf("阈值设置：到位=400mm(停止) 满芯=<100mm 卡芯差值<2mm\r\n");
  printf("地面阈值：800mm(反转停止)\r\n");
  printf("测距间隔：100ms\r\n");
  printf("暂停时间：内管到位=%dms 卡芯/满芯=%dms\r\n", PAUSE_TIME_IN_PLACE, PAUSE_TIME_CORE);
  printf("====================================\r\n\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    BSP_Loop();  // 处理传感器数据（会更新gSDM18_Distance）
    
    // 产生电机脉冲
    Motor_Generate_Pulse();
    
    // 只在WiFi未连接时才检查（避免干扰透传）
    if(wifi_connected == 0)
    {
        WiFi_CheckConnection();
    }
    
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
        
        // 串口打印状态（同时通过WiFi发送）
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
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

  /** Initializes the CPU, AHB and APB buses clocks
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
  * @brief 电机GPIO初始化
  */
void Motor_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PA5、PA6和PA7作为普通推挽输出
    GPIO_InitStruct.Pin = MOTOR_EN_PIN | MOTOR_STP_PIN | MOTOR_DIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_PORT, &GPIO_InitStruct);
    
    // 初始状态
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
    
    // 使用简单的空循环实现微秒级延时
    // 调整这个数值可以改变电机速度：减小变快，增大变慢
    for(volatile int i = 0; i < 1000; i++)
    {
        __NOP();  // 空操作
    }
    
    // 翻转脉冲引脚
    HAL_GPIO_TogglePin(MOTOR_PORT, MOTOR_STP_PIN);
    pulse_count++;
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
  */
void Drill_Control(uint16_t dist)
{
    // 如果距离为0（还没收到有效数据），不动作
    if(dist == 0) return;
    
    /********** 第一步：处理暂停状态 **********/
    if(drill_mode == MODE_PAUSE)
    {
        // 检查是否到达暂停时间（卡芯/满芯用 PAUSE_TIME_CORE）
        if(HAL_GetTick() - pause_start_time >= PAUSE_TIME_CORE)
        {
            // 暂停结束，开始反转上提
            drill_mode = MODE_UP;
            motor_status = MODE_UP;
            is_reversing = 1;  // 标记为反转阶段
            is_in_place = 0;    // 离开到位状态
            Motor_Set_Direction(MODE_UP);
            printf("⬆️ 暂停结束，反转上提（目标800mm）\r\n");
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
            printf("⏹️ 内管到位 %dmm，停止\r\n", dist);
        }
        
        // 标记为到位状态
        if(is_in_place == 0)
        {
            is_in_place = 1;
            printf("🔍 开始卡芯/满芯监测...\r\n");
            
            // 内管到位后暂停（用 PAUSE_TIME_IN_PLACE）
            uint32_t pause_start = HAL_GetTick();
            while(HAL_GetTick() - pause_start < PAUSE_TIME_IN_PLACE)
            {
                // 等待期间也要产生脉冲？不需要，已经停止了
                HAL_Delay(10);
            }
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
            printf("⬇️ 距离=%dmm > 400mm，正转下钻\r\n", dist);
        }
    }
}

// ==================== WiFi模块相关函数 ====================

/**
  * @brief WiFi模块引脚初始化
  */
void WiFi_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOB时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 配置PB9为复位输出（推挽）
    GPIO_InitStruct.Pin = WIFI_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(WIFI_RST_PORT, &GPIO_InitStruct);
    
    // PB8配置为悬空输入（不控制）
    GPIO_InitStruct.Pin = WIFI_IO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(WIFI_RST_PORT, &GPIO_InitStruct);
    
    printf("WiFi引脚初始化完成\r\n");
}

/**
  * @brief 复位WiFi模块
  */
void WiFi_Reset(void)
{
    HAL_GPIO_WritePin(WIFI_RST_PORT, WIFI_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(WIFI_RST_PORT, WIFI_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(2000);
    printf("WiFi模块已复位\r\n");
}

/**
  * @brief 发送AT指令到WiFi模块并等待响应
  * @param cmd: AT指令字符串（不包含\r\n）
  * @param expected: 期望的响应
  * @param timeout: 超时时间(ms)
  * @retval 1-成功 0-失败
  */
uint8_t WiFi_SendAT(const char* cmd, const char* expected, uint32_t timeout)
{
    uint8_t buffer[128];
    uint32_t start = HAL_GetTick();
    uint16_t index = 0;
    
    // 发送指令（加\r\n）
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)cmd, strlen(cmd), 100);
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)"\r\n", 2, 100);
    
    // 等待响应
    while(HAL_GetTick() - start < timeout)
    {
        if(HAL_UART_Receive(WIFI_UART_HANDLE, &buffer[index], 1, 50) == HAL_OK)
        {
            if(index < sizeof(buffer)-1)
            {
                index++;
                buffer[index] = 0;
                // 检查是否收到期望的响应
                if(strstr((char*)buffer, expected) != NULL)
                {
                    printf("AT响应: %s\r\n", buffer);
                    return 1;
                }
            }
        }
    }
    printf("AT指令超时: %s\r\n", cmd);
    return 0;
}

/**
  * @brief 配置WiFi模块连接网络
  */
void WiFi_Connect(void)
{
    char cmd[128];
    
    printf("\r\n========== WiFi配置开始 ==========\r\n");
    
    // 1. 先退出透传模式（如果之前是透传模式）
    // 发送+++退出透传（注意：+++前后不需要回车换行）
    HAL_UART_Transmit(WIFI_UART_HANDLE, (uint8_t*)"+++", 3, 100);
    HAL_Delay(500);
    
    // 2. 测试AT
    printf("测试AT指令...\r\n");
    if(!WiFi_SendAT("AT", "OK", 2000))
    {
        printf("WiFi模块无响应，尝试复位...\r\n");
        WiFi_Reset();
        if(!WiFi_SendAT("AT", "OK", 2000))
        {
            printf("WiFi模块连接失败，请检查接线\r\n");
            wifi_connected = 0;
            return;
        }
    }
    printf("✓ AT指令正常\r\n");
    HAL_Delay(500);
    
    // 3. 设置STA模式
    printf("设置STA模式...\r\n");
    WiFi_SendAT("AT+CWMODE=1", "OK", 2000);
    HAL_Delay(500);
    
    // 4. 连接WiFi
    printf("连接WiFi: %s...\r\n", WIFI_SSID);
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASSWORD);
    if(WiFi_SendAT(cmd, "OK", 10000))
    {
        printf("✓ WiFi连接成功\r\n");
    }
    else
    {
        printf("✗ WiFi连接失败，请检查SSID和密码\r\n");
        wifi_connected = 0;
        return;
    }
    HAL_Delay(2000);
    
    // 5. 查询IP地址（可选）
    WiFi_SendAT("AT+CIFSR", "OK", 2000);
    HAL_Delay(500);
    
    // 6. 连接TCP服务器
    printf("连接TCP服务器 %s:%s...\r\n", SERVER_IP, SERVER_PORT);
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", SERVER_IP, SERVER_PORT);
    if(WiFi_SendAT(cmd, "CONNECT", 5000))
    {
        printf("✓ TCP连接成功\r\n");
    }
    else
    {
        printf("✗ TCP连接失败，请检查服务器IP和端口\r\n");
        wifi_connected = 0;
        return;
    }
    HAL_Delay(500);
    
    // 7. 开启透传模式
    printf("开启透传模式...\r\n");
    WiFi_SendAT("AT+CIPMODE=1", "OK", 2000);
    HAL_Delay(500);
    
    // 8. 进入透传
    printf("进入透传模式...\r\n");
    WiFi_SendAT("AT+CIPSEND", ">", 2000);
    printf("✓ WiFi透传模式已开启，数据将无线传输\r\n");
    printf("====================================\r\n\r\n");
    
    // 标记WiFi已连接
    wifi_connected = 1;
}

/**
  * @brief 通过WiFi发送数据（透传模式）
  * @param data: 要发送的数据
  * @param len: 数据长度
  */
void WiFi_SendData(uint8_t* data, uint16_t len)
{
    if(wifi_connected)
    {
        HAL_UART_Transmit(WIFI_UART_HANDLE, data, len, 100);
    }
}

/**
  * @brief 发送状态数据（格式化，用于WiFi传输）
  */
void WiFi_SendStatus(uint16_t dist, uint8_t mode, uint8_t in_place, uint8_t reversing, 
                      uint16_t avg, uint16_t diff)
{
    char buffer[256];
    char *mode_str = "";
    char *status_str = "";
    
    // 模式英文
    switch(mode)
    {
        case MODE_IDLE:   mode_str = "IDLE"; break;
        case MODE_DOWN:   mode_str = "DOWN"; break;
        case MODE_UP:     mode_str = "UP"; break;
        case MODE_PAUSE:  mode_str = "PAUSE"; break;
        default:          mode_str = "UNKNOWN"; break;
    }
    
    // 状态描述英文
    if(in_place == 1)
    {
        status_str = "IN_PLACE";      // 内管到位
    }
    else if(reversing == 1)
    {
        status_str = "REVERSING";     // 反转上提
    }
    else if(mode == MODE_DOWN)
    {
        status_str = "DRILLING";      // 下钻中
    }
    else if(mode == MODE_IDLE)
    {
        status_str = "WAITING";       // 等待
    }
    else if(mode == MODE_PAUSE)
    {
        status_str = "PAUSING";       // 暂停中
    }
    else
    {
        status_str = "NORMAL";
    }
    
    // 格式：时间 | 距离 | 模式 | 状态 | 到位标记 | 反转标记 | 平均值 | 差值
    sprintf(buffer, "[%d] DIST=%dmm MODE=%s STATUS=%s PLACE=%d REV=%d AVG=%dmm DIFF=%d\r\n",
            (int)(HAL_GetTick()/1000), 
            dist, 
            mode_str, 
            status_str,
            in_place, 
            reversing, 
            avg, 
            diff);
    
    WiFi_SendData((uint8_t*)buffer, strlen(buffer));
}

/**
  * @brief 检查WiFi连接状态（自动重连）
  */
uint8_t WiFi_CheckConnection(void)
{
    static uint32_t last_check = 0;
    static uint8_t reconnect_attempt = 0;
    
    // 每30秒检查一次
    if(HAL_GetTick() - last_check >= 30000)
    {
        last_check = HAL_GetTick();
        
        if(wifi_connected == 1)
        {
            // 已连接，不需要重复检查
            // 透传模式下不要发送AT指令，否则会退出透传
            return 1;
        }
        else
        {
            // 未连接，尝试重连（最多3次）
            if(reconnect_attempt < 3)
            {
                reconnect_attempt++;
                printf("WiFi断开，第%d次尝试重连...\r\n", reconnect_attempt);
                WiFi_Connect();
            }
            else
            {
                reconnect_attempt = 0;
                printf("WiFi重连失败，请检查网络\r\n");
            }
            return 0;
        }
    }
    return wifi_connected;
}

/**
  * @brief 串口打印状态信息（同时通过WiFi发送）
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
        uint32_t sum_d = 0;
        uint16_t avg_d = 0;
        
        for(int i = 0; i < DIST_ARRAY_SIZE; i++)
        {
            if(dist_array[i] > max_d) max_d = dist_array[i];
            if(dist_array[i] < min_d) min_d = dist_array[i];
            sum_d += dist_array[i];
        }
        
        if(dist_array_full)
            avg_d = sum_d / DIST_ARRAY_SIZE;
        else
            avg_d = 0;
        
        char *mode_str = "";
        switch(drill_mode)
        {
            case MODE_IDLE:   mode_str = "空闲"; break;
            case MODE_DOWN:   mode_str = "下钻"; break;
            case MODE_UP:     mode_str = "上提"; break;
            case MODE_PAUSE:  mode_str = "暂停"; break;
        }
        
        // 本地串口打印（用于调试）
        printf("[%03d] dist=%4dmm | %s |到位=%d|rev=%d| avg=%3dmm| diff=%2dmm\r\n", 
               (HAL_GetTick()/1000)%1000, dist, mode_str, is_in_place, 
               is_reversing, avg_d, max_d - min_d);
        
        // 通过WiFi发送（简洁格式）
        WiFi_SendStatus(dist, drill_mode, is_in_place, is_reversing, avg_d, max_d - min_d);
    }
}
/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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