#include "menu.h"                  // Device header
#include "OLED.h"
#include "MyRTC.h"
#include "Key.h"
#include "LED.h"
#include "SetTime.h"
#include "MPU6050.h"
#include "Delay.h"
#include "math.h"
#include "dino.h"
#include "AD.h"
#include "AHT20.h"
#include "freertos_port.h"
#include "common_math.h"
#include "usart.h"
#include <stdio.h>
uint8_t KeyNum;

#define MENU_ITEM_COUNT 8

static const char *Menu_Names[MENU_ITEM_COUNT] = {
	"BACK",
	"TIMER",
	"LED",
	"MPU6050",
	"GAME",
	"EMOJI",
	"LEVEL",
	"AHT20"
};

static void Menu_ShowIcon(uint8_t index, int16_t x, int16_t y)
{
	if(index < MENU_ITEM_COUNT)
	{
		OLED_ShowImage(x,y,32,32,Menu_Graph[index]);
	}
}

// 外部变量声明
extern QueueHandle_t xKeyQueue;
extern QueueHandle_t xMenuQueue;
extern SemaphoreHandle_t xOLEDMutex;
extern TaskHandle_t xKeyTaskHandle;
extern TaskHandle_t xClockTaskHandle;

// OLED访问保护宏
#ifndef OLED_TAKE
#define OLED_TAKE() xOLEDTake(pdMS_TO_TICKS(100))
#endif

#ifndef OLED_GIVE
#define OLED_GIVE() xOLEDGive()
#endif

void Peripheral_Init(void)
{
	
	MyRTC_Init();
	Key_Init();
	LED_Init();
	
	MPU6050_Init();
	AD_Init();
}

/*----------------------------------首页时钟-------------------------------------*/

uint16_t ADValue;
float VBAT;
int Battery_Capacity;
void Show_Battery(void){
	int sum = 0;
	for(uint16_t i=0;i<3000;i++){
		ADValue=AD_GetValue();
		sum+=ADValue;
	}
	ADValue=sum/3000;
	VBAT=(float)ADValue/4095*3.3;
	Battery_Capacity=(ADValue-3276)*100/819;
	if(Battery_Capacity<0) Battery_Capacity=0;
	
	OLED_ShowNum(85,4,Battery_Capacity,3,OLED_6X8);
	OLED_ShowChar(103,4,'%',OLED_6X8);
	
	if(Battery_Capacity==100) 
	OLED_ShowImage(110,0,16,16,Battery);
	else if(Battery_Capacity>=10&&Battery_Capacity<100)
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea((112+Battery_Capacity/10),5,(10-Battery_Capacity/10),6);
		OLED_ClearArea(85,4,6,8);
	}
	else
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea(112,5,10,6);
		OLED_ClearArea(85,4,12,8);
	}
}

void Show_Clock_UI(void)
{
	Show_Battery();
	MyRTC_ReadTime();
	OLED_Printf(0,0,OLED_6X8,"%d-%d-%d",MyRTC_Time[0],MyRTC_Time[1],MyRTC_Time[2]);
	OLED_Printf(16,16,OLED_12X24,"%02d:%02d:%02d",MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);
	OLED_ShowString(0,48,"菜单",OLED_8X16);
	OLED_ShowString(96,48,"设置",OLED_8X16);
}

//时钟
int clkflag=1;//范围1-2 1表示菜单 2表示设置
int First_Page_Clock(void)
{
	uint8_t key_num;
	
	for(;;)
	{
		// 检查按键
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				clkflag--;
				if(clkflag<=0)clkflag=2;
			}
			else if(key_num==2)//右
			{
				clkflag++;
				if(clkflag>=3)clkflag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				return clkflag;
			}
			else if(key_num==4)
			{
				GPIO_ResetBits(GPIOB, GPIO_Pin_13);
				GPIO_SetBits(GPIOB, GPIO_Pin_12);
			}
		}
		
		if(OLED_TAKE())
		{
			switch(clkflag)
			{
				case 1:
					Show_Clock_UI();
					OLED_ReverseArea(0,48,32,16);
					OLED_Update();
					break;
				
				case 2:
					Show_Clock_UI();
					OLED_ReverseArea(96,48,32,16);
					OLED_Update();
					break;
			}
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*-----------------------------------------设置--------------------------------*/

void Show_SettingPage_UI(void){

	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"日期时间设置",OLED_8X16);

}

int setflag=1;//范围1-2 1为返回 2为时间

int SettingPage(void){
	uint8_t key_num;
	
	for(;;)
	{
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				setflag--;
				if(setflag<=0)setflag=2;
			}
			else if(key_num==2)//右
			{
				setflag++;
				if(setflag>=3)setflag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				if(setflag==1){return 0;}
				else if(setflag==2){SetTime();}
			}
		}
		
		if(OLED_TAKE())
		{
			OLED_Clear();
			Show_SettingPage_UI();
			switch(setflag)
			{
				case 1:
					OLED_ReverseArea(0,0,16,16);
					break;
				
				case 2:
					OLED_ReverseArea(0,16,96,16);
					break;
			}
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*------------------------------------菜单-----------------------------*/

uint8_t pre_selection;//当前选中项索引，从0开始
uint8_t target_selection;//目标选中项索引，从0开始
uint8_t x_pre=48;//当前选中x坐标
uint8_t Speed=16;//移动像素
uint8_t move_flag;//移动标志位1为开始移动0为停止

void Menu_Animation(void){

	OLED_Clear();
	OLED_ShowImage(42,10,44,44,Frame);
	//图标向右移动
	if(pre_selection<target_selection){
	
		x_pre-=Speed;
		if(x_pre==0){
			pre_selection++;//指向下一图标
			move_flag=0;
			x_pre=48;
		}
	
	}
	//图标向左移动
	if(pre_selection>target_selection){
	
		x_pre+=Speed;
		if(x_pre==96){
			
			pre_selection--;//指向上一图标
			move_flag=0;
			x_pre=48;
		}
	
	}
	if(pre_selection>=1){//移动前pre_selection前一图标
		Menu_ShowIcon(pre_selection-1,x_pre-48,16);
	}
	if(pre_selection>=2){//移动前pre_selection2图标
		Menu_ShowIcon(pre_selection-2,x_pre-96,16);
	}
	Menu_ShowIcon(pre_selection,x_pre,16);
	Menu_ShowIcon(pre_selection+1,x_pre+48,16);
	Menu_ShowIcon(pre_selection+2,x_pre+96,16);
	if(target_selection < MENU_ITEM_COUNT)
	{
		OLED_ShowString(44,0,(char *)Menu_Names[target_selection],OLED_6X8);
	}
	
	OLED_Update();
}

void Set_Selection(uint8_t move_flag,uint8_t Pre_Selection,uint8_t Target_selection){

	if(move_flag==1){
		pre_selection=Pre_Selection;
		target_selection=Target_selection;
	}
	Menu_Animation();
}


void MenuToFunction(void){
	
	for(uint8_t y=16;y<=64;y+=16){
		OLED_Clear();
		if(pre_selection>=1){//移动前pre_selection前一图标
			Menu_ShowIcon(pre_selection-1,x_pre-48,y);
		}

		Menu_ShowIcon(pre_selection,x_pre,y);
		Menu_ShowIcon(pre_selection+1,x_pre+48,y);
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(5));
	}	
}

//秒表
uint8_t stop_watch_flag=1;//位置
uint8_t stop_watch_start;//是否开始标志位
uint8_t hour,min,sec=0;

void Show_Stop_WatchUI(void){

	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(32,20,OLED_8X16,"%02d:%02d:%02d",hour,min,sec);
	OLED_ShowString(8,40,"开始",OLED_8X16);
	OLED_ShowString(48,40,"停止",OLED_8X16);
	OLED_ShowString(88,40,"清除",OLED_8X16);
}
uint16_t timer=0;
void Watch_Start(void){
	timer++;
	if(timer==1000){
		timer=0;
		sec++;
		if(sec>=60){
			sec=0;
			min++;
			if(min>=60){
				min=0;
				hour++;
				if(hour>=100){
					hour=0;
				}
			}
		}
	}
}

int Stop_Watch(void){
	uint8_t key_num;
	
	for(;;)
	{
		if(xQueueReceive(xMenuQueue, &stop_watch_flag, pdMS_TO_TICKS(100)) == pdPASS)
		{
			if(stop_watch_flag==1){return 0;}
			if(stop_watch_flag==2){stop_watch_start=1;}
			if(stop_watch_flag==3){stop_watch_start=0;}
			if(stop_watch_flag==4){stop_watch_start=0;hour=min=sec=0;}
		}
		
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				stop_watch_flag--;
				if(stop_watch_flag<=0)stop_watch_flag=4;
			}
			else if(key_num==2)//右
			{
				stop_watch_flag++;
				if(stop_watch_flag>=5)stop_watch_flag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				xQueueSend(xMenuQueue, &stop_watch_flag, pdMS_TO_TICKS(100));
			}
		}
		
		if(OLED_TAKE())
		{
			Show_Stop_WatchUI();
			switch(stop_watch_flag)
			{
				case 1:
					OLED_ReverseArea(0,0,16,16);
					break;
				
				case 2:
					OLED_ReverseArea(8,40,32,16);
					break;
				case 3:
					OLED_ReverseArea(48,40,32,16);
					break;
				case 4:
					OLED_ReverseArea(88,40,32,16);
					break;
			}
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

/*------------------------------------LED--------------------------------*/

void Show_Led_UI(void){

	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(20,20,"OFF",OLED_12X24);
	OLED_ShowString(72,20,"ON",OLED_12X24);
	
}

uint8_t Led_flag=1;
int Led(void){
	uint8_t key_num;
	
	for(;;)
	{
		if(xQueueReceive(xMenuQueue, &Led_flag, pdMS_TO_TICKS(100)) == pdPASS)
		{
			if(Led_flag==1){return 0;}
			if(Led_flag==2){LED1_OFF();}
			if(Led_flag==3){LED1_ON();}
		}
		
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				Led_flag--;
				if(Led_flag<=0)Led_flag=3;
			}
			else if(key_num==2)//右
			{
				Led_flag++;
				if(Led_flag>=4)Led_flag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				xQueueSend(xMenuQueue, &Led_flag, pdMS_TO_TICKS(100));
			}
		}
		
		if(OLED_TAKE())
		{
			Show_Led_UI();
			switch(Led_flag)
			{
				case 1:
					OLED_ReverseArea(0,0,16,16);
					break;
				
				case 2:
					OLED_ReverseArea(20,20,36,24);
					break;
				
				case 3:
					OLED_ReverseArea(72,20,24,24);
					break;
			}
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

//menu_flag 菜单选择 menu_flag_temp 菜单确定值
//确定后
uint8_t menu_flag=1;
int Menu(void){
	uint8_t key_num;
	move_flag=1;
	uint8_t DirectFlag=2;//为1 左移一次，2右移一次
	for(;;)
	{
		uint8_t menu_flag_temp=0;

		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				DirectFlag=1;
				move_flag=1;
				menu_flag--;
				if(menu_flag<=0)menu_flag=MENU_ITEM_COUNT;
			}
			else if(key_num==2)//右
			{
				DirectFlag=2;
				move_flag=1;
				menu_flag++;
				if(menu_flag>MENU_ITEM_COUNT)menu_flag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				menu_flag_temp=menu_flag;
			}
		}

		if(menu_flag_temp==1){return 0;}
		else if(menu_flag_temp==2){MenuToFunction();Stop_Watch();}
		else if(menu_flag_temp==3){MenuToFunction();Led();}
		else if(menu_flag_temp==4){MenuToFunction();MPU_6050();}
		else if(menu_flag_temp==5){MenuToFunction();Game();}
		else if(menu_flag_temp==6){MenuToFunction();Emoji();}
		else if(menu_flag_temp==7){MenuToFunction();Gradienter();}
		else if(menu_flag_temp==8){MenuToFunction();AHT20_Page();}
		
		if(menu_flag==1){
			if(DirectFlag==1)Set_Selection(move_flag,1,0);
			else if(DirectFlag==2)Set_Selection(move_flag,0,0);
		} 
		else{
			if(DirectFlag==1)Set_Selection(move_flag,menu_flag,menu_flag-1);
			else if(DirectFlag==2)Set_Selection(move_flag,menu_flag-2,menu_flag-1);
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/*----------------------------------MPU6050------------------------------------*/

int16_t ax,ay,az,gx,gy,gz;
float roll_a,pitch_a;
float roll_g,pitch_g,yaw_g;
float Roll,Pitch,Yaw;
// float Delta_t=0.005;
float a=0.9;
// double pi=3.1415927;

// #define GYRO_SCALE  16.4f   // 2000/s 满量程 16.4 LSB/s

// void MPU6050_Calculation(void) {
//     Delay_ms(5);
//     MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

//     // 角速度/秒 角度
//     roll_g  = Roll  + ((float)gx / GYRO_SCALE) * Delta_t;
//     pitch_g = Pitch + ((float)gy / GYRO_SCALE) * Delta_t;
//     yaw_g   = Yaw   + ((float)gz / GYRO_SCALE) * Delta_t;

//     roll_a  = atan2(ay, az) * 180 / pi;
//     pitch_a = atan2((-1) * ax, az) * 180 / pi;

//     Roll  = a * roll_g  + (1 - a) * roll_a;
//     Pitch = a * pitch_g + (1 - a) * pitch_a;
//     Yaw   = a*roll_g;
// }


#define GYRO_SCALE  16.4f

void MPU6050_Calculation(void)
{
    static TickType_t last_tick = 0;
    TickType_t now;
    float dt;

    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

    now = xTaskGetTickCount();

    if(last_tick == 0)
    {
        last_tick = now;
        return;
    }

    dt = (now - last_tick) * portTICK_PERIOD_MS / 1000.0f;
    last_tick = now;

    roll_g  = Roll  + ((float)gx / GYRO_SCALE) * dt;
    pitch_g = Pitch + ((float)gy / GYRO_SCALE) * dt;
    yaw_g   = Yaw   + ((float)gz / GYRO_SCALE) * dt;

    roll_a  = atan2f((float)ay, (float)az) * 180.0f / PI;
    pitch_a = atan2f(-(float)ax,
                     sqrtf((float)ay * ay + (float)az * az)) * 180.0f / PI;

    Roll  = a * roll_g  + (1.0f - a) * roll_a;
    Pitch = a * pitch_g + (1.0f - a) * pitch_a;
    Yaw   = a * yaw_g;
}

void Show_MPU6050_UI(void){

	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(0,16,OLED_8X16,"Roll: %.2f",Roll);
	OLED_Printf(0,32,OLED_8X16,"Pitch:%.2f",Pitch);
	OLED_Printf(0,48,OLED_8X16,"Yaw:  %.2f",Yaw);
}

int MPU_6050(void){
	uint8_t key_num;
	
	for(;;){
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==3){
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				return 0;
			}
		}
		
		if(OLED_TAKE())
		{
			Show_MPU6050_UI();
			OLED_ReverseArea(0,0,16,16);
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(25));
	}
}
/*----------------------------------AHT20------------------------------------*/
void Show_AHT20_UI(void)
{
	int16_t temp;
	uint16_t humi;
	int16_t temp_abs;

	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(32,0,"AHT20",OLED_8X16);

	if(AHT20_DataValid == 0)
	{
		OLED_ShowString(0,24,"AHT20 Error",OLED_8X16);
		OLED_ShowString(0,44,"Check I2C2",OLED_8X16);
		return;
	}

	temp = AHT20_TemperatureX10;
	humi = AHT20_HumidityX10;

	if(temp < 0)
	{
		temp_abs = -temp;
		OLED_Printf(0,20,OLED_8X16,"Temp:-%d.%dC",temp_abs / 10,temp_abs % 10);
	}
	else
	{
		OLED_Printf(0,20,OLED_8X16,"Temp: %d.%dC",temp / 10,temp % 10);
	}

	OLED_Printf(0,40,OLED_8X16,"Humi: %d.%d%%",humi / 10,humi % 10);
}

int AHT20_Page(void)
{
	uint8_t key_num;

	for(;;)
	{
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==3)
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				return 0;
			}
		}

		if(OLED_TAKE())
		{
			OLED_Clear();
			Show_AHT20_UI();
			OLED_ReverseArea(0,0,16,16);
			OLED_Update();
			OLED_GIVE();
		}

		vTaskDelay(pdMS_TO_TICKS(200));
	}
}
/*------------------------------------游戏--------------------------------*/
void Show_Game_UI(void){

	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"谷歌小恐龙",OLED_8X16);
}

uint8_t game_flag=1;
int Game(void){
	uint8_t key_num;
	
	for(;;)
	{
		if(xQueueReceive(xMenuQueue, &game_flag, pdMS_TO_TICKS(100)) == pdPASS)
		{
			if(game_flag==1){return 0;}
			else if(game_flag==2){DinoGame_Pos_Init();DinoGame_Animation();}
		}
		
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==1)//左
			{
				game_flag--;
				if(game_flag<=0)game_flag=2;
			}
			else if(key_num==2)//右
			{
				game_flag++;
				if(game_flag>=3)game_flag=1;
			}
			else if(key_num==3)//确定
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				xQueueSend(xMenuQueue, &game_flag, pdMS_TO_TICKS(100));
			}
		}
		
		if(OLED_TAKE())
		{
			Show_Game_UI();
			switch(game_flag)
			{
				case 1:
					OLED_ReverseArea(0,0,16,16);
					break;
				
				case 2:
					OLED_ReverseArea(0,16,80,16);
					break;
			}
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(50));
	}

}

/*----------------------------------表情-------------------------------------*/

static int Emoji_ReturnKeyPressed(void)
{
	uint8_t key_num;
	if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
	{
		return key_num == 3;
	}
	return 0;
}

static int Emoji_DelayWithReturn(uint16_t delay_ms)
{
	uint16_t elapsed = 0;
	while(elapsed < delay_ms)
	{
		if(Emoji_ReturnKeyPressed()) return 1;
		vTaskDelay(pdMS_TO_TICKS(20));
		elapsed += 20;
	}
	return 0;
}

static int Show_Emoji_UI(void)
{
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);
		OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);
		OLED_DrawEllipse(40,32,6,6-i,1);
		OLED_DrawEllipse(88,32,6,6-i,1);
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		if(Emoji_DelayWithReturn(40)) return 1;
	}
	
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);
		OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);
		OLED_DrawEllipse(40,32,6,4+i,1);
		OLED_DrawEllipse(88,32,6,4+i,1);
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		if(Emoji_DelayWithReturn(40)) return 1;
	}
	
	return Emoji_DelayWithReturn(200);
}

int Emoji(void){
	while(1)
	{
		if(Emoji_ReturnKeyPressed())
		{
			if(OLED_TAKE())
			{
				OLED_Clear();
				OLED_Update();
				OLED_GIVE();
			}
			return 0;
		}
		
		if(OLED_TAKE())
		{
			if(Show_Emoji_UI())
			{
				OLED_Clear();
				OLED_Update();
				OLED_GIVE();
				return 0;
			}
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}
/*----------------------------------水平仪-------------------------------------*/

void Show_Gradienter_UI(void)
{
	OLED_DrawCircle(64,32,30,0);
	OLED_DrawCircle(64-Roll,32+Pitch,4,1);
}

int Gradienter(void){
	uint8_t key_num;
	
	while(1)
	{
		if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
		{
			if(key_num==3)
			{
				if(OLED_TAKE())
				{
					OLED_Clear();
					OLED_Update();
					OLED_GIVE();
				}
				return 0;
			}
		}
		
		if(OLED_TAKE())
		{
			OLED_Clear();
			Show_Gradienter_UI();
			OLED_Update();
			OLED_GIVE();
		}
		
		vTaskDelay(pdMS_TO_TICKS(25));
	}
}
