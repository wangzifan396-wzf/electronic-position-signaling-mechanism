/**
  ******************************************************************************
  * @file       bsp_i2c_vl53l0x.c
  * @author     embedfire
  * @version     V1.0
  * @date        2025
  * @brief      VL53L0X激光测距驱动接口
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
  

#include "vl53l0x/bsp_i2c_vl53l0x.h"
#include "i2c/bsp_i2c.h"
#include "led/bsp_led.h" 
#include "usart/bsp_usart.h"

uint8_t Vl53l0x_int_flag = 0;                 // VL53L0X 中断标志
VL53L0X_Dev_t vl53l0x_dev;                    // 设备 I2C 数据参数

/**
  * @brief  VL53L0X 所需 GPIO 初始化
  * @note   包括 XSHUT 引脚和中断引脚配置
  * @retval 无
  */
void VL_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 使能 GPIOB 时钟 */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* 设置 XSHUT 引脚初始电平为低 */
  HAL_GPIO_WritePin(VL53L0X_XSH_GPIO_Port, VL53L0X_XSH_Pin, GPIO_PIN_SET);

  /* 配置 XSHUT 引脚为推挽输出 */
  GPIO_InitStruct.Pin = VL53L0X_XSH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(VL53L0X_XSH_GPIO_Port, &GPIO_InitStruct);

  /* 配置 INT 引脚为下降沿中断输入 */
  GPIO_InitStruct.Pin = VL53L0X_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(VL53L0X_INT_GPIO_Port, &GPIO_InitStruct);

  /* 使能 EXTI 中断 */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
  * @brief  GPIO EXTI 中断回调函数（由 HAL 自动调用）
  * @param  GPIO_Pin: 产生中断的引脚编号
  * @retval 无
  * @note   此函数在 HAL_GPIO_EXTI_IRQHandler() 中被调用，用于处理中断事件。
  *         此处用于检测 VL53L0X 的中断引脚（PB9），并设置数据读取标志位。
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == VL53L0X_INT_Pin)
    {
        // 设置中断标志位，主循环中读取 VL53L0X 数据时使用
        Vl53l0x_int_flag = 1;

        // 翻转红色 LED，作为中断触发指示（调试用）
        //R_LED_TOGGLE();
    }
}


/**
 * @brief  打印VL53L0X错误信息
 * @param  status：错误代码，参考VL53L0X_Error定义
 * @retval 无
 */
void VL53L0X_PrintError(VL53L0X_Error status)
{
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(status, buf);               // 获取错误字符串
    printf("API 状态: %d : %s\r\n", status, buf);       	// 打印错误码与信息
}


/**
 * @brief  修改VL53L0X的I2C地址
 * @param  dev     VL53L0X设备结构体
 * @param  newAddr 新的I2C地址
 * @retval 错误状态，类型为VL53L0X_Error
 */
VL53L0X_Error VL53L0X_AddrSet(VL53L0X_Dev_t *dev, uint8_t newAddr)
{
    uint16_t Id;
    uint8_t FinalAddress;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t sta = 0x00;

    FinalAddress = newAddr;

    /*新设备I2C地址与旧地址一致,直接退出*/
    if (FinalAddress == dev->I2cDevAddr)
        return VL53L0X_ERROR_NONE;

    /*在进行第一个寄存器访问之前设置I2C标准模式(400Khz)*/
    Status = VL53L0X_WrByte(dev, 0x88, 0x00);
    if (Status != VL53L0X_ERROR_NONE)
    {
        /*设置I2C标准模式出错*/
        sta = 0x01;
        goto error;
    }
    /*尝试使用默认的0x52地址读取一个寄存器*/
    Status = VL53L0X_RdWord(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &Id);
    if (Status != VL53L0X_ERROR_NONE)
    {
        /*读取寄存器出错*/
        sta = 0x02;
        goto error;
    }
    if (Id == 0xEEAA)
    {
        /*设置设备新的I2C地址*/
        Status = VL53L0X_SetDeviceAddress(dev, FinalAddress);
        if (Status != VL53L0X_ERROR_NONE)
        {
            /*设置I2C地址出错*/
            sta = 0x03;
            goto error;
        }
        /*修改参数结构体的I2C地址*/
        dev->I2cDevAddr = FinalAddress;
        /*检查新的I2C地址读写是否正常*/
        Status = VL53L0X_RdWord(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &Id);
        if (Status != VL53L0X_ERROR_NONE)
        {
            /*新I2C地址读写出错*/
            sta = 0x04;
            goto error;
        }
    }
error:
    if (Status != VL53L0X_ERROR_NONE)
    {
        /*打印错误信息*/
        VL53L0X_PrintError(Status);
    }
    if (sta != 0)
        printf("sta:0x%x\r\n", sta);
    return Status;
}

/**
 * @brief  VL53L0X 复位函数
 * @param  dev：VL53L0X 设备结构体指针
 * @retval 无
 */
void VL53L0X_Reset(VL53L0X_Dev_t *dev)
{
    uint8_t old_addr;

    /* 保存设备上电前设置的原始 I2C 地址 */
    old_addr = dev->I2cDevAddr;

    /* 通过 XSH 引脚关闭模块 */
    HAL_GPIO_WritePin(VL53L0X_XSH_GPIO_Port, VL53L0X_XSH_Pin, GPIO_PIN_RESET);
    HAL_Delay(30);

    /* 再次拉高 XSH，上电模块，进入默认地址模式（0x52） */
    HAL_GPIO_WritePin(VL53L0X_XSH_GPIO_Port, VL53L0X_XSH_Pin, GPIO_PIN_SET);
    HAL_Delay(30);

    /* 设置为上电默认地址 0x52（8位地址）以便能与模块通信 */
    dev->I2cDevAddr = VL53L0X_Addr;

    /* 初始化模块内部状态 */
    VL53L0X_DataInit(dev);

    /* 恢复用户原来设置的地址 */
    VL53L0X_AddrSet(dev, old_addr);
}

/* 模式配置表（用于 VL53L0X 传感器不同测距模式）
 * 每组参数用于配置不同的测量模式，影响测量距离、精度和速度
 * 模式说明（数组下标）：
 * 0 - 默认模式
 * 1 - 高精度模式（预算时间更长，精度更高）
 * 2 - 长距离模式（支持更远距离，但精度较低）
 * 3 - 高速模式（测距速度更快，精度稍低）
 */
mode_data Mode_data[] =
{
    {
        (FixPoint1616_t)(0.25 * 65536),   // 信号阈值
        (FixPoint1616_t)(18 * 65536),     // 噪声阈值
        33000,                            // 测距预算时间
        14,                               // 预测距脉冲周期
        10                                // 最终测距脉冲周期
    }, // 默认模式

    {
        (FixPoint1616_t)(0.25 * 65536),
        (FixPoint1616_t)(18 * 65536),
        200000,
        14,
        10
    }, // 高精度模式

    {
        (FixPoint1616_t)(0.1 * 65536),
        (FixPoint1616_t)(60 * 65536),
        33000,
        18,
        14
    }, // 长距离模式

    {
        (FixPoint1616_t)(0.25 * 65536),
        (FixPoint1616_t)(32 * 65536),
        20000,
        14,
        10
    }  // 高速模式
};


/**
 * @brief  配置VL53L0X测量模式
 * @param  dev: 设备I2C参数结构体指针
 * @param  mode: 测量模式，0:默认; 1:高精度; 2:长距离; 3:高速
 * @retval 状态信息，VL53L0X_Error类型
 */
VL53L0X_Error VL53L0X_SetMode_Simple(VL53L0X_Dev_t *dev, uint8_t mode)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
	
    uint32_t refSpadCount = 0;
    uint8_t isApertureSpads = 0;
	
    uint8_t VhvSettings = 0, PhaseCal = 0;

    /* 复位传感器，保证模式切换稳定 */
    VL53L0X_Reset(dev);

    /* 静态初始化 */
    status = VL53L0X_StaticInit(dev);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 执行参考校准 */
    status = VL53L0X_PerformRefCalibration(dev, &VhvSettings, &PhaseCal);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 执行参考SPAD管理 */
    status = VL53L0X_PerformRefSpadManagement(dev, &refSpadCount, &isApertureSpads);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设定设备工作模式为单次测量 */
    status = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 启用SIGMA范围检查 */
    status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 启用信号速率范围检查 */
    status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设置SIGMA范围阈值 */
    status = VL53L0X_SetLimitCheckValue(dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
                                        Mode_data[mode].sigmaLimit);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设置信号速率范围阈值 */
    status = VL53L0X_SetLimitCheckValue(dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
                                        Mode_data[mode].signalLimit);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设置测量时间预算 */
    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, Mode_data[mode].timingBudget);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设置VCSEL脉冲周期 - 预备阶段 */
    status = VL53L0X_SetVcselPulsePeriod(dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE,
                                         Mode_data[mode].preRangeVcselPeriod);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

    /* 设置VCSEL脉冲周期 - 最终阶段 */
    status = VL53L0X_SetVcselPulsePeriod(dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE,
                                         Mode_data[mode].finalRangeVcselPeriod);
    if (status != VL53L0X_ERROR_NONE)
        goto error;

error:
    if (status != VL53L0X_ERROR_NONE) {
        VL53L0X_PrintError(status);
    }
    return status;
}


/**
 * @brief  初始化vl53l0x
 * @param  dev:设备I2C参数结构体
 * @retval 状态信息
 */
VL53L0X_Error VL53L0X_init(VL53L0X_Dev_t *dev , uint8_t mode)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;

    /*I2C地址(上电默认0x52)*/
    dev->I2cDevAddr = VL53L0X_Addr;
    /*I2C通信模式*/
    dev->comms_type = 1;
    /*I2C通信速率*/
    dev->comms_speed_khz = 400;

    /*初始化IIC*/
    MX_I2C2_Init();
	
		/*初始化对应引脚*/
		VL_GPIO_Init();
	
    /* 通过 XSHUT 引脚关闭模块 */
    HAL_GPIO_WritePin(VL53L0X_XSH_GPIO_Port, VL53L0X_XSH_Pin, GPIO_PIN_RESET);
    HAL_Delay(30);

    /* 再次拉高 XSHUT，上电模块 */
    HAL_GPIO_WritePin(VL53L0X_XSH_GPIO_Port, VL53L0X_XSH_Pin, GPIO_PIN_SET);
    HAL_Delay(30);

    /*设置VL53L0X传感器I2C地址*/
    VL53L0X_AddrSet(dev, 0x50);
    if (Status != VL53L0X_ERROR_NONE)
        goto error;

    // 初始化内部数据结构
    Status = VL53L0X_DataInit(dev);
    if (Status) {
        printf("VL53L0X_DataInit 失败!!!\r\n");
        goto error;
    }

		/*配置测量模式*/
    while (VL53L0X_SetMode_Simple(dev, mode))
    {
        printf("VL53L0X_SetMode_Simple失败!!!\r\n");
    }
		
error:
    if (Status != VL53L0X_ERROR_NONE)
    {
        VL53L0X_PrintError(Status); /*打印错误信息*/
        return Status;
    }

    return Status;
}

