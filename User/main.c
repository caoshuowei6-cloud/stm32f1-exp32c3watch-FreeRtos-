/*
 * @Author: csw
 * @Date: 2026-06-09 14:50:38
 * @Description:  
 * @Copyright: Copyright (c) 2026 1060137882@qq.com, All Rights Reserved.
 * 
 */
#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "menu.h"
#include "Timer.h"
#include "Key.h"
#include "LED.h"
#include "dino.h"
#include "freertos_port.h"
#include "queue.h"
#include "usart.h"
extern void xPortSysTickHandler(void);
/**
  * 函数功能：主函数入口
  * 返回值：无
  * 说明：坐标原点为(0, 0)
  * X为行号，取值范围0~127
  * Y为列号，取值范围0~63
  
       0             X           127 
      .------------------------------->
    0 |
      |
      |
      |
  Y |
      |
      |
      |
   63 |
      v
  
*/

void vAssertCalled(const char *file,int line){

}

int main(void)
{
	Serial_Init();
	printf("usart init\r\n");
	/*OLED初始化*/
	OLED_Init();
	OLED_Clear();
	Peripheral_Init();
	Timer_Init();
	
	/* 初始化FreeRTOS任务 */
	// printf("init tasks\n");
	vInitTasks();
	
	/* 应该不会执行到这里 */
	while (1)
	{
	}
}



void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key3_Tick();
		Key_Tick();
		if(stop_watch_start==1)
		Watch_Start();
		Dino_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

void SysTick_Handler(void)
{
	if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
		xPortSysTickHandler();
	}
}
