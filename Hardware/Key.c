/*
 * @Author: csw
 * @Date: 2026-06-09 14:50:35
 * @Description: Key input handling functions for STM32F10x
 * @Copyright: Copyright (c) 2026 1060137882@qq.com, All Rights Reserved.
 * 
 */
#include "stm32f10x.h"                  // Device header
#include "Delay.h"

volatile uint8_t Key_Num;

#define KEY3_LONG_PRESS_MS 1000

static volatile uint16_t Key3_PressMs;
static volatile uint8_t Key3_LongReported;

void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_4 ;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
void Key3_Tick(void)
{
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == 0)
	{
		if(Key3_PressMs < 60000)
		{
			Key3_PressMs++;
		}
		
		if((Key3_PressMs >= KEY3_LONG_PRESS_MS) && (Key3_LongReported == 0))
		{
			Key_Num = 4;
			Key3_LongReported = 1;
		}
	}
	else
	{
		Key3_PressMs = 0;
	}
}

// uint8_t Key_GetNum(void)
// {
// 	uint8_t Temp;
// 	if(Key_Num)
// 	{
// 		Temp=Key_Num;
// 		Key_Num=0;
// 		return Temp;
// 	}
// 	else
// 	{
// 		return 0;
// 	}
// }

uint8_t Key_GetNum(void)
{
    uint8_t Temp;

    __disable_irq();
    Temp = Key_Num;
    Key_Num = 0;
    __enable_irq();

    return Temp;
}

uint8_t Key_GetState(void)
{
	
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
	{
		return 1;
	}
	else if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
	{
		return 2;
	}
	else if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == 0)
	{
		return 3;
	}
	
	else
	{
		return 0;
	}
	
}

void Key_Tick(void)
{
	static uint8_t Count;
	static uint8_t CurrentState,PreState;
	Count++;
	if(Count>=20)
	{
		Count=0;
		PreState=CurrentState;
		CurrentState=Key_GetState();
		if(PreState!=0&&CurrentState==0)
		{
			if((PreState == 3) && (Key3_LongReported != 0))
			{
				Key3_LongReported = 0;
			}
			else
			{
				Key_Num=PreState;
			}
		}
	}
}
