/**
  ******************************************************************************
  * @file       app_vl53l0x.c
  * @author     embedfire
  * @version     V1.0
  * @date        2025
  * @brief      激光测距模块 应用层功能接口
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

#include "vl53l0x/app_vl53l0x.h"
#include "vl53l0x/bsp_i2c_vl53l0x.h"

/**
 * @brief  单次测量并读取一次距离数据
 * @param  dev: 设备I2C参数结构体
 * @param  pdata: 用于保存测距结果的结构体指针
 * @retval 状态信息
 */
/**
 * @brief  单次测量并读取一次距离数据
 * @param  dev: 设备I2C参数结构体
 * @param  pdata: 用于保存测距结果的结构体指针(可为NULL)
 * @retval 状态信息
 */
VL53L0X_Error VL53L0X_Task(VL53L0X_Dev_t *dev, VL53L0X_RangingMeasurementData_t *pdata)
{
    VL53L0X_Error status;
    char buf[VL53L0X_MAX_STRING_LENGTH] = {0};
    uint32_t timeout = 0;
    VL53L0X_RangingMeasurementData_t data;  // 局部变量存储数据
    
    Vl53l0x_int_flag = 0;  // 清标志，准备等待新数据

    // 启动单次测量
    status = VL53L0X_StartMeasurement(dev);
    if (status != VL53L0X_ERROR_NONE)
        return status;

    // 等待中断标志位，表示测量完成
    while (Vl53l0x_int_flag == 0)
    {
        if (timeout++ > 1000)  // 超时保护，避免死循环
            return VL53L0X_ERROR_TIME_OUT;
        HAL_Delay(5);
    }

    // 读取测距数据
    status = VL53L0X_GetRangingMeasurementData(dev, &data);
    if (status != VL53L0X_ERROR_NONE)
        return status;

    // 如果传入了指针，将数据返回
    if (pdata != NULL)
    {
        *pdata = data;
    }

    // 获取状态字符串
    VL53L0X_GetRangeStatusString(data.RangeStatus, buf);

    // 清除中断标志，准备下一次测量
    status = VL53L0X_ClearInterruptMask(dev, 0);
    if (status != VL53L0X_ERROR_NONE)
        return status;

    // 打印测量结果
    printf("距离: %4d mm, 状态: %s\r\n", data.RangeMilliMeter, buf);

    return VL53L0X_ERROR_NONE;
}