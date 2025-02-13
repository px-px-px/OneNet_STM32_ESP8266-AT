/**
 ******************************************************************************
 * @file     main.c
 * @author   正点原子团队(ALIENTEK)
 * @version  V1.0
 * @date     2020-08-20
 * @brief    新建工程实验-HAL库版本 实验
 * @license  Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ******************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 STM32F103 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 ******************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/LED.h"
#include "./BSP/KEY/KEY.h"
#include "./BSP/WDG/wdg.h"
#include "./BSP/DHT11/dht11.h"
#include "./BSP/TIMER/btim.h"

//网络设备驱动
#include "esp8266.h"

//平台驱动
#include "onenet.h"

int main(void)
{
	int len;
	char *dataPtr = NULL;
	
    HAL_Init();                         		/* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9); 		/* 设置时钟, 72Mhz */
    delay_init(72);                     		/* 延时初始化 */
	usart_init(115200);							/* 调试串口初始化 */
	KEY_Init();									/* 按键初始化 */
	LED_Init();									/* LED初始化 */
	TIM_Tool_Init(60, 10, 500, 200);	//设定心跳时间为1min，更新时间为10s，DHT11等待时间为500ms，TCP等待时间为200ms
	if(dht11_init())							/* DHT11初始化 */
	{
		printf("\r\n");
		printf("===================\r\n");
		printf("DTH11 不存在!\r\n");
		printf("===================\r\n");
		printf("\r\n");
	}	

	iwdg_init(IWDG_PRESCALER_256, 4096 - 1);	/* 初始化看门狗,超时时间大概26s */
	
	ESP8266_Init();								/* 网络通信设备ESP8266初始化 */ 
	iwdg_feed();
	OneNet_Unify();								/* 硬件同步初始化 */
	iwdg_feed();
	
	
	while(OneNet_DevLink())						/* 连接OneNet服务器 */
		delay_ms(500);
	
	MQTT_Common_Subscribe();					/* 订阅常用MQTT topic */	
	iwdg_feed();
	
	
	
	printf("\r\n");
	printf("+++++++++++++++++++\r\n");
	printf("        实验       \r\n");
	printf("-------------------\r\n");
	
	if(dht11_read_data(&temp, &humi))
	{
		if(dht11_check())printf("间隔测试1时DHT11断连，无法获取温湿度数据!\r\n");
		else printf("间隔测试1时DHT11出现异常错误，无法获取温湿度数据!\r\n");
		humi = humi_t;temp = temp_t;
	}
															
	if(dht11_read_data(&temp, &humi))
	{
		if(dht11_check())printf("间隔测试2时DHT11断连，无法获取温湿度数据!\r\n");
		else printf("间隔测试2时DHT11出现异常错误，无法获取温湿度数据!\r\n");
		humi = humi_t;temp = temp_t;
	}
	
	printf("-------------------\r\n");
	
	printf("结论：上述问题待解决\r\n");
	
	printf("-------------------\r\n");
	printf("+++++++++++++++++++\r\n");
	printf("\r\n");
	
    while(1)
    { 
		
		if(Time1_t >= Time1)
		{
			Time1_t = 0;
			printf("\r\n");
			printf("+++++++++++++++++++\r\n");
			printf("        心跳       \r\n");
			MQTT_Attribute_Reporting(); //心跳
			printf("+++++++++++++++++++\r\n");
			printf("\r\n");
		}
        
		if(Time2_t >= Time2)
		{
			Time2_t = 0;
			if(dht11_read_data(&temp, &humi))
			{
				printf("\r\n");
				printf("===================\r\n");
				if(dht11_check())printf("检测数据更新时DHT11断连，无法获取温湿度数据!\r\n");
				else printf("检测数据更新时DHT11出现异常错误，无法获取温湿度数据!\r\n");
				humi = humi_t;temp = temp_t;
				printf("===================\r\n");
				printf("\r\n");
			}
			
			if(temp != temp_t || humi != humi_t || LED != LED_t)
			{
				printf("\r\n");
				printf("+++++++++++++++++++\r\n");
				printf("      更新数据     \r\n");
				MQTT_Attribute_Reporting(); //更新数据
				temp_t = temp;humi_t = humi;LED_t = LED;
				printf("+++++++++++++++++++\r\n");
				printf("\r\n");
			}
		}
		
		
		if(KEY_Scan() == 1)						/* 扫描按键并更改LED状态 */
		{
			LED_Overturn();
		}
		
		dataPtr = ESP8266_GetIPD(0);			/* 获取平台响应 */
		if(dataPtr != NULL)
		{
			OneNet_RevPro(dataPtr);				/* 解析响应 */
		}
		
		
		iwdg_feed();        				
    }
}
