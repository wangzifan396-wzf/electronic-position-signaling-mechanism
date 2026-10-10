#include "stepper.h"
#include "gpio.h"

/* 私有变量 */
static uint32_t pulse_delay = PULSE_DELAY_US;  // 当前脉冲延时（微秒）
static MotorState_t motor_state = MOTOR_STOP;   // 电机当前状态
static uint32_t total_steps_count = 0;          // 总步数计数

/* 私有函数声明 */
static void delay_us(uint32_t us);
static void SendOnePulse(void);

/**
 * @brief 微秒延时函数
 * @param us 延时微秒数
 * @retval 无
 * @note 基于72MHz主频的简单延时，如需精确延时建议使用定时器
 */
static void delay_us(uint32_t us)
{
    uint32_t count = us * 8;  // 72MHz主频时大约为8个时钟周期/微秒
    while(count--)
    {
        __NOP();  // 空操作，占用一个指令周期
    }
}

/**
 * @brief 发送一个脉冲
 * @param 无
 * @retval 无
 * @note 产生一个完整的脉冲信号：高电平->延时->低电平->延时
 */
static void SendOnePulse(void)
{
    // 脉冲高电平
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_SET);
    delay_us(pulse_delay / 2);
    
    // 脉冲低电平
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_RESET);
    delay_us(pulse_delay / 2);
    
    // 统计步数
    total_steps_count++;
}

/**
 * @brief 初始化步进电机
 * @param 无
 * @retval 无
 * @note 设置电机默认参数并使能
 */
void Stepper_Init(void)
{
    // GPIO已经在MX_GPIO_Init中配置好了
    // 这里进行电机初始化设置
    
    Stepper_Enable();                    // 使能电机
    Stepper_SetDirection(STEPPER_DIR_CW); // 默认顺时针方向
    Stepper_SetSpeed(SPEED_MEDIUM);       // 默认中速
    
    motor_state = MOTOR_STOP;
    total_steps_count = 0;
}

/**
 * @brief 使能电机
 * @param 无
 * @retval 无
 * @note EN引脚低电平有效（根据实际驱动器可能不同）
 */
void Stepper_Enable(void)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, GPIO_PIN_RESET);  // EN低电平有效
}

/**
 * @brief 禁用电机
 * @param 无
 * @retval 无
 * @note 禁用后电机失去保持力，可以自由转动
 */
void Stepper_Disable(void)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_EN_PIN, GPIO_PIN_SET);    // 高电平禁用
    motor_state = MOTOR_STOP;
}

/**
 * @brief 设置电机方向
 * @param dir 方向：STEPPER_DIR_CW(顺时针) 或 STEPPER_DIR_CCW(逆时针)
 * @retval 无
 */
void Stepper_SetDirection(uint8_t dir)
{
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN, dir);
}

/**
 * @brief 设置电机速度
 * @param delay_us 脉冲间隔（微秒）
 * @retval 无
 * @note 值越小速度越快，建议范围：500-5000微秒
 *       过快的速度可能导致电机丢步
 */
void Stepper_SetSpeed(uint32_t delay_us)
{
    // 限制速度在合理范围内，防止参数错误
    if(delay_us < 100)
    {
        pulse_delay = 100;  // 最快速度限制
    }
    else if(delay_us > 10000)
    {
        pulse_delay = 10000; // 最慢速度限制
    }
    else
    {
        pulse_delay = delay_us;
    }
}

/**
 * @brief 走指定步数
 * @param steps 要走的步数
 * @retval 无
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
 * @param direction 转动方向
 * @param duration_ms 持续时间（毫秒）
 * @retval 无
 * @note 持续转动指定时间，用于需要连续运动的场景
 */
void Stepper_RotateContinuous(uint8_t direction, uint32_t duration_ms)
{
    uint32_t start_time = HAL_GetTick();
    
    // 设置方向
    Stepper_SetDirection(direction);
    motor_state = (direction == STEPPER_DIR_CW) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    
    // 持续转动
    while((HAL_GetTick() - start_time) < duration_ms)
    {
        SendOnePulse();
        // 不添加额外延时，靠脉冲延时控制速度
    }
    
    motor_state = MOTOR_STOP;
}

/**
 * @brief 根据距离控制电机
 * @param distance 当前测量的距离（厘米）
 * @param target 目标距离（厘米）
 * @param tolerance 允许的误差范围（厘米）
 * @retval 无
 * 
 * @note 控制逻辑：
 *       距离 < 目标-公差：电机正转（远离）
 *       距离 > 目标+公差：电机反转（靠近）
 *       距离在目标±公差内：停止
 *       每次发送5个脉冲，连续100步后短暂延时防止过热
 */
void Stepper_ControlByDistance(float distance, float target, float tolerance)
{
    static uint16_t continuous_steps = 0;  // 连续步数计数
    static float last_distance = 0;         // 上次距离
    
    float min_distance = target - tolerance;  // 最小允许距离
    float max_distance = target + tolerance;  // 最大允许距离
    
    // 保存上次距离
    last_distance = distance;
    
    // 根据距离决定电机动作
    if(distance < min_distance)
    {
        // 太近，需要远离（正转）
        if(motor_state != MOTOR_FORWARD)
        {
            Stepper_SetDirection(STEPPER_DIR_CW);  // 正转（远离）
            motor_state = MOTOR_FORWARD;
            continuous_steps = 0;  // 方向改变时重置计数
        }
        
        // 每次发送5个脉冲
        Stepper_Step(5);
        continuous_steps += 5;
        
        // 每100步短暂延时，防止电机驱动器过热
        if(continuous_steps >= 100)
        {
            HAL_Delay(5);  // 延时5ms
            continuous_steps = 0;
        }
    }
    else if(distance > max_distance)
    {
        // 太远，需要靠近（反转）
        if(motor_state != MOTOR_BACKWARD)
        {
            Stepper_SetDirection(STEPPER_DIR_CCW);  // 反转（靠近）
            motor_state = MOTOR_BACKWARD;
            continuous_steps = 0;  // 方向改变时重置计数
        }
        
        // 每次发送5个脉冲
        Stepper_Step(5);
        continuous_steps += 5;
        
        // 每100步短暂延时
        if(continuous_steps >= 100)
        {
            HAL_Delay(5);  // 延时5ms
            continuous_steps = 0;
        }
    }
    else
    {
        // 在目标范围内，停止
        if(motor_state != MOTOR_STOP)
        {
            motor_state = MOTOR_STOP;
            continuous_steps = 0;  // 停止时重置计数
        }
    }
}

/**
 * @brief 获取电机当前状态
 * @retval MotorState_t 电机状态（停止/正转/反转）
 */
MotorState_t Stepper_GetState(void)
{
    return motor_state;
}

/**
 * @brief 获取总步数
 * @retval uint32_t 从初始化开始的总步数
 */
uint32_t Stepper_GetTotalSteps(void)
{
    return total_steps_count;
}

/**
 * @brief 重置总步数计数器
 * @param 无
 * @retval 无
 */
void Stepper_ResetTotalSteps(void)
{
    total_steps_count = 0;
}

/**
 * @brief 获取当前速度
 * @retval uint32_t 当前脉冲延时（微秒）
 */
uint32_t Stepper_GetSpeed(void)
{
    return pulse_delay;
}

/**
 * @brief 紧急停止
 * @param 无
 * @retval 无
 * @note 立即停止电机，并禁用输出
 */
void Stepper_EmergencyStop(void)
{
    // 停止发送脉冲
    HAL_GPIO_WritePin(STEPPER_GPIO_PORT, STEPPER_STEP_PIN, GPIO_PIN_RESET);
    
    // 禁用电机
    Stepper_Disable();
    
    motor_state = MOTOR_STOP;
}

/**
 * @brief 复位电机
 * @param 无
 * @retval 无
 * @note 重新使能电机，设置为默认状态
 */
void Stepper_Reset(void)
{
    Stepper_Disable();
    HAL_Delay(10);  // 等待10ms
    Stepper_Enable();
    Stepper_SetDirection(STEPPER_DIR_CW);
    motor_state = MOTOR_STOP;
}

/**
 * @brief 移动到指定位置（相对位置）
 * @param steps 相对步数
 * @param direction 方向
 * @retval 无
 */
void Stepper_MoveRelative(uint16_t steps, uint8_t direction)
{
    Stepper_SetDirection(direction);
    motor_state = (direction == STEPPER_DIR_CW) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    
    Stepper_Step(steps);
    
    motor_state = MOTOR_STOP;
}

/**
 * @brief 判断电机是否在运行
 * @retval uint8_t 1:运行中, 0:停止
 */
uint8_t Stepper_IsRunning(void)
{
    return (motor_state != MOTOR_STOP);
}

/**
 * @brief 获取当前方向
 * @retval uint8_t 当前方向
 */
uint8_t Stepper_GetDirection(void)
{
    return HAL_GPIO_ReadPin(STEPPER_GPIO_PORT, STEPPER_DIR_PIN);
}

/**
 * @brief 获取当前使能状态
 * @retval uint8_t 1:使能, 0:禁用
 */
uint8_t Stepper_IsEnabled(void)
{
    // EN引脚低电平有效，所以读取到的值是0表示使能
    return (HAL_GPIO_ReadPin(STEPPER_GPIO_PORT, STEPPER_EN_PIN) == GPIO_PIN_RESET) ? 1 : 0;
}