/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   所有使用的GPIO引脚配置文件
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
#include "gpio.h"

/* USER CODE BEGIN 0 */
/* 用户代码区域 0 - 可以添加私有变量和函数声明 */
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* 配置GPIO                                                                   */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */
/* 用户代码区域 1 - 可以添加初始化前的代码 */
/* USER CODE END 1 */

/**
  * @brief 配置所有GPIO引脚
  * @param 无
  * @retval 无
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 使能GPIO端口时钟 */
  __HAL_RCC_GPIOC_CLK_ENABLE();   // 使能GPIOC时钟
  __HAL_RCC_GPIOD_CLK_ENABLE();   // 使能GPIOD时钟
  __HAL_RCC_GPIOA_CLK_ENABLE();   // 使能GPIOA时钟

  /* 配置所有输出引脚的初始电平 */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);     // LED引脚初始高电平（灯灭）
  
  /* 电机控制引脚初始配置：
     - PA5 (EN)：使能引脚，低电平有效
     - PA6 (STP)：脉冲引脚，初始低电平
     - PA7 (DIR)：方向引脚，初始低电平
  */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /* 配置LED引脚 */
  GPIO_InitStruct.Pin = LED_Pin;                                // LED引脚
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;                   // 推挽输出模式
  GPIO_InitStruct.Pull = GPIO_NOPULL;                           // 无上下拉电阻
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;               // 中等速度
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* 配置电机控制引脚 - PA5(EN使能), PA6(STEP脉冲), PA7(DIR方向) */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;       // PA5, PA6, PA7引脚
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;                   // 推挽输出模式
  GPIO_InitStruct.Pull = GPIO_NOPULL;                           // 无上下拉电阻
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;                 // 高速模式（为了产生精确脉冲）
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);                       // 初始化GPIOA的这些引脚

  /* 使能步进电机驱动器（EN引脚低电平有效） */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);         // EN = 0，使能电机
}

/* USER CODE BEGIN 2 */
/* 用户代码区域 2 - 可以添加初始化后的代码 */
/* USER CODE END 2 */