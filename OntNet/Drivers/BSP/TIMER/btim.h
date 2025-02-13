/**
 ****************************************************************************************************
 * @file        btim.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       基本定时器 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211216
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 基本定时器 定义 */

/* TIMX 中断定义 
 * 默认是针对TIM6/TIM7
 * 注意: 通过修改这4个宏定义,可以支持TIM1~TIM8任意一个定时器.
 */
 
#define BTIM_TIMX_INT                       TIM6
#define BTIM_TIMX_INT_IRQn                  TIM6_DAC_IRQn
#define BTIM_TIMX_INT_IRQHandler            TIM6_DAC_IRQHandler
#define BTIM_TIMX_INT_CLK_ENABLE()          do{ __HAL_RCC_TIM6_CLK_ENABLE(); }while(0)   /* TIM6 时钟使能 */

/******************************************************************************************/

extern int Time1;								/* 心跳时间，单位s */
extern int Time1_t;								/* 心跳计数器，单位s */
extern int Time2;								/* 更新时间，单位s */
extern int Time2_t;								/* 更新计数器，单位s */
extern int Time3;								/* DHT11等待时间，单位ms */
extern int Time3_t;								/* DHT11等待计数器，单位ms */
extern int Time4;								/* TCP等待时间，单位ms */
extern int Time4_t;								/* TCP等待计数器，单位ms */
extern uint8_t LED_t;							/* 前一次的LED数据 */
extern uint8_t humi, temp;						/* 当前的温湿度数据 */
extern uint8_t humi_t, temp_t;					/* 前一次的温湿度数据 */

void DHT11_Wait(void);
void TCP_Wait(void);
void btim_timx_int_init(uint16_t arr, uint16_t psc);    /* 基本定时器 定时中断初始化函数 */

#endif

















