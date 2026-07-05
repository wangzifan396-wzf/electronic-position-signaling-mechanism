#ifndef __BSP_I2C_VL53L0X_H
#define __BSP_I2C_VL53L0X_H

#include "main.h"
#include "vl53l0x_api.h"

#define VL53L0X_XSH_Pin           GPIO_PIN_8       // XSHUT 控制引脚
#define VL53L0X_XSH_GPIO_Port     GPIOB            // XSHUT 引脚所在端口
#define VL53L0X_INT_Pin           GPIO_PIN_9       // 中断引脚
#define VL53L0X_INT_GPIO_Port     GPIOB            // 中断引脚所在端口

//VL53L0X传感器上电默认IIC地址为0X52
#define VL53L0X_Addr 0x52

//vl53l0x模式配置参数集
typedef __packed struct
{
	FixPoint1616_t signalLimit;    //Signal极限数值 
	FixPoint1616_t sigmaLimit;     //Sigmal极限数值
	uint32_t timingBudget;         //采样时间周期
	uint8_t preRangeVcselPeriod ;  //VCSEL脉冲周期
	uint8_t finalRangeVcselPeriod ;//VCSEL脉冲周期范围
}mode_data;


/************ 变量声明 ************/

extern uint8_t Vl53l0x_int_flag;   /* VL53L0X 中断标志 */
extern mode_data Mode_data[];			 /* 模式数据数组 */
extern VL53L0X_Dev_t vl53l0x_dev;  /* 设备 I2C 数据参数 */


/************ 函数声明 ************/
void VL_GPIO_Init(void);
void VL53L0X_PrintError(VL53L0X_Error status);
VL53L0X_Error VL53L0X_AddrSet(VL53L0X_Dev_t *dev, uint8_t newAddr);
void VL53L0X_Reset(VL53L0X_Dev_t *dev);
VL53L0X_Error VL53L0X_SetMode_Simple(VL53L0X_Dev_t *dev, uint8_t mode);
VL53L0X_Error VL53L0X_init(VL53L0X_Dev_t *dev, uint8_t mode);


#endif /* __BSP_I2C_VL53L0X_H */
