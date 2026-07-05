#include "board.h"
#include "led/bsp_led.h"
#include "stm32f1xx_hal.h"  // ?? HAL ????

/**********************************************************
***	Emm_V5.0???????
***	????:ZHANGDATOU
***	????:????????
***	????:https://zhangdatou.taobao.com
***	CSDN??:https://blog.csdn.net/zhangdatou666
***	qq???:262438510
**********************************************************/

/**
	*	@brief		?????
	*	@param		?
	*	@retval		?
	*/
void clock_init(void)
{
	// ?? GPIOA ??(HAL ???)
	__HAL_RCC_GPIOA_CLK_ENABLE();
	// ???? GPIO ??,???? __HAL_RCC_GPIOB_CLK_ENABLE() ?
}

/**
	*	@brief		gpio?????(???????LED)
	*	@param		?
	*	@retval		?
	*/
// ?board.c??????????????
void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    // ??????:PA5(En), PA6(Stp), PA7(Dir)
    GPIO_InitStructure.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // ??????
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);  // En??
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);  // Stp???
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // Dir????
}

/**
	*	@brief		?????
	*	@param		?
	*	@retval		?
	*/
void board_init(void)
{
	clock_init();  // ?????
	gpio_init();   // ??? GPIO(??+LED)
}