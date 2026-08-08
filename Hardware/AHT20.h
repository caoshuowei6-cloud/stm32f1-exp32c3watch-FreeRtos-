#ifndef __AHT20_H
#define __AHT20_H

#include "stm32f10x.h"

#define AHT20_OK        0
#define AHT20_ERROR     1

void AHT20_Init(void);
uint8_t AHT20_ReadInt(int16_t *temperature_x10, uint16_t *humidity_x10);
uint8_t AHT20_Update(void);
void AHT20_PrintSerial(void);

extern volatile int16_t AHT20_TemperatureX10;
extern volatile uint16_t AHT20_HumidityX10;
extern volatile uint8_t AHT20_DataValid;

#endif
