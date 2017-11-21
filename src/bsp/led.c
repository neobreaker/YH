#include "stm32f10x.h"
#include "led.h"

void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void led_on(uint32_t n)
{
	switch (n)
	{
		case LED_0:
			GPIO_SetBits(GPIOC, GPIO_Pin_6);
			break;
		case LED_1:
			GPIO_SetBits(GPIOC, GPIO_Pin_7);
			break;
		case LED_2:
			GPIO_SetBits(GPIOC, GPIO_Pin_8);
			break;
		default:
			break;
	}
}

void led_off(uint32_t n)
{
	switch (n)
	{
		case LED_0:
			GPIO_ResetBits(GPIOC, GPIO_Pin_6);
			break;
		case LED_1:                            
			GPIO_ResetBits(GPIOC, GPIO_Pin_7);
			break;
		case LED_2:
			GPIO_ResetBits(GPIOC, GPIO_Pin_8);
			break;
		default:
			break;
	}
}
