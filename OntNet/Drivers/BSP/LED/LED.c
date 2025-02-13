#include "./BSP/LED/LED.h"

uint8_t LED;

/**
  * @brief     LED��ʼ������
  * @param     ��
  * @retval    ��
  * @note	  ��
  */
void LED_Init(void)
{
	/* ����ʱ�� */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	/* ����GPIO */
	GPIO_InitTypeDef GPIO_InitStruct;	//����GPIO��ʼ���ṹ��
	
	GPIO_InitStruct.Pin		=	GPIO_PIN_5;
	GPIO_InitStruct.Mode	=	GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed	=	GPIO_SPEED_FREQ_LOW;
	
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/* ���ó�ʼֵ */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);	//PB5����LED(DS0)�ĸ���
}

//LED��ת
void LED_Overturn(void)
{
	//Ӳ����ת
	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
	//����ͬ��
	if(LED)
	{
		LED = 0;
	}
	else
	{
		LED = 1;
	}
}

