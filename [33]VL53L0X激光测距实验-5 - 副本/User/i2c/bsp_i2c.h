#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include "main.h"

extern I2C_HandleTypeDef hi2c2;  // 声明I2C2的句柄变量hi2c2，定义在别的文件中，方便多个文件访问I2C2接口


/************ 函数声明 ************/
void MX_I2C2_Init(void);  



#endif /* __BSP_I2C_H */
