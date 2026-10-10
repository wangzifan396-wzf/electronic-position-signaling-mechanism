/**
  ******************************************************************************
  * @file       bsp_i2c.c
  * @author     embedfire
  * @version     V1.0
  * @date        2025
  * @brief      API底层函数接口（STM32）
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
  

#include "vl53l0x_i2c_rw.h"


/**
 * @brief  向 VL53L0X 写入多个字节的数据
 * @param  address: 器件地址（7位地址左移1位，即符合 HAL 要求的写地址）
 * @param  index: 起始寄存器地址
 * @param  pdata: 要写入的数据指针
 * @param  count: 数据长度，最大 65535
 * @retval 状态：STATUS_OK 或 STATUS_FAIL
 */
uint8_t VL53L0X_write_multi(uint8_t address,
                            uint8_t index,
                            uint8_t *pdata,
                            uint16_t count)
{
    uint8_t status = STATUS_OK;

    // 申请一块新内存，第一字节放 index，后面跟实际数据
    uint8_t *buffer = malloc(count + 1);
    if (buffer == NULL) return STATUS_FAIL;  // 分配失败
    buffer[0] = index;                       // 设置起始寄存器
    memcpy(&buffer[1], pdata, count);       // 拷贝数据

    // 使用 HAL I2C 接口发送数据
    if (HAL_I2C_Master_Transmit(&hi2c2, address, buffer, count + 1, VL53L0X_I2C_TIMEOUT) != HAL_OK)
    {
        status = STATUS_FAIL; // 传输失败
    }

    free(buffer); // 释放临时分配的内存
    return status;
}

/**
 * @brief  从 VL53L0X 读取多个字节的数据
 * @param  address: 器件地址
 * @param  index: 起始寄存器地址
 * @param  pdata: 接收数据的缓冲区指针
 * @param  count: 要读取的字节数，最大 65535
 * @retval 状态：STATUS_OK 或 STATUS_FAIL
 */
uint8_t VL53L0X_read_multi(uint8_t address,
                           uint8_t index,
                           uint8_t *pdata,
                           uint16_t count)
{
    uint8_t status = STATUS_OK;

    // 首先写入要读取的寄存器地址
    if (HAL_I2C_Master_Transmit(&hi2c2, address, &index, 1, VL53L0X_I2C_TIMEOUT) != HAL_OK)
    {
        status = STATUS_FAIL;
    }
    // 然后读取数据
    else if (HAL_I2C_Master_Receive(&hi2c2, address, pdata, count, VL53L0X_I2C_TIMEOUT) != HAL_OK)
    {
        status = STATUS_FAIL;
    }

    return status;
}

/**
 * @brief  写入 1 字节数据
 * @param  address: 器件地址
 * @param  index: 要写入的寄存器地址
 * @param  data: 要写入的数据
 * @retval 状态：STATUS_OK 或 STATUS_FAIL
 */
uint8_t VL53L0X_write_byte(uint8_t address,
                           uint8_t index,
                           uint8_t data)
{
    return VL53L0X_write_multi(address, index, &data, 1);
}

/**
 * @brief  写入 2 字节数据（16 位）
 * @param  data: 要写入的数据（高位先传）
 */
uint8_t VL53L0X_write_word(uint8_t address,
                           uint8_t index,
                           uint16_t data)
{
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(data >> 8);      // 高字节
    buffer[1] = (uint8_t)(data & 0xFF);    // 低字节

    return VL53L0X_write_multi(address, index, buffer, 2);
}

/**
 * @brief  写入 4 字节数据（32 位）
 */
uint8_t VL53L0X_write_dword(uint8_t address,
                            uint8_t index,
                            uint32_t data)
{
    uint8_t buffer[4];

    buffer[0] = (uint8_t)(data >> 24);           // 最高字节
    buffer[1] = (uint8_t)((data >> 16) & 0xFF);  // 次高字节
    buffer[2] = (uint8_t)((data >> 8) & 0xFF);   // 次低字节
    buffer[3] = (uint8_t)(data & 0xFF);          // 最低字节

    return VL53L0X_write_multi(address, index, buffer, 4);
}

/**
 * @brief  读取 1 字节数据
 * @param  pdata: 接收数据的指针
 */
uint8_t VL53L0X_read_byte(uint8_t address,
                          uint8_t index,
                          uint8_t *pdata)
{
    return VL53L0X_read_multi(address, index, pdata, 1);
}

/**
 * @brief  读取 2 字节数据（16 位）
 */
uint8_t VL53L0X_read_word(uint8_t address,
                          uint8_t index,
                          uint16_t *pdata)
{
    uint8_t buffer[2];
    uint8_t status = VL53L0X_read_multi(address, index, buffer, 2);

    if (status == STATUS_OK)
    {
        // 合并高低字节
        *pdata = ((uint16_t)buffer[0] << 8) | buffer[1];
    }

    return status;
}

/**
 * @brief  读取 4 字节数据（32 位）
 */
uint8_t VL53L0X_read_dword(uint8_t address,
                           uint8_t index,
                           uint32_t *pdata)
{
    uint8_t buffer[4];
    uint8_t status = VL53L0X_read_multi(address, index, buffer, 4);

    if (status == STATUS_OK)
    {
        *pdata = ((uint32_t)buffer[0] << 24) |
                 ((uint32_t)buffer[1] << 16) |
                 ((uint32_t)buffer[2] << 8) |
                 ((uint32_t)buffer[3]);
    }

    return status;
}

/******************************** END OF FILE *********************************/


