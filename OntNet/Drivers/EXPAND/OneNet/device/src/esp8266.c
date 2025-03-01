//单片机头文件
#include "stm32f1xx.h"

//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"
#include "./BSP/TIMER/btim.h"

//C库
#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"{WIFI 名称}\",\"{WIFI 密码}\"\r\n"


unsigned char esp8266_buf[2048];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;


//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;
	esp8266_cntPre = 0;

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
		esp8266_cntPre = 0;
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 400;

	Usart_SendString(USART3, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		delay_ms(10);
	}
	
	return 1;

}

//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//清空接收缓存
	TCP_Wait();										//忙等
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		Usart_SendString(USART3, data, len);		//发送设备连接请求数据
	}

}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
//				printf("\r\n");
//				printf("===================\r\n");
//				printf("ESP8266 数据传输出错: \r\n");
//				printf("头部\"+IPD\"丢失!\r\n");
//				printf("===================\r\n");
//				printf("\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					Time1_t = 0;										//心跳计数器重置
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		delay_ms(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Init(void)
{
	
	/* 开启时钟 */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	/* 配置GPIO */
	GPIO_InitTypeDef GPIO_InitStruct;	//定义GPIO初始化结构体
	
	GPIO_InitStruct.Pin		=	GPIO_PIN_4;
	GPIO_InitStruct.Mode	=	GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed	=	GPIO_SPEED_FREQ_LOW;
	
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	/* 硬件重置 */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	delay_ms(500);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	delay_ms(500);
	
	ESP8266_Clear();
	
	usart3_init(115200);
	printf("\r\n");
	printf("===================\r\n");
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		delay_ms(500);
	printf("ESP8266 AT OK!\r\n");
	
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		delay_ms(500);
	printf("CWMODE SET OK!\r\n");
	
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		delay_ms(500);
	printf("CWDHCP SET OK!\r\n");
	
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
		delay_ms(500);
	printf("WIFI Link OK!\r\n");

	printf("-------------------\r\n");
	printf("ESP8266 Init OK!\r\n");
	printf("-------------------\r\n");
	
	printf("===================\r\n");
	printf("\r\n");

}
