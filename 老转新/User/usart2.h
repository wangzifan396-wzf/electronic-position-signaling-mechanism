#ifndef __USART2_H
#define __USART2_H

#include "main.h"

extern UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void);
void USART2_DataByte(uint8_t data);
void USART2_DataString(uint8_t *data_str, uint16_t datasize);

#endif