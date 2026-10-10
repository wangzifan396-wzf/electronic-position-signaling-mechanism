/*
 * COPYRIGHT (C) STMicroelectronics 2014. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/**
 * @file   VL53L0X_platform.h
 * @brief  Function prototype definitions for Ewok Platform layer.
 *
 */


#ifndef _VL53L0X_I2C_PLATFORM_H_
#define _VL53L0X_I2C_PLATFORM_H_

#include "vl53l0x_def.h"

#ifdef __cplusplus
extern "C" {
#endif

// Include uint8_t, unit16_t  etc definitions

#include <stdint.h>
#include <stdarg.h>


/**
 *  @brief Typedef defining .\n
 * The developer shoud modify this to suit the platform being deployed.
 *
 */

// enum  {TRUE = true, FALSE = false};

/**
 * @brief Typedef defining 8 bit unsigned char type.\n
 * The developer shoud modify this to suit the platform being deployed.
 *
 */

#ifndef bool_t
typedef unsigned char bool_t;
#endif


#define	   I2C                0x01
#define	   SPI                0x00

#define    COMMS_BUFFER_SIZE    64  // MUST be the same size as the SV task buffer

#define    BYTES_PER_WORD        2
#define    BYTES_PER_DWORD       4

#define    VL53L0X_MAX_STRING_LENGTH_PLT       256






#ifdef __cplusplus
}
#endif

#endif //_VL53L0X_I2C_PLATFORM_H_

