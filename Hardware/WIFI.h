#ifndef __WIFI_H
#define __WIFI_H

#include "stm32f10x.h"
#include "ESP32.h"

#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID "csw"
#endif

#ifndef WIFI_STA_PASS
#define WIFI_STA_PASS "87654321"
#endif

typedef enum
{
    WIFI_MODE_STA = 1,
    WIFI_MODE_AP = 2
} WIFI_MODE;

void WIFI_Init(WIFI_MODE mode);
uint8_t WIFI_TestConnectSTA(void);
uint8_t WIFI_TCP_ServerStart(uint16_t port);
uint8_t WIFI_TCP_SendData(uint8_t id, const uint8_t *data, uint16_t len);
uint8_t WIFI_TCP_ReadData(uint8_t rxBuff[], uint16_t *rxLen, uint8_t *id, uint8_t *ip, uint16_t *port);
uint8_t WIFI_TCP_GetClientId(uint8_t *id);
void WIFI_TCP_Poll(void);

#endif
