#include "led.h"

/**************************************************************************
Function: Led flashing
Input   : time：Flicker frequency
Output  : none
函数功能：LED闪烁
入口参数：闪烁频率 
返回  值：无
**************************************************************************/
void Led_Flash(uint16_t time)
{
	 static int temp;
	 if(0==time) 
	 {
			LED_OFF;
	 }
		 
	 else if(++temp==time)
	 {
			LED_OFF; 
			temp=0;
	 }
		 
}
