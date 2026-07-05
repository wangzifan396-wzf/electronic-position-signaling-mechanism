#ifndef __BSP_DWT_H
#define __BSP_DWT_H

#include "stm32f1xx_hal.h"

/* 补充DWT相关寄存器定义（兼容原有代码） */
#define DEMCR                  *((volatile uint32_t *)0xE000EDFC)
#define DEMCR_TRCENA           (1 << 24)
#define DWT_CTRL               *((volatile uint32_t *)0xE0001000)
#define DWT_CTRL_CYCCNTENA     (1 << 0)
#define DWT_CYCCNT             *((volatile uint32_t *)0xE0001004)

/* 原有函数声明 */
void DWT_Init(void);
uint32_t DWT_GetTick(void);
uint32_t DWT_TickToMicrosecond(uint32_t tick,uint32_t frequency);
void DWT_DelayUs(uint32_t time);
void DWT_DelayMs(uint32_t time);
void DWT_DelayS(uint32_t time);

/* 新增DWT_GetTickUs函数声明 */
uint32_t DWT_GetTickUs(void);

#endif /* __BSP_DWT_H__ */
// 注意：文件最后按Enter加空行，解决#1-D警告