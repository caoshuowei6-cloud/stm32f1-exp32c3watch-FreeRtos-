#ifndef __MENU_H
#define __MENU_H
#include "stm32f10x.h"                  // Device header

void Peripheral_Init(void);
void Show_Clock_UI(void);
int First_Page_Clock(void);
int SettingPage(void);
int Menu(void);
void Watch_Start(void);
void MPU6050_Calculation(void);
int Stop_Watch(void);
int MPU_6050(void);
int AHT20_Page(void);
int Game(void);
int Emoji(void);
int Gradienter(void);
extern uint8_t stop_watch_start;
extern float Roll;
extern float Pitch;
extern float Yaw;

#endif
