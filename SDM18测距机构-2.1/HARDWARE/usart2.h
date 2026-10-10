#ifndef __USART2_H
#define __USART2_H

#include "main.h"
#include "bsp.h"
#include "usart.h"
#include "gpio.h"

void USART2_UART_Init(void);
void USART2_DataByte(uint8_t data_byte);
void USART2_DataString(uint8_t *data_str, uint16_t datasize);

#endif


