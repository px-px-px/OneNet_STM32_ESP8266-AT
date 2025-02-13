/**
 ******************************************************************************
 * @file     main.c
 * @author   ����ԭ���Ŷ�(ALIENTEK)
 * @version  V1.0
 * @date     2020-08-20
 * @brief    �½�����ʵ��-HAL��汾 ʵ��
 * @license  Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ******************************************************************************
 * @attention
 * 
 * ʵ��ƽ̨:����ԭ�� STM32F103 ������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
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

//�����豸����
#include "esp8266.h"

//ƽ̨����
#include "onenet.h"

int main(void)
{
	int len;
	char *dataPtr = NULL;
	
    HAL_Init();                         		/* ��ʼ��HAL�� */
    sys_stm32_clock_init(RCC_PLL_MUL9); 		/* ����ʱ��, 72Mhz */
    delay_init(72);                     		/* ��ʱ��ʼ�� */
	usart_init(115200);							/* ���Դ��ڳ�ʼ�� */
	KEY_Init();									/* ������ʼ�� */
	LED_Init();									/* LED��ʼ�� */
	TIM_Tool_Init(60, 10, 500, 200);	//�趨����ʱ��Ϊ1min������ʱ��Ϊ10s��DHT11�ȴ�ʱ��Ϊ500ms��TCP�ȴ�ʱ��Ϊ200ms
	if(dht11_init())							/* DHT11��ʼ�� */
	{
		printf("\r\n");
		printf("===================\r\n");
		printf("DTH11 ������!\r\n");
		printf("===================\r\n");
		printf("\r\n");
	}	

	iwdg_init(IWDG_PRESCALER_256, 4096 - 1);	/* ��ʼ�����Ź�,��ʱʱ����26s */
	
	ESP8266_Init();								/* ����ͨ���豸ESP8266��ʼ�� */ 
	iwdg_feed();
	OneNet_Unify();								/* Ӳ��ͬ����ʼ�� */
	iwdg_feed();
	
	
	while(OneNet_DevLink())						/* ����OneNet������ */
		delay_ms(500);
	
	MQTT_Common_Subscribe();					/* ���ĳ���MQTT topic */	
	iwdg_feed();
	
	
	
	printf("\r\n");
	printf("+++++++++++++++++++\r\n");
	printf("        ʵ��       \r\n");
	printf("-------------------\r\n");
	
	if(dht11_read_data(&temp, &humi))
	{
		if(dht11_check())printf("�������1ʱDHT11�������޷���ȡ��ʪ������!\r\n");
		else printf("�������1ʱDHT11�����쳣�����޷���ȡ��ʪ������!\r\n");
		humi = humi_t;temp = temp_t;
	}
															
	if(dht11_read_data(&temp, &humi))
	{
		if(dht11_check())printf("�������2ʱDHT11�������޷���ȡ��ʪ������!\r\n");
		else printf("�������2ʱDHT11�����쳣�����޷���ȡ��ʪ������!\r\n");
		humi = humi_t;temp = temp_t;
	}
	
	printf("-------------------\r\n");
	
	printf("���ۣ�������������\r\n");
	
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
			printf("        ����       \r\n");
			MQTT_Attribute_Reporting(); //����
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
				if(dht11_check())printf("������ݸ���ʱDHT11�������޷���ȡ��ʪ������!\r\n");
				else printf("������ݸ���ʱDHT11�����쳣�����޷���ȡ��ʪ������!\r\n");
				humi = humi_t;temp = temp_t;
				printf("===================\r\n");
				printf("\r\n");
			}
			
			if(temp != temp_t || humi != humi_t || LED != LED_t)
			{
				printf("\r\n");
				printf("+++++++++++++++++++\r\n");
				printf("      ��������     \r\n");
				MQTT_Attribute_Reporting(); //��������
				temp_t = temp;humi_t = humi;LED_t = LED;
				printf("+++++++++++++++++++\r\n");
				printf("\r\n");
			}
		}
		
		
		if(KEY_Scan() == 1)						/* ɨ�谴��������LED״̬ */
		{
			LED_Overturn();
		}
		
		dataPtr = ESP8266_GetIPD(0);			/* ��ȡƽ̨��Ӧ */
		if(dataPtr != NULL)
		{
			OneNet_RevPro(dataPtr);				/* ������Ӧ */
		}
		
		
		iwdg_feed();        				
    }
}
