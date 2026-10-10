#include "BSP.h"

extern uint8_t newlines;

void BSP_init(void)
{
	USART2_UART_Init();
	
	printf("waiting for SDM18 start work!\r\n");
	
	SMD18_init(B921600);//SDM18初始化
	
}

void BSP_Loop(void)
{
	if(newlines == 1)
		{
			LED;
			newlines = 0;//清掉它
			//打印信息
			print_message();
			HAL_Delay(200);
		}
			
}

