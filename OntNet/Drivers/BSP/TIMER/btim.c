/**
 ****************************************************************************************************
 * @file        btim.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       ������ʱ�� ��������
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20211216
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#include "./BSP/LED/led.h"
#include "./BSP/TIMER/btim.h"
#include "./BSP/DHT11/dht11.h"
#include "onenet.h"


TIM_HandleTypeDef g_timx_handle;  /* ��ʱ����� */

/**
 * @brief       ������ʱ��TIMX��ʱ�жϳ�ʼ������
 * @note
 *              ������ʱ����ʱ������APB1,��PPRE1 �� 2��Ƶ��ʱ��
 *              ������ʱ����ʱ��ΪAPB1ʱ�ӵ�2��, ��APB1Ϊ36M, ���Զ�ʱ��ʱ�� = 72Mhz
 *              ��ʱ�����ʱ����㷽��: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=��ʱ������Ƶ��,��λ:Mhz
 *
 * @param       arr: �Զ���װֵ��
 * @param       psc: ʱ��Ԥ��Ƶ��
 * @retval      ��
 */
void btim_timx_int_init(uint16_t arr, uint16_t psc)
{
    g_timx_handle.Instance = BTIM_TIMX_INT;                      /* ͨ�ö�ʱ��X */
    g_timx_handle.Init.Prescaler = psc;                          /* ����Ԥ��Ƶϵ�� */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;         /* ��������ģʽ */
    g_timx_handle.Init.Period = arr;                             /* �Զ�װ��ֵ */
    HAL_TIM_Base_Init(&g_timx_handle);

    HAL_TIM_Base_Start_IT(&g_timx_handle);    /* ʹ�ܶ�ʱ��x��������ж� */
}

/**
 * @brief       ��ʱ���ײ�����������ʱ�ӣ������ж����ȼ�
                �˺����ᱻHAL_TIM_Base_Init()��������
 * @param       htim:��ʱ�����
 * @retval      ��
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIMX_INT)
    {
        BTIM_TIMX_INT_CLK_ENABLE();                     /* ʹ��TIMʱ�� */
        HAL_NVIC_SetPriority(BTIM_TIMX_INT_IRQn, 1, 3); /* ��ռ1�������ȼ�3����2 */
        HAL_NVIC_EnableIRQ(BTIM_TIMX_INT_IRQn);         /* ����ITM3�ж� */
    }
}

/**
 * @brief       ��ʱ��TIMX�жϷ�����
 * @param       ��
 * @retval      ��
 */
void BTIM_TIMX_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_handle); /* ��ʱ���жϹ��������� */
}

int Time1;								/* ����ʱ�䣬��λs */
int Time1_t = 0;						/* ��������������λs */
int Time2;								/* ����ʱ�䣬��λs */
int Time2_t = 0;						/* ���¼���������λs */
int Time3;								/* DHT11�ȴ�ʱ�䣬��λms */
int Time3_t = 0;						/* DHT11�ȴ�����������λms */
int Time4;								/* TCP�ȴ�ʱ�䣬��λms */
int Time4_t = 0;						/* TCP�ȴ�����������λms */
uint8_t LED_t;							/* ǰһ�ε�LED���� */
uint8_t humi, temp;						/* ��ǰ����ʪ������ */
uint8_t humi_t, temp_t;					/* ǰһ�ε���ʪ������ */

//DHT11æ��
void DHT11_Wait(void)
{
	while(Time3_t);
	Time3_t = Time3;
}
//TCPæ��
void TCP_Wait(void)
{
	while(Time4_t);
	Time4_t = Time4;
}


/**
 * @brief       ��ʱ�������жϻص�����
 * @param       htim:��ʱ�����
 * @retval      ��
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == BTIM_TIMX_INT)
    {
		Time1_t ++;
		Time2_t ++;
		if(Time3_t > 0)
			Time3_t --;
		if(Time4_t > 0)
			Time4_t --;
    }
}




