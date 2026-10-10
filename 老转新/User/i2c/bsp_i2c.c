/**
  ******************************************************************************
  * @file       bsp_i2c.c
  * @author     embedfire
  * @version     V1.0
  * @date        2025
  * @brief      硬件IIC函数接口
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
  
#include "i2c/bsp_i2c.h"


I2C_HandleTypeDef hi2c2;  // 定义I2C句柄变量

/**
  * @brief  初始化 I2C2
  * @note   I2C2 的引脚配置：SDA -> PB10，SCL -> PB11
  */
void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;                      // 选择使用的I2C外设为I2C2
    hi2c2.Init.ClockSpeed = 400000;            // 设置I2C时钟频率为400kHz，属于高速模式
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;   // 设置时钟占空比为2（标准模式常用）
    hi2c2.Init.OwnAddress1 = 0;                // 主机模式下自定义地址，0表示不使用
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;  // 使用7位地址模式
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; // 禁用双地址模式
    hi2c2.Init.OwnAddress2 = 0;                // 第二地址无效
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; // 禁用通用呼叫模式
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;     // 允许时钟拉伸

    // 调用HAL库函数初始化I2C2，若失败则调用错误处理函数
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  I2C外设相关GPIO和时钟初始化，HAL库自动调用
  * @param  i2cHandle 指向I2C句柄的指针
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // 判断是否为I2C2外设
  if(i2cHandle->Instance==I2C2)
  {
    /* 开启GPIOB端口时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /** 配置I2C2的SCL和SDA引脚
      PB10 -> I2C2_SCL
      PB11 -> I2C2_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;       // 选择PB10和PB11
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;              // 复用开漏模式（I2C必选）
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;        // 设置引脚高速模式
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);              // 初始化GPIOB对应引脚

    /* 使能I2C2时钟 */
    __HAL_RCC_I2C2_CLK_ENABLE();
  }
}

/**
  * @brief  I2C外设GPIO和时钟反初始化，HAL库自动调用
  * @param  i2cHandle 指向I2C句柄的指针
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  // 判断是否为I2C2外设
  if(i2cHandle->Instance==I2C2)
  {
    /* 关闭I2C2时钟 */
    __HAL_RCC_I2C2_CLK_DISABLE();

    /** 反初始化I2C2对应的GPIO引脚 */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);   // 释放PB10引脚
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);   // 释放PB11引脚
  }
}


/******************************** END OF FILE *********************************/
