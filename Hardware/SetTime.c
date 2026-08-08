#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Key.h"
#include "MyRTC.h"
#include "freertos_port.h"

extern uint8_t KeyNum;

#define SETTIME_LOOP_DELAY_MS 20

static uint8_t SetTime_GetKey(void)
{
	uint8_t key_num = 0;
	if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
	{
		KeyNum = key_num;
		return key_num;
	}
	KeyNum = 0;
	return 0;
}

static void SetTime_Delay(void)
{
	vTaskDelay(pdMS_TO_TICKS(SETTIME_LOOP_DELAY_MS));
}

void Show_SetTime_FirstUI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(0,16,OLED_8X16,"年:%4d",MyRTC_Time[0]);
	OLED_Printf(0,32,OLED_8X16,"月:%2d",MyRTC_Time[1]);
	OLED_Printf(0,48,OLED_8X16,"日:%2d",MyRTC_Time[2]);
}

void Show_SetTime_SecondUI(void)
{
	OLED_Printf(0,0,OLED_8X16,"时:%2d",MyRTC_Time[3]);
	OLED_Printf(0,16,OLED_8X16,"分:%2d",MyRTC_Time[4]);
	OLED_Printf(0,32,OLED_8X16,"秒:%2d",MyRTC_Time[5]);
}

static void Normalize_RTC_Time(uint8_t i)
{
	switch(i)
	{
		case 1:
			if(MyRTC_Time[1] > 12) MyRTC_Time[1] = 1;
			else if(MyRTC_Time[1] <= 0) MyRTC_Time[1] = 12;
			break;
		case 2:
			if(MyRTC_Time[2] > 31) MyRTC_Time[2] = 1;
			else if(MyRTC_Time[2] <= 0) MyRTC_Time[2] = 31;
			break;
		case 3:
			if(MyRTC_Time[3] > 23) MyRTC_Time[3] = 0;
			else if(MyRTC_Time[3] < 0) MyRTC_Time[3] = 23;
			break;
		case 4:
			if(MyRTC_Time[4] >= 60) MyRTC_Time[4] = 0;
			else if(MyRTC_Time[4] < 0) MyRTC_Time[4] = 59;
			break;
		case 5:
			if(MyRTC_Time[5] >= 60) MyRTC_Time[5] = 0;
			else if(MyRTC_Time[5] < 0) MyRTC_Time[5] = 59;
			break;
		default:
			break;
	}
}

void Change_RTC_Time(uint8_t i,uint8_t flag)
{
	if(flag==1) MyRTC_Time[i]++;
	else MyRTC_Time[i]--;
	Normalize_RTC_Time(i);
	MyRTC_SetTime();
}

static void Draw_SetTime_FirstSelection(uint8_t selection)
{
	if(OLED_TAKE())
	{
		OLED_Clear();
		Show_SetTime_FirstUI();
		switch(selection)
		{
			case 1:
				OLED_ReverseArea(0,0,16,16);
				break;
			case 2:
				OLED_ReverseArea(0,16,16,16);
				break;
			case 3:
				OLED_ReverseArea(0,32,16,16);
				break;
			case 4:
				OLED_ReverseArea(0,48,16,16);
				break;
			default:
				break;
		}
		OLED_Update();
		OLED_GIVE();
	}
}

static void Draw_SetTime_SecondSelection(uint8_t selection)
{
	if(OLED_TAKE())
	{
		OLED_Clear();
		Show_SetTime_SecondUI();
		switch(selection)
		{
			case 5:
				OLED_ReverseArea(0,0,16,16);
				break;
			case 6:
				OLED_ReverseArea(0,16,16,16);
				break;
			case 7:
				OLED_ReverseArea(0,32,16,16);
				break;
			default:
				break;
		}
		OLED_Update();
		OLED_GIVE();
	}
}

static void Draw_SetTime_FirstValue(uint8_t selection)
{
	if(OLED_TAKE())
	{
		OLED_Clear();
		Show_SetTime_FirstUI();
		switch(selection)
		{
			case 2:
				OLED_ReverseArea(24,16,32,16);
				break;
			case 3:
				OLED_ReverseArea(24,32,16,16);
				break;
			case 4:
				OLED_ReverseArea(24,48,16,16);
				break;
			default:
				break;
		}
		OLED_Update();
		OLED_GIVE();
	}
}

static void Draw_SetTime_SecondValue(uint8_t selection)
{
	if(OLED_TAKE())
	{
		OLED_Clear();
		Show_SetTime_SecondUI();
		switch(selection)
		{
			case 5:
				OLED_ReverseArea(24,0,16,16);
				break;
			case 6:
				OLED_ReverseArea(24,16,16,16);
				break;
			case 7:
				OLED_ReverseArea(24,32,16,16);
				break;
			default:
				break;
		}
		OLED_Update();
		OLED_GIVE();
	}
}

static int SetTime_AdjustValue(uint8_t rtc_index, uint8_t selection)
{
	uint8_t key_num;

	for(;;)
	{
		key_num = SetTime_GetKey();
		if(key_num==1)
		{
			Change_RTC_Time(rtc_index,1);
		}
		else if(key_num==2)
		{
			Change_RTC_Time(rtc_index,0);
		}
		else if(key_num==3)
		{
			return 0;
		}

		if(selection <= 4) Draw_SetTime_FirstValue(selection);
		else Draw_SetTime_SecondValue(selection);
		SetTime_Delay();
	}
}

int SetYear(void)
{
	return SetTime_AdjustValue(0,2);
}

int SetMonth(void)
{
	return SetTime_AdjustValue(1,3);
}

int SetDay(void)
{
	return SetTime_AdjustValue(2,4);
}

int SetHour(void)
{
	return SetTime_AdjustValue(3,5);
}

int SetMin(void)
{
	return SetTime_AdjustValue(4,6);
}

int SetSec(void)
{
	return SetTime_AdjustValue(5,7);
}

int set_time_flag=1;
int SetTime(void)
{
	uint8_t key_num;
	uint8_t set_time_flag_temp;

	set_time_flag = 1;

	while(1)
	{
		set_time_flag_temp = 0;
		key_num = SetTime_GetKey();
		if(key_num==1)
		{
			set_time_flag--;
			if(set_time_flag<=0)set_time_flag=7;
		}
		else if(key_num==2)
		{
			set_time_flag++;
			if(set_time_flag>=8)set_time_flag=1;
		}
		else if(key_num==3)
		{
			set_time_flag_temp=set_time_flag;
		}

		if(set_time_flag_temp==1)
		{
			if(OLED_TAKE())
			{
				OLED_Clear();
				OLED_Update();
				OLED_GIVE();
			}
			return 0;
		}
		else if(set_time_flag_temp==2){SetYear();}
		else if(set_time_flag_temp==3){SetMonth();}
		else if(set_time_flag_temp==4){SetDay();}
		else if(set_time_flag_temp==5){SetHour();}
		else if(set_time_flag_temp==6){SetMin();}
		else if(set_time_flag_temp==7){SetSec();}

		if(set_time_flag <= 4) Draw_SetTime_FirstSelection(set_time_flag);
		else Draw_SetTime_SecondSelection(set_time_flag);
		SetTime_Delay();
	}
}
