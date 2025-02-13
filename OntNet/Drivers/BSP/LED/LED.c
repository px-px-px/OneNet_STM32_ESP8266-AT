#include "./BSP/LED/LED.h"

uint8_t LED;

/**
  * @brief     LED初始化函数
  * @param     无
  * @retval    无
  * @note	  无
  */
void LED_Init(void)
{
	/* 开启时钟 */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	/* 配置GPIO */
	GPIO_InitTypeDef GPIO_InitStruct;	//定义GPIO初始化结构体
	
	GPIO_InitStruct.Pin		=	GPIO_PIN_5;
	GPIO_InitStruct.Mode	=	GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed	=	GPIO_SPEED_FREQ_LOW;
	
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/* 设置初始值 */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);	//PB5接在LED(DS0)的负极
}

//LED翻转
void LED_Overturn(void)
{
	//硬件翻转
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
	//数据同步
	if(LED)
	{
		LED = 0;
	}
	else
	{
		LED = 1;
	}
}

