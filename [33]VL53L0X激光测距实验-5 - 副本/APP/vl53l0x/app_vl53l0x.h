#ifndef __APP_VL53L0X_H
#define __APP_VL53L0X_H

#include "main.h"
#include "vl53l0x_api.h"

/************ 函数声明 ************/
VL53L0X_Error VL53L0X_Task(VL53L0X_Dev_t *dev, VL53L0X_RangingMeasurementData_t *pdata);

#endif /* __APP_VL53L0X_H */
