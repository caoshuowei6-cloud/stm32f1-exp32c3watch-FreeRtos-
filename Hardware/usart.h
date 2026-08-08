#ifndef __USART_H
#define __USART_H
#include "stm32f10x.h"                  // Device header
#include "stdio.h"

//extern uint8_t Serial_TxPacket[];//声明全局数组
extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number);
void Serial_Printf(char *format,...);
void memset_clear(char *Serial_RxPacket);

//void Serial_SendPacket(void);

#endif
