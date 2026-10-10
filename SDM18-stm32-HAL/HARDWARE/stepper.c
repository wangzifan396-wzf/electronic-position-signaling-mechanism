#include "stepper.h"
#include "gpio.h"

/* 私有变量 */
static uint32_t pulse_delay = PULSE_DELAY_US;
static MotorState_t motor_state = MOTOR_STOP;
static uint32_t total_steps_count = 0;

/* 私有函数声明 */
static void delay_us(uint32_t us);
static void SendOnePulse(void);

/**
 * @brief 微秒延时函数
 * @param us 延时微秒数
 */
static void delay_us(uint32_t us)
{
    uint32_t count = us * 8;
    while(count--)
    {
        __NOP();
    }
}

/**
 * @brief 发送一个脉冲 - 修改为和老代码一样的寄存器翻转方式
 */
static void SendOnePulse(void)
{
    // 翻转PA6产生脉冲（和老代码完全一样）
    GPIOA->ODR ^= GPIO_PIN_6;
    delay_us(pulse_delay);
    GPIOA->ODR ^= GPIO_PIN_6;
    delay_us(pulse_delay);
    
    total_steps_count++;
}

/**
 * @brief 初始化步进电机
 */
void Stepper_Init(void)
{
    Stepper_Enable();
    Stepper_SetDirection(STEPPER_DIR_CW);
    Stepper_SetSpeed(SPEED_SLOW);
    
    motor_state = MOTOR_STOP;
    total_steps_count = 0;
}

/**
 * @brief 使能电机
 */
void Stepper_Enable(void)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 禁用电机
 */
void Stepper_Disable(void)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, GPIO_PIN_SET);
    motor_state = MOTOR_STOP;
}

/**
 * @brief 设置电机方向
 */
void Stepper_SetDirection(uint8_t dir)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN, dir);
}

/**
 * @brief 设置电机速度
 */
void Stepper_SetSpeed(uint32_t delay_us)
{
    if(delay_us < 100)
    {
        pulse_delay = 100;
    }
    else if(delay_us > 50000)
    {
        pulse_delay = 50000;
    }
    else
    {
        pulse_delay = delay_us;
    }
}

/**
 * @brief 走指定步数
 */
void Stepper_Step(uint16_t steps)
{
    for(uint16_t i = 0; i < steps; i++)
    {
        SendOnePulse();
    }
}

/**
 * @brief 连续转动
 */
void Stepper_RotateContinuous(uint8_t direction, uint32_t duration_ms)
{
    uint32_t start_time = HAL_GetTick();
    
    Stepper_SetDirection(direction);
    motor_state = (direction == STEPPER_DIR_CW) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    
    while((HAL_GetTick() - start_time) < duration_ms)
    {
        SendOnePulse();
    }
    
    motor_state = MOTOR_STOP;
}

/**
 * @brief 根据距离控制电机
 */
void Stepper_ControlByDistance(float distance, float target, float tolerance)
{
    static uint16_t continuous_steps = 0;
    
    float min_distance = target - tolerance;
    float max_distance = target + tolerance;
    
    if(distance < min_distance)
    {
        if(motor_state != MOTOR_FORWARD)
        {
            Stepper_SetDirection(STEPPER_DIR_CW);
            motor_state = MOTOR_FORWARD;
            continuous_steps = 0;
        }
        
        Stepper_Step(5);
        continuous_steps += 5;
        
        if(continuous_steps >= 100)
        {
            HAL_Delay(5);
            continuous_steps = 0;
        }
    }
    else if(distance > max_distance)
    {
        if(motor_state != MOTOR_BACKWARD)
        {
            Stepper_SetDirection(STEPPER_DIR_CCW);
            motor_state = MOTOR_BACKWARD;
            continuous_steps = 0;
        }
        
        Stepper_Step(5);
        continuous_steps += 5;
        
        if(continuous_steps >= 100)
        {
            HAL_Delay(5);
            continuous_steps = 0;
        }
    }
    else
    {
        if(motor_state != MOTOR_STOP)
        {
            motor_state = MOTOR_STOP;
            continuous_steps = 0;
        }
    }
}

// 以下函数保持不变...
MotorState_t Stepper_GetState(void)
{
    return motor_state;
}

uint32_t Stepper_GetTotalSteps(void)
{
    return total_steps_count;
}

void Stepper_ResetTotalSteps(void)
{
    total_steps_count = 0;
}

uint32_t Stepper_GetSpeed(void)
{
    return pulse_delay;
}

void Stepper_EmergencyStop(void)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_RESET);
    Stepper_Disable();
    motor_state = MOTOR_STOP;
}

void Stepper_Reset(void)
{
    Stepper_Disable();
    HAL_Delay(10);
    Stepper_Enable();
    Stepper_SetDirection(STEPPER_DIR_CW);
    motor_state = MOTOR_STOP;
}

void Stepper_MoveRelative(uint16_t steps, uint8_t direction)
{
    Stepper_SetDirection(direction);
    motor_state = (direction == STEPPER_DIR_CW) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    Stepper_Step(steps);
    motor_state = MOTOR_STOP;
}

uint8_t Stepper_IsRunning(void)
{
    return (motor_state != MOTOR_STOP);
}

uint8_t Stepper_GetDirection(void)
{
    return HAL_GPIO_ReadPin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN);
}

uint8_t Stepper_IsEnabled(void)
{
    return (HAL_GPIO_ReadPin(STEPPER_GPIO_PORT, STEPPER_EN_PIN) == GPIO_PIN_RESET) ? 1 : 0;
}