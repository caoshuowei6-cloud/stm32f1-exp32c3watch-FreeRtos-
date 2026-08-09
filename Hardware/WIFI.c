#include "WIFI.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static uint8_t WIFI_ClientId;
static uint8_t WIFI_ClientConnected;

static void WIFI_TCP_UpdateClientState(uint8_t *buffer, uint16_t len)
{
    uint8_t parsed_id;

    if(buffer == NULL || len == 0)
    {
        return;
    }

    if(sscanf((char *)buffer, "%hhu,CONNECT", &parsed_id) == 1)
    {
        WIFI_ClientId = parsed_id;
        WIFI_ClientConnected = 1;
        printf("WIFI client %d connected\r\n", WIFI_ClientId);
        return;
    }

    if(sscanf((char *)buffer, "+LINK_CONN:%hhu", &parsed_id) == 1)
    {
        WIFI_ClientId = parsed_id;
        WIFI_ClientConnected = 1;
        printf("WIFI client %d connected\r\n", WIFI_ClientId);
        return;
    }

    if(sscanf((char *)buffer, "%hhu,DISCONNECT", &parsed_id) == 1)
    {
        if(parsed_id == WIFI_ClientId)
        {
            WIFI_ClientConnected = 0;
        }
        printf("WIFI client %d disconnected\r\n", parsed_id);
        return;
    }

    if(strstr((char *)buffer, "CLOSED") != NULL ||
       strstr((char *)buffer, "LINK CLOSED") != NULL)
    {
        WIFI_ClientConnected = 0;
        printf("WIFI client closed\r\n");
    }
}

static uint8_t WIFI_STA_Mode(void)
{
    char cmd[96];
    uint8_t ok = 1;

    ok &= ESP32_SendCmd("AT+CWMODE=1\r\n", "OK", 2000);
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_STA_SSID, WIFI_STA_PASS);
    ok &= ESP32_SendCmd(cmd, "OK", 20000);
    ok &= ESP32_SendCmd("AT+CIPSTA?\r\n", "OK", 2000);

    return ok;
}

static uint8_t WIFI_AP_Mode(void)
{
    uint8_t ok = 1;

    ok &= ESP32_SendCmd("AT+CWMODE=2\r\n", "OK", 2000);
    ok &= ESP32_SendCmd("AT+CWSAP=\"ESP32_softAP\",\"1234567890\",5,3\r\n", "OK", 3000);
    ok &= ESP32_SendCmd("AT+CIPAP=\"192.168.8.1\"\r\n", "OK", 2000);
    ok &= ESP32_SendCmd("AT+CIPAP?\r\n", "OK", 2000);

    return ok;
}

void WIFI_Init(WIFI_MODE mode)
{
    uint8_t ok;

    WIFI_ClientId = 0;
    WIFI_ClientConnected = 0;

    ESP32_Init();
    printf("ESP32 USART2 DMA+IDLE init done\r\n");

    if(!ESP32_SendCmd("AT\r\n", "OK", 1000))
    {
        printf("ESP32 AT no response, try reset\r\n");
    }

    ESP32_SendCmd("AT+RST\r\n", "ready", 5000);
    ESP32_SendCmd("ATE0\r\n", "OK", 1000);

    if(mode == WIFI_MODE_AP)
    {
        ok = WIFI_AP_Mode();
    }
    else
    {
        ok = WIFI_STA_Mode();
    }

    printf("WIFI_Init %s\r\n", ok ? "ok" : "failed");
}

uint8_t WIFI_TestConnectSTA(void)
{
    WIFI_Init(WIFI_MODE_STA);
    return ESP32_SendCmd("AT+CIPSTATUS\r\n", "OK", 2000);
}

uint8_t WIFI_TCP_ServerStart(uint16_t port)
{
    char cmd[40];
    uint8_t ok = 1;

    ok &= ESP32_SendCmd("AT+CIPMUX=1\r\n", "OK", 2000);
    sprintf(cmd, "AT+CIPSERVER=1,%d\r\n", port);
    ok &= ESP32_SendCmd(cmd, "OK", 3000);
    ok &= ESP32_SendCmd("AT+CIPDINFO=1\r\n", "OK", 2000);

    printf("WIFI TCP server %s, port=%d\r\n", ok ? "started" : "failed", port);
    return ok;
}

uint8_t WIFI_TCP_SendData(uint8_t id, const uint8_t *data, uint16_t len)
{
    char cmd[40];

    if(data == NULL || len == 0)
    {
        return 0;
    }

    sprintf(cmd, "AT+CIPSEND=%d,%d\r\n", id, len);
    if(!ESP32_SendCmd(cmd, ">", 3000))
    {
        WIFI_ClientConnected = 0;
        return 0;
    }

    ESP32_SendBytes(data, len);
    return ESP32_WaitFor("SEND OK", 3000);
}

uint8_t WIFI_TCP_GetClientId(uint8_t *id)
{
    if(WIFI_ClientConnected)
    {
        if(id != NULL)
        {
            *id = WIFI_ClientId;
        }
        return 1;
    }

    return 0;
}

void WIFI_TCP_Poll(void)
{
    uint8_t buffer[ESP32_RX_FRAME_SIZE];
    uint16_t len;

    if(ESP32_ReadFrame(buffer, sizeof(buffer) - 1, &len))
    {
        buffer[len] = '\0';
        printf("ESP32 RX: %s\r\n", buffer);
        WIFI_TCP_UpdateClientState(buffer, len);
    }
}

uint8_t WIFI_TCP_ReadData(uint8_t rxBuff[], uint16_t *rxLen, uint8_t *id, uint8_t *ip, uint16_t *port)
{
    static uint8_t buffer[ESP32_RX_FRAME_SIZE];
    uint16_t len;
    char *p_ipd;
    char *p_data;
    uint8_t parsed = 0;

    if(rxBuff == NULL || rxLen == NULL || id == NULL || ip == NULL || port == NULL)
    {
        return 0;
    }

    *rxLen = 0;
    if(!ESP32_ReadFrame(buffer, sizeof(buffer) - 1, &len))
    {
        return 0;
    }

    buffer[len] = '\0';
    printf("ESP32 RX: %s\r\n", buffer);
    WIFI_TCP_UpdateClientState(buffer, len);

    p_ipd = strstr((char *)buffer, "+IPD,");
    if(p_ipd == NULL)
    {
        return 0;
    }

    p_data = strstr(p_ipd, ":");
    *id = 0;
    ip[0] = '\0';
    *port = 0;

    if(sscanf(p_ipd, "+IPD,%hhu,%hu,\"%15[^\"]\",%hu", id, rxLen, ip, port) == 4)
    {
        parsed = 1;
    }
    else if(sscanf(p_ipd, "+IPD,%hhu,%hu,%15[^,],%hu", id, rxLen, ip, port) == 4)
    {
        parsed = 1;
    }
    else if(sscanf(p_ipd, "+IPD,%hhu,%hu", id, rxLen) == 2)
    {
        parsed = 1;
    }
    else if(sscanf(p_ipd, "+IPD,%hu", rxLen) == 1)
    {
        *id = 0;
        parsed = 1;
    }

    if(parsed && p_data != NULL && *rxLen < ESP32_RX_FRAME_SIZE)
    {
        p_data++;
        memcpy(rxBuff, p_data, *rxLen);
        WIFI_ClientId = *id;
        WIFI_ClientConnected = 1;
        return 1;
    }

    *rxLen = 0;
    return 0;
}
