#include "led/bsp_led.h"
#include "stm32f1xx_hal.h"  // 引入HAL库头文件

// LED引脚定义（根据实际硬件修改）
#define LED_GPIO_PORT    GPIOA
#define LED_GPIO_PIN     GPIO_PIN_0  // 示例：使用PA0作为LED引脚

/**
  * @brief  LED GPIO初始化配置
  * @param  无
  * @retval 无
  */
void LED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能LED所在GPIO端口时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();  // 若使用其他端口，修改为对应时钟宏

    // 配置LED引脚
    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速模式
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    // 初始化LED状态（例如：默认熄灭）
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
}

/**
  * @brief  控制LED点亮
  * @param  无
  * @retval 无
  */
void LED_On(void)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  控制LED熄灭
  * @param  无
  * @retval 无
  */
void LED_Off(void)
{
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
}

/**
  * @brief  翻转LED状态
  * @param  无
  * @retval 无
  */
void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
}