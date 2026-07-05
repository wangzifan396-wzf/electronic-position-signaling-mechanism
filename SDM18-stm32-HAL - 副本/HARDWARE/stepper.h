#ifndef __STEPPER_H
#define __STEPPER_H

#include "main.h"
#include "gpio.h"

/* 步进电机参数定义 */
#define STEPS_PER_REV    200      // 每圈步数（1.8度步距角）
#define PULSE_DELAY_US    1000    // 默认脉冲间隔（微秒）
#define SPEED_SLOW        2000    // 慢速（微秒）
#define SPEED_MEDIUM      1000    // 中速（微秒）
#define SPEED_FAST        500     // 快速（微秒）

/* 电机状态枚举 */
typedef enum {
    MOTOR_STOP = 0,     // 停止
    MOTOR_FORWARD,      // 正转（远离）
    MOTOR_BACKWARD      // 反转（靠近）
} MotorState_t;

/* 基本控制函数 */
void Stepper_Init(void);                                    // 初始化步进电机
void Stepper_Enable(void);                                  // 使能电机
void Stepper_Disable(void);                                 // 禁用电机
void Stepper_SetDirection(uint8_t dir);                     // 设置方向
void Stepper_SetSpeed(uint32_t delay_us);                   // 设置速度
void Stepper_Step(uint16_t steps);                          // 走指定步数

/* 高级控制函数 */
void Stepper_RotateContinuous(uint8_t direction, uint32_t duration_ms);  // 连续转动
void Stepper_ControlByDistance(float distance, float target, float tolerance); // 根据距离控制
void Stepper_MoveRelative(uint16_t steps, uint8_t direction);            // 相对移动

/* 状态查询函数 */
MotorState_t Stepper_GetState(void);                        // 获取电机状态
uint32_t Stepper_GetTotalSteps(void);                       // 获取总步数
uint32_t Stepper_GetSpeed(void);                            // 获取当前速度
uint8_t Stepper_IsRunning(void);                            // 判断是否在运行
uint8_t Stepper_GetDirection(void);                         // 获取当前方向
uint8_t Stepper_IsEnabled(void);                            // 获取使能状态

/* 其他功能函数 */
void Stepper_ResetTotalSteps(void);                         // 重置总步数
void Stepper_EmergencyStop(void);                           // 紧急停止
void Stepper_Reset(void);                                   // 复位电机

#endif