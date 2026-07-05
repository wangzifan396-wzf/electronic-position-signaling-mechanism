#ifndef __VL53L0X_IIC_H
#define __VL53L0X_IIC_H

#include "main.h"
#include "i2c/bsp_i2c.h"

#define VL53L0X_I2C_TIMEOUT 100   // I2C通信超时时间（单位：ms）

// 状态码定义
#define STATUS_OK    0            // 操作成功
#define STATUS_FAIL  1            // 操作失败


/************ 函数声明 ************/
uint8_t VL53L0X_write_byte(uint8_t device_address, uint8_t index, uint8_t data);
uint8_t VL53L0X_write_word(uint8_t device_address, uint8_t index, uint16_t data);
uint8_t VL53L0X_write_dword(uint8_t device_address, uint8_t index, uint32_t data);

uint8_t VL53L0X_read_byte(uint8_t device_address, uint8_t index, uint8_t *pdata);
uint8_t VL53L0X_read_word(uint8_t device_address, uint8_t index, uint16_t *pdata);
uint8_t VL53L0X_read_dword(uint8_t device_address, uint8_t index, uint32_t *pdata);

uint8_t VL53L0X_write_multi(uint8_t device_address, uint8_t index, uint8_t *pdata, uint16_t count);
uint8_t VL53L0X_read_multi(uint8_t device_address, uint8_t index, uint8_t *pdata, uint16_t count);


#endif 

