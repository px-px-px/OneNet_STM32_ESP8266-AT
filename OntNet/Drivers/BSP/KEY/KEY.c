#include "./SYSTEM/delay/delay.h"
#include "./BSP/KEY/KEY.h"

/**
  * @brief     KEY��ʼ������
  * @param     ��
  * @retval    ��
  * @note	   ��
  */
void KEY_Init(void)
{
	/* ����ʱ�� */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	
	/* ����GPIO */
	GPIO_InitTypeDef GPIO_InitStruct;	//����GPIO��ʼ���ṹ��
	
	GPIO_InitStruct.Pin		=	GPIO_PIN_2;
	GPIO_InitStruct.Mode	=	GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull	=	GPIO_PULLUP;
	
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	//PE2����KEY2������
}

/**
  * @brief     KEYɨ�躯��
  * @param     ��
  * @retval    ����uint8_t���������·���1��û�а��·���0
  * @note	   ��
  */
uint8_t KEY_Scan(void)
{
	uint8_t Key_State = 0;
	if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET)
	{
		delay_ms(10);//����
		while(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET)Key_State = 1;//�ȴ������ɿ�����ֹ�ظ�ʶ��
		delay_ms(10);//����
	}
	return Key_State;
}
