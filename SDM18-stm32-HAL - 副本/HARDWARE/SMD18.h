#ifndef SMD18_H
#define SMD18_H

#include "main.h"

#define CRC16_POLYNOMIAL 0x8005
#define bool _Bool
#define true 1
#define false 0

typedef enum sdm18Baud
{
  B9600 ,
	B14400,
	B19200,
	B38400,
	B43000,
	B57600,
	B76800,
	B115200 ,
	B128000,
	B230400,
	B256000,
	B460800,
	B921600 
} SDM18_Baud_t;

/* 外部变量声明 */
extern uint8_t newlines;        // 新数据标志
extern uint8_t Rx_buffer_ok[30]; // 数据缓冲区
extern uint16_t CRC_buff;        // CRC缓冲区

/* 原有函数声明 */
void SMD18_init(SDM18_Baud_t bound);
void stop_scan(void);
void start_scan(void);
void SMD18_setbaudrate(SDM18_Baud_t i);
void print_message(void);
void SDM18_Decode(uint8_t RxData);
uint16_t calculate_crc16(uint8_t *puchMsg, int usDataLen);

/* 新增函数声明 - 兼容main.c调用 */
void SMD18_Init(void);                    // 初始化（无参数版本）
float SMD18_GetDistance(void);             // 获取距离（厘米）
uint16_t SMD18_GetDistance_MM(void);       // 获取距离（毫米）
uint16_t SMD18_GetStrength(void);          // 获取信号强度

#endif