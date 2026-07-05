#include "usart2.h"
#include "SMD18.h"

UART_HandleTypeDef huart2;  // 定义USART2句柄
static uint8_t rx_buffer;    // 接收缓冲区

/**
  * @brief USART2 底层初始化
  * @param huart UART句柄
  * @retval 无
  */
void HAL_UART_MspInit2(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (huart->Instance == USART2)
    {
        // 使能USART2时钟
        __HAL_RCC_USART2_CLK_ENABLE();
        // 使能GPIOA时钟
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        // 配置TX (PA2)
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // 配置RX (PA3)
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // 配置中断优先级
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

/**
  * @brief 初始化 USART2
  * @param 无
  * @retval 无
  */
void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;      // 波特率115200
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    
    // 手动调用底层初始化
    HAL_UART_MspInit2(&huart2);
    
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 使能接收中断
    HAL_UART_Receive_IT(&huart2, &rx_buffer, 1);
}

/**
  * @brief USART2 中断服务函数
  * @param 无
  * @retval 无
  */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/**
  * @brief UART接收完成回调函数
  * @param huart UART句柄
  * @retval 无
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    static uint8_t rx_data;
    static uint32_t rx_count = 0;
    
    if (huart->Instance == USART2)
    {
        rx_data = (uint8_t)(huart->Instance->DR & 0xFF);  // 读取数据
        
        // 打印每个收到的字节（调试用）
        rx_count++;
        printf("RX[%lu]: 0x%02X\r\n", rx_count, rx_data);
        
        SDM18_Decode(rx_data);  // 解析数据
        HAL_UART_Receive_IT(&huart2, &rx_buffer, 1);  // 重新开启接收
    }
}

/**
  * @brief 发送一个字节
  * @param data 要发送的数据
  * @retval 无
  */
void USART2_DataByte(uint8_t data)
{
    HAL_UART_Transmit(&huart2, &data, 1, HAL_MAX_DELAY);
}

/**
  * @brief 发送字符串
  * @param data_str 数据指针
  * @param datasize 数据长度
  * @retval 无
  */
void USART2_DataString(uint8_t *data_str, uint16_t datasize)
{
    HAL_UART_Transmit(&huart2, data_str, datasize, HAL_MAX_DELAY);
}