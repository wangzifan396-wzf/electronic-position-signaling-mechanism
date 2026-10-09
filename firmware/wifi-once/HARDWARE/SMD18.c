#include <string.h>
#include "SMD18.h"
#include "usart2.h"
#include "usart.h"	
#include "led.h"

uint16_t gSDM18_Distance = 0;  // 鏂板锛氫繚瀛樿窛绂荤殑鍏ㄥ眬鍙橀噺
uint8_t send_buf[18]={'\0'};

uint8_t Rx_buffer_temp[30];
uint8_t Rx_buffer_ok[30];
uint8_t Sensor_Data[30];
uint8_t newlines = 0;//1:接收到一包完整的数据 0:没数据

uint16_t CRC_buff = 0;

uint16_t calculate_crc16(uint8_t *puchMsg, int usDataLen)
{
  uint8_t wChar = 0;
	uint16_t wCRCin = 0xffff; //初始值
	uint16_t wCPoly = CRC16_POLYNOMIAL;//多项式
	uint16_t wResultXOR = 0; //结果异或值
	bool input_invert = true; //输入反转
	bool ouput_invert = true; //输出反转
	
	
    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        if(input_invert)//输入值反转
        {
            uint8_t temp_char = wChar;
            wChar=0;
            for(int i=0;i<8;++i)
            {
                if(temp_char&0x01)
                    wChar|=0x01<<(7-i);
                temp_char>>=1;
            }
        }
        wCRCin ^= (wChar << 8);
        for (int i = 0; i < 8; i++)
        {
            if (wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    if(ouput_invert)
    {
        uint16_t temp_short = wCRCin;
        wCRCin=0;
        for(int i=0;i<16;++i)
        {
            if(temp_short&0x01)
                wCRCin|=0x01<<(15-i);
            temp_short>>=1;
        }
    }
    return (wCRCin^wResultXOR);
}




//初始化配置SMD18
void SMD18_init(SDM18_Baud_t bound)
{
	HAL_Delay(200);//等待串口初始化完成
	
	stop_scan();//停止扫描
	HAL_Delay(200); //发送命令之间要有间隔
	
	//配置波特率
	//SMD18_setbaudrate(bound);

//	//开始扫描
	start_scan();

}

//停止扫描
void stop_scan(void)
{
	uint16_t crc_data=0x00;
	send_buf[0] = 0xA5;
	send_buf[1] = 0x03;
	send_buf[2] = 0x20;
	send_buf[3] = 0x02;
	send_buf[4] = 0x00;
	send_buf[5] = 0x00;
	send_buf[6] = 0x00;
	
	crc_data = calculate_crc16(send_buf,7);
	
	send_buf[7] = (crc_data & 0xFF00)>>8; //最高位
	send_buf[8] = crc_data & 0xFF; //最低位
	
	//校验位
//	send_buf[7] = 0x46;
//	send_buf[8] = 0x6E;
	
	USART2_DataString(send_buf,9);
	memset(send_buf,0,sizeof(send_buf));
}

//开始扫描
void start_scan(void)
{
	send_buf[0] = 0xA5;
	send_buf[1] = 0x03;
	send_buf[2] = 0x20;
	send_buf[3] = 0x01;
	send_buf[4] = 0x00;
	send_buf[5] = 0x00;
	send_buf[6] = 0x00;
	
	//校验位
	send_buf[7] = 0x02;
	send_buf[8] = 0x6E;
	
	USART2_DataString(send_buf,9);
	memset(send_buf,0,sizeof(send_buf));
}



//设置波特率
void SMD18_setbaudrate(SDM18_Baud_t i)
{
	uint16_t crc_data=0x00;
	
	send_buf[0] = 0xA5;
	send_buf[1] = 0x03;
	send_buf[2] = 0x20;
	send_buf[3] = 0x10;
	send_buf[4] = 0x00;
	send_buf[5] = 0x00;
	send_buf[6] = 0x01;
	
	
	switch(i)
	{
		case B9600:send_buf[7] = 0x00;break;
		case B14400:send_buf[7] = 0x01;break;
		case B19200:send_buf[7] = 0x02;break;
		case B38400:send_buf[7] = 0x03;break;
		case B43000:send_buf[7] = 0x04;break;
		
		case B57600:send_buf[7] = 0x05;break;
		case B76800:send_buf[7] = 0x06;break;
		case B115200:send_buf[7] = 0x07;break;
		case B128000:send_buf[7] = 0x08;break;
		case B230400:send_buf[7] = 0x09;break;
		
		case B256000:send_buf[7] = 0x0A;break;
		case B460800:send_buf[7] = 0x0B;break;
		case B921600:send_buf[7] = 0x0C;break;
		
		default :break;
	}
	
	crc_data = calculate_crc16(send_buf,8);
	
	send_buf[8] = (crc_data & 0xFF00)>>8; //最高位
	send_buf[9] = crc_data & 0xFF; //最低位
	
	USART2_DataString(send_buf,10);
	memset(send_buf,0,sizeof(send_buf));
	
}


//判断接收到的数据包是否完整 -只判断测距数据
void SDM18_Decode(uint8_t RxData)
{
	static uint8_t step_index = 0;
	static uint16_t len = 0;//数据长度 一般是14
	static uint16_t index = 0;
	
	switch(step_index)
	{
		case 0: Rx_buffer_temp[0] = RxData; //0xA5
						if(Rx_buffer_temp[0] == 0xA5)
							step_index++; 
						break; 
						
		case 1: Rx_buffer_temp[1] = RxData;//0x03
						if(Rx_buffer_temp[1] == 0x03)
							step_index++; 
						else
						{
							step_index = 0;//数据不对,重置接收
						}
						break; 
						
						
		case 2: Rx_buffer_temp[2] = RxData;//0x20
						if(Rx_buffer_temp[2] == 0x20)
							step_index++; 
						else
						{
							step_index = 0;//数据不对,重置接收
						}
						break; 
						
		case 3: Rx_buffer_temp[3] = RxData; 
						if(Rx_buffer_temp[3] == 0x01)
							step_index++; 
						else
						{
							step_index = 0;//数据不对,重置接收
						}
						break; 
						
		case 4: Rx_buffer_temp[4] = RxData;  step_index++; break; //0x00
		case 5: Rx_buffer_temp[5] = RxData;  step_index++; break; //数据长度 high
		case 6: Rx_buffer_temp[6] = RxData;  step_index++; //数据长度 LOW
						len = (Rx_buffer_temp[5]<<8 |Rx_buffer_temp[6]) -1;   //从0开始算
						break; 
		case 7: if(index< len) 
						{
							Rx_buffer_temp[7+index] = RxData;
							index ++;
						}
						else //超出数据长度
						{
							step_index++;
							index = 0;//数据段步长归0
						}
						break;
						
		//CRC校验码判断
		case 8: Rx_buffer_temp[8+len] = RxData; step_index++; break;//校验码 high
		case 9: Rx_buffer_temp[9+len] = RxData; step_index++;			//校验码 low
		
		//赋值操作		
		case 10: 
			memcpy(Rx_buffer_ok,Rx_buffer_temp,sizeof(Rx_buffer_temp));
			memset(Rx_buffer_temp,0,sizeof(Rx_buffer_temp));//清空
			
			CRC_buff = Rx_buffer_ok[8+len]<<8 | Rx_buffer_ok[9+len];
		
			index = 0;
			len = 0;
			step_index = 0;
			newlines = 1; //完整的数据
			break;					
		
	}
	
}

//输出距离、强度、干扰
void print_message(void)
{
	uint16_t crc_data;
	uint16_t dis = 0;//距离
	uint16_t strength = 0;//强度
	
	//长度计算
	int crclen = (Rx_buffer_ok[5]<<8 |Rx_buffer_ok[6]);
	crclen = crclen +7; //根据协议所得
	
	crc_data = calculate_crc16(Rx_buffer_ok , crclen );
	
	if(crc_data != CRC_buff)//校验码不对
	{
		return;
	}
	
	dis = Rx_buffer_ok[7+7]<<8 | Rx_buffer_ok[7+6]; //基本固定
	strength = Rx_buffer_ok[7+11]<<8 | Rx_buffer_ok[7+10];
	gSDM18_Distance = dis;  // 鏂板锛氫繚瀛樿窛绂诲�煎埌鍏ㄥ眬鍙橀噺
	printf("dis = %dmm\t , strength = %d\r\n",dis,strength);
	
}
