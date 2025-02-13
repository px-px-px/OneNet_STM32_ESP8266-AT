#include "./SYSTEM/delay/delay.h"
#include "./BSP/KEY/KEY.h"

/**
  * @brief     KEY初始化函数
  * @param     无
  * @retval    无
  * @note	   无
  */
void KEY_Init(void)
{
	/* 开启时钟 */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	
	/* 配置GPIO */
	GPIO_InitTypeDef GPIO_InitStruct;	//定义GPIO初始化结构体
	
	GPIO_InitStruct.Pin		=	GPIO_PIN_2;
	GPIO_InitStruct.Mode	=	GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull	=	GPIO_PULLUP;
	
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	//PE2接在KEY2的正极
}

/**
  * @brief     KEY扫描函数
  * @param     无
  * @retval    类型uint8_t：按键按下返回1，没有按下返回0
  * @note	   无
  */
uint8_t KEY_Scan(void)
{
	uint8_t Key_State = 0;
	if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET)
	{
		delay_ms(10);//消抖
		while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET)Key_State = 1;//等待按键松开，防止重复识别
		delay_ms(10);//消抖
	}
	return Key_State;
}
