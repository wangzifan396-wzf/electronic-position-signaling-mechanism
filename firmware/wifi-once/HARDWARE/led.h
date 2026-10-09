#ifndef __LED_H
#define __LED_H


#include "main.h"
#include "usart.h"
#include "gpio.h"


//LED 端口定义

#define LED_OFF  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
#define LED_ON  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
#define LED HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);


void LED_Init(void);  //初始化
void Led_Flash(uint16_t time);


#endif
