#ifndef __ESP32_H
#define __ESP32_H

#include "stm32f10x.h"

#define ESP32_RX_DMA_SIZE     512
#define ESP32_RX_FRAME_SIZE   512
#define ESP32_CMD_RESP_SIZE   768

void ESP32_Init(void);
void ESP32_SendBytes(const uint8_t *data, uint16_t len);
void ESP32_SendString(const char *str);
uint8_t ESP32_SendCmd(const char *cmd, const char *ack, uint32_t timeout_ms);
uint8_t ESP32_WaitFor(const char *ack, uint32_t timeout_ms);
uint8_t ESP32_ReadFrame(uint8_t *buffer, uint16_t buffer_size, uint16_t *len);
void ESP32_ClearRx(void);
void ESP32_USART2_IRQHandler(void);

#endif
