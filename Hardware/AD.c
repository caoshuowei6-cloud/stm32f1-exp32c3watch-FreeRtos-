#include "stm32f10x.h"                  // Device header

void AD_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//最大频率14MHz,对72MHz进行6分频
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	//配置gpio模式为模拟输入
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	//选择规则组的输入通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);
	
	//初始化ADC
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode=ADC_Mode_Independent;
	ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_ContinuousConvMode=ENABLE;//是否连续模式
	ADC_InitStructure.ADC_ScanConvMode=DISABLE;//
	ADC_InitStructure.ADC_NbrOfChannel=1;//通道数目
	
	ADC_Init(ADC1,&ADC_InitStructure);
	
	ADC_Cmd(ADC1,ENABLE);
	//校准
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1)==SET);
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)==SET);
	
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
}
uint16_t AD_GetValue(void){
	//触发转换
	
//	//等待ADC总转换时间
//	while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
	//读取
	return ADC_GetConversionValue(ADC1);
}
