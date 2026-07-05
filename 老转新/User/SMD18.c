#include <string.h>
#include "SMD18.h"
#include "usart2.h"
//#include "usart.h"	
#include "led.h"


uint8_t send_buf[18]={'\0'};

uint8_t Rx_buffer_temp[30];
uint8_t Rx_buffer_ok[30];
uint8_t Sensor_Data[30];
uint8_t newlines = 0;//1:接收到一包完整数据 0:无数据

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

//初始化SMD18
void SMD18_init(SDM18_Baud_t bound)
{
	HAL_Delay(200);//等待串口初始化完成
	
	stop_scan();//停止扫描
	HAL_Delay(200); //发送命令后等待一段时间
	
	//设置波特率
	SMD18_setbaudrate(bound);

	//开始扫描
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
	
	send_buf[7] = (crc_data & 0xFF00)>>8; //存高8位
	send_buf[8] = crc_data & 0xFF; //存低8位
	
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
	
	send_buf[8] = (crc_data & 0xFF00)>>8; //存高8位
	send_buf[9] = crc_data & 0xFF; //存低8位
	
	USART2_DataString(send_buf,10);
	memset(send_buf,0,sizeof(send_buf));
	
}

//接收到的数据包解析函数 -只解析测距数据
void SDM18_Decode(uint8_t RxData)
{
	static uint8_t step_index = 0;
	static uint16_t len = 0;//数据长度一般14
	static uint16_t index = 0;
	static uint32_t packet_count = 0;
	
	switch(step_index)
	{
		case 0: 
			Rx_buffer_temp[0] = RxData;
			printf("Step0: 0x%02X\r\n", RxData);
			if(Rx_buffer_temp[0] == 0xA5)
			{
				step_index++; 
				printf("找到包头 A5\r\n");
			}
			break; 
						
		case 1: 
			Rx_buffer_temp[1] = RxData;
			printf("Step1: 0x%02X\r\n", RxData);
			if(Rx_buffer_temp[1] == 0x03)
			{
				step_index++; 
				printf("设备号正确 03\r\n");
			}
			else
			{
				step_index = 0;
				printf("设备号错误，重新接收\r\n");
			}
			break; 
						
		case 2: 
			Rx_buffer_temp[2] = RxData;
			printf("Step2: 0x%02X\r\n", RxData);
			if(Rx_buffer_temp[2] == 0x20)
			{
				step_index++; 
				printf("设备类型正确 20\r\n");
			}
			else
			{
				step_index = 0;
				printf("设备类型错误，重新接收\r\n");
			}
			break; 
						
		case 3: 
			Rx_buffer_temp[3] = RxData;
			printf("Step3: 0x%02X\r\n", RxData);
			if(Rx_buffer_temp[3] == 0x01)
			{
				step_index++; 
				printf("命令类型正确 01\r\n");
			}
			else
			{
				step_index = 0;
				printf("命令类型错误，重新接收\r\n");
			}
			break; 
						
		case 4: 
			Rx_buffer_temp[4] = RxData;  
			printf("Step4(预留位): 0x%02X\r\n", RxData);
			step_index++; 
			break;
			
		case 5: 
			Rx_buffer_temp[5] = RxData;  
			printf("Step5(数据长度高): 0x%02X\r\n", RxData);
			step_index++; 
			break;
			
		case 6: 
			Rx_buffer_temp[6] = RxData;
			printf("Step6(数据长度低): 0x%02X\r\n", RxData);
			len = (Rx_buffer_temp[5]<<8 | Rx_buffer_temp[6]) - 1;
			printf("数据长度: %d\r\n", len+1);
			step_index++; 
			break;
			
		case 7: 
			if(index < len) 
			{
				Rx_buffer_temp[7+index] = RxData;
				printf("数据[%d]: 0x%02X\r\n", index, RxData);
				index++;
			}
			else
			{
				printf("数据段接收完成，共%d字节\r\n", index);
				step_index++;
				index = 0;
			}
			break;
						
		case 8: 
			Rx_buffer_temp[8+len] = RxData;
			printf("CRC高8位: 0x%02X\r\n", RxData);
			step_index++; 
			break;
			
		case 9: 
			Rx_buffer_temp[9+len] = RxData;
			printf("CRC低8位: 0x%02X\r\n", RxData);
			step_index++; 
			break;
		
		case 10: 
			memcpy(Rx_buffer_ok, Rx_buffer_temp, sizeof(Rx_buffer_temp));
			memset(Rx_buffer_temp, 0, sizeof(Rx_buffer_temp));
			
			CRC_buff = Rx_buffer_ok[8+len]<<8 | Rx_buffer_ok[9+len];
			printf("CRC值: 0x%04X\r\n", CRC_buff);
			
			index = 0;
			len = 0;
			step_index = 0;
			newlines = 1;
			packet_count++;
			printf("===== 第%lu包数据接收完成 =====\r\n\r\n", packet_count);
			break;					
	}
}

//输出距离、强度、角度
void print_message(void)
{
	uint16_t crc_data;
	uint16_t dis = 0;//距离
	uint16_t strength = 0;//强度
	
	//长度计算
	int crclen = (Rx_buffer_ok[5]<<8 | Rx_buffer_ok[6]);
	crclen = crclen + 7; //根据协议手册
	
	crc_data = calculate_crc16(Rx_buffer_ok , crclen );
	
	if(crc_data != CRC_buff)//校验不对
	{
		printf("CRC校验失败: 计算值 0x%04X, 接收值 0x%04X\r\n", crc_data, CRC_buff);
		return;
	}
	
	dis = Rx_buffer_ok[7+7]<<8 | Rx_buffer_ok[7+6]; //基本固定
	strength = Rx_buffer_ok[7+11]<<8 | Rx_buffer_ok[7+10];
	
	printf("dis = %dmm\t , strength = %d\r\n", dis, strength);
}

/**
  * @brief SMD18传感器初始化（兼容main.c调用）
  * @param 无
  * @retval 无
  */
void SMD18_Init(void)
{
    SMD18_init(B115200);  // 使用115200波特率初始化
}

/**
  * @brief 获取距离值（毫米）
  * @param 无
  * @retval 距离值（毫米）
  */
uint16_t SMD18_GetDistance_MM(void)
{
    static uint16_t last_distance = 0;
    uint16_t crc_data;
    uint16_t distance_mm = 0;
    
    if(newlines == 1)
    {
        int crclen = (Rx_buffer_ok[5] << 8 | Rx_buffer_ok[6]);
        crclen = crclen + 7;
        
        crc_data = calculate_crc16(Rx_buffer_ok, crclen);
        
        if(crc_data == CRC_buff)
        {
            distance_mm = Rx_buffer_ok[7+7] << 8 | Rx_buffer_ok[7+6];
            last_distance = distance_mm;
            printf("解析到距离: %d mm\r\n", distance_mm);
        }
        else
        {
            printf("GetDistance CRC失败\r\n");
        }
        
        newlines = 0;
    }
    
    return last_distance;
}