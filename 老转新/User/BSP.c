#include "BSP.h"

extern uint8_t newlines;

void BSP_init(void)
{
	MX_USART2_UART_Init();
	
	printf("waiting for SDM18 start work!\r\n");
	
	SMD18_init(B921600);  // SDM18初始化
}

void BSP_Loop(void)
{
	if(newlines == 1)
	{
		// LED;  // 注释掉，不操作LED
		// HAL_GPIO_TogglePin(LED_Port, LED_Pin);  // 也注释掉
		
		newlines = 0;  // 清除标志
		
		// 打印数据
		print_message();
		HAL_Delay(200);
	}
}