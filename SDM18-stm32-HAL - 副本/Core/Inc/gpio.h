/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   包含gpio.c文件中所有函数的原型声明
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
/* 用户包含文件区域 */
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
/* 私有定义区域 - 引脚定义 */

// LED引脚定义（根据您的实际电路修改）
#define LED_Pin                         GPIO_PIN_13      // LED连接的引脚号
#define LED_GPIO_Port                   GPIOC            // LED连接的端口

// 步进电机控制引脚定义（新增）
#define STEPPER_EN_PIN                   GPIO_PIN_5       // PA5 - 使能引脚(EN)
#define STEPPER_STEP_PIN                 GPIO_PIN_6       // PA6 - 脉冲引脚(STP)
#define STEPPER_DIR_PIN                   GPIO_PIN_7       // PA7 - 方向引脚(DIR)
#define STEPPER_GPIO_PORT                GPIOA            // 步进电机使用的GPIO端口

// 为了方便使用，也可以定义组合
#define STEPPER_ALL_PINS                 (GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)  // 所有步进电机引脚

// 方向定义（可选，方便记忆）
#define STEPPER_DIR_CW                    GPIO_PIN_SET     // 顺时针方向
#define STEPPER_DIR_CCW                   GPIO_PIN_RESET   // 逆时针方向

// 使能电平定义（根据您的驱动器调整）
#define STEPPER_ENABLE_LEVEL              GPIO_PIN_RESET   // 使能电平（低电平有效）
#define STEPPER_DISABLE_LEVEL              GPIO_PIN_SET     // 禁用电平

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
/* 用户原型声明区域 */

// 这里可以添加一些便捷的宏定义或内联函数
// 例如：快速设置方向
#define STEPPER_SetDir_CW()     HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN, GPIO_PIN_SET)
#define STEPPER_SetDir_CCW()    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN, GPIO_PIN_RESET)

// 快速使能/禁用电机
#define STEPPER_Enable()        HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, STEPPER_ENABLE_LEVEL)
#define STEPPER_Disable()       HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, STEPPER_DISABLE_LEVEL)

// 发送单个脉冲
#define STEPPER_Pulse_High()     HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_SET)
#define STEPPER_Pulse_Low()      HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_RESET)

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */