#include "usart.h"
#include "stdarg.h"
#include "string.h"

//extern uint8_t Serial_TxPacket[];在头文件声明
//uint8_t Serial_TxPacket[4];
char Serial_RxPacket[110];
uint8_t pp;
uint8_t Serial_RxFlag;
uint8_t pSerial_RxPacket;
void Serial_Init(void){

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	
	
	
	USART_InitStructure.USART_BaudRate=115200;
	USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
	USART_InitStructure.USART_Parity=USART_Parity_No;
	USART_InitStructure.USART_StopBits=USART_StopBits_1;
	USART_InitStructure.USART_WordLength=USART_WordLength_8b;
	
	
	USART_Init(USART1,&USART_InitStructure);
	//以下是中断
//	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
//	
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	
//	NVIC_InitTypeDef NVIC_InitStructure;
//	
//	NVIC_InitStructure.NVIC_IRQChannel=USART1_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority=1;
//	
//	NVIC_Init(&NVIC_InitStructure); 
	
	USART_Cmd(USART1,ENABLE);
	
}

void Serial_SendByte(uint8_t Byte){

	
	USART_SendData(USART1,Byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);
	
}

//void Serial_SendArray(uint8_t *Array,uint16_t Length){

//	uint16_t i;
//	for(i=0;i<Length;i++){
//		
//		Serial_SendByte(Array[i]);
//	}
//}


void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Array[i]);
	}
}


void Serial_SendString(char *String){
	uint8_t i;
	for(i=0;String[i]!='\0';i++){
	
		Serial_SendByte(String[i]);
	
	}

}

uint32_t Serial_Pow(uint32_t x,uint32_t y){
	uint32_t result=1;
	while(y--){
		result*=x;
		
	}
	return result;
}


void Serial_SendNumber(uint32_t Number){
	uint32_t count=0;
	uint32_t Number_temp=Number;
	while(Number_temp!=0){
		count++;
		Number_temp/=10;
	}
	uint8_t i;
	for(i=count;i>0;i--){
		Serial_SendByte((Number/Serial_Pow(10,i-1))%10+'0');
	}
}

int fputc(int ch,FILE *f){

	Serial_SendByte(ch);
	return ch;
}

void Serial_Printf(char *format,...){

	char String[100];
	va_list arg;
	va_start(arg,format);
	vsnprintf(String,sizeof(String),format,arg); 
	va_end(arg);
	Serial_SendString(String);
}



//void Serial_SendPacket(void){
//	Serial_SendByte(0xFF);
//	Serial_SendArray(Serial_TxPacket,4);
//	Serial_SendByte(0xFE);
//}

void memset_clear(char *Serial_RxPacket)
{
		memset(Serial_RxPacket, 0, pSerial_RxPacket);
		pSerial_RxPacket=0;
}


//void USART1_IRQHandler(void){
//	static  uint8_t RxState=0;
//	if(USART_GetITStatus(USART1,USART_IT_RXNE)==SET){
//		uint8_t RxData=USART_ReceiveData(USART1);
//	
//		if(RxData == '@' && Serial_RxFlag == 0){
//			RxState = 1;                // 强制回到接收状态，解决卡死
//			memset_clear(Serial_RxPacket); // 清空数组+复位指针
//     }
//		else if(RxState==1){
//			if(RxData=='\r'){
//				RxState=2;
//			}
//			else{
//				Serial_RxPacket[pSerial_RxPacket]=RxData;
//				pSerial_RxPacket++;			
//			}
//		}
//		else if(RxState==2){
//			if(RxData=='\n'){
//				Serial_RxPacket[pSerial_RxPacket]='\0';
//				Serial_RxFlag=1;
//			}
//		}
//		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
//		
//	}
//}
