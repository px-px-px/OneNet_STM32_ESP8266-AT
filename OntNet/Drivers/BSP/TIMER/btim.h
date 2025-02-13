/**
 ****************************************************************************************************
 * @file        btim.h
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

#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* ������ʱ�� ���� */

/* TIMX �ж϶��� 
 * Ĭ�������TIM6/TIM7
 * ע��: ͨ���޸���4���궨��,����֧��TIM1~TIM8����һ����ʱ��.
 */
 
#define BTIM_TIMX_INT                       TIM6
#define BTIM_TIMX_INT_IRQn                  TIM6_DAC_IRQn
#define BTIM_TIMX_INT_IRQHandler            TIM6_DAC_IRQHandler
#define BTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)   /* TIM6 ʱ��ʹ�� */

/******************************************************************************************/

extern int Time1;								/* ����ʱ�䣬��λs */
extern int Time1_t;								/* ��������������λs */
extern int Time2;								/* ����ʱ�䣬��λs */
extern int Time2_t;								/* ���¼���������λs */
extern int Time3;								/* DHT11�ȴ�ʱ�䣬��λms */
extern int Time3_t;								/* DHT11�ȴ�����������λms */
extern int Time4;								/* TCP�ȴ�ʱ�䣬��λms */
extern int Time4_t;								/* TCP�ȴ�����������λms */
extern uint8_t LED_t;							/* ǰһ�ε�LED���� */
extern uint8_t humi, temp;						/* ��ǰ����ʪ������ */
extern uint8_t humi_t, temp_t;					/* ǰһ�ε���ʪ������ */

void DHT11_Wait(void);
void TCP_Wait(void);
void btim_timx_int_init(uint16_t arr, uint16_t psc);    /* ������ʱ�� ��ʱ�жϳ�ʼ������ */

#endif

















