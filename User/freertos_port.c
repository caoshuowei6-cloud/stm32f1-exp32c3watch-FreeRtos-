#include "freertos_port.h"
#include "stm32f10x.h"
#include "Key.h"
#include "menu.h"
#include "AHT20.h"
#include "WIFI.h"
#include <stdio.h>
#include <string.h>

#define WIFI_UPLOAD_INTERVAL_MS 1000

SemaphoreHandle_t xOLEDMutex = NULL;
QueueHandle_t xKeyQueue = NULL;
QueueHandle_t xMenuQueue = NULL;
TaskHandle_t xKeyTaskHandle = NULL;
TaskHandle_t xUITaskHandle = NULL;

void vOLEDMutexCreate(void)
{
    xOLEDMutex = xSemaphoreCreateMutex();
}

BaseType_t xOLEDTake(uint32_t timeout)
{
    if(xOLEDMutex == NULL)
    {
        return pdFALSE;
    }

    return xSemaphoreTake(xOLEDMutex, timeout);
}

void xOLEDGive(void)
{
    if(xOLEDMutex != NULL)
    {
        xSemaphoreGive(xOLEDMutex);
    }
}

static void vKeyTask(void *pvParameters)
{
    uint8_t key_num;

    (void)pvParameters;

    for(;;)
    {
        key_num = Key_GetNum();
        if(key_num != 0)
        {
            if(key_num == 4)
            {
                GPIO_ResetBits(GPIOB, GPIO_Pin_13);
                GPIO_SetBits(GPIOB, GPIO_Pin_12);
            }
            else
            {
                xQueueSend(xKeyQueue, &key_num, portMAX_DELAY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static int32_t Wifi_FloatToX100(float value)
{
    if(value >= 0.0f)
    {
        return (int32_t)(value * 100.0f + 0.5f);
    }

    return (int32_t)(value * 100.0f - 0.5f);
}

static uint32_t Wifi_Abs32(int32_t value)
{
    if(value < 0)
    {
        return (uint32_t)(-value);
    }

    return (uint32_t)value;
}

static void Wifi_BuildSensorJson(char *buffer, uint16_t size)
{
    static uint32_t upload_seq = 0;
    int16_t temp_x10;
    uint16_t humi_x10;
    uint16_t temp_abs;
    int32_t roll_x100;
    int32_t pitch_x100;
    int32_t yaw_x100;
    uint32_t roll_abs;
    uint32_t pitch_abs;
    uint32_t yaw_abs;

    upload_seq++;
    temp_x10 = AHT20_TemperatureX10;
    humi_x10 = AHT20_HumidityX10;
    roll_x100 = Wifi_FloatToX100(Roll);
    pitch_x100 = Wifi_FloatToX100(Pitch);
    yaw_x100 = Wifi_FloatToX100(Yaw);

    temp_abs = (temp_x10 < 0) ? (uint16_t)(-temp_x10) : (uint16_t)temp_x10;
    roll_abs = Wifi_Abs32(roll_x100);
    pitch_abs = Wifi_Abs32(pitch_x100);
    yaw_abs = Wifi_Abs32(yaw_x100);

    snprintf(buffer, size,
             "{\"seq\":%lu,\"temperature\":%s%d.%d,\"humidity\":%d.%d,\"roll\":%s%ld.%02ld,\"pitch\":%s%ld.%02ld,\"yaw\":%s%ld.%02ld,\"aht20_valid\":%d}\r\n",
             (unsigned long)upload_seq,
             (temp_x10 < 0) ? "-" : "", temp_abs / 10, temp_abs % 10,
             humi_x10 / 10, humi_x10 % 10,
             (roll_x100 < 0) ? "-" : "", (long)(roll_abs / 100), (long)(roll_abs % 100),
             (pitch_x100 < 0) ? "-" : "", (long)(pitch_abs / 100), (long)(pitch_abs % 100),
             (yaw_x100 < 0) ? "-" : "", (long)(yaw_abs / 100), (long)(yaw_abs % 100),
             AHT20_DataValid);
}

static void Wifi_SendSensorData(void)
{
    static char tx_buffer[192];
    uint8_t client_id;

    if(WIFI_TCP_GetClientId(&client_id) == 0)
    {
        return;
    }

    Wifi_BuildSensorJson(tx_buffer, sizeof(tx_buffer));
    if(WIFI_TCP_SendData(client_id, (const uint8_t *)tx_buffer, strlen(tx_buffer)))
    {
        printf("WiFi upload: %s", tx_buffer);
    }
    else
    {
        printf("WiFi upload failed\r\n");
    }
}

static void vWifiTask(void *pvParameters)
{
    TickType_t last_upload_tick;
    TickType_t now_tick;

    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("WiFi test start\r\n");
    WIFI_Init(WIFI_MODE_STA);
    WIFI_TCP_ServerStart(8080);
    last_upload_tick = xTaskGetTickCount();

    for(;;)
    {
        WIFI_TCP_Poll();

        now_tick = xTaskGetTickCount();
        if((now_tick - last_upload_tick) >= pdMS_TO_TICKS(WIFI_UPLOAD_INTERVAL_MS))
        {
            Wifi_SendSensorData();
            last_upload_tick = now_tick;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
static void vUITask(void *pvParameters)
{
    int clkflag;

    (void)pvParameters;

    for(;;)
    {
        clkflag = First_Page_Clock();

        if(clkflag == 1)
        {
            Menu();
        }
        else if(clkflag == 2)
        {
            SettingPage();
        }
    }
}

static void vSensorTask(void *pvParameters)
{
    TickType_t last_aht_tick;

    (void)pvParameters;
    AHT20_Init();
    AHT20_Update();
    last_aht_tick = xTaskGetTickCount();

    for(;;)
    {
        MPU6050_Calculation();

        if((xTaskGetTickCount() - last_aht_tick) >= pdMS_TO_TICKS(1000))
        {
            AHT20_Update();
            last_aht_tick = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void vInitTasks(void)
{
    xKeyQueue = xQueueCreate(10, sizeof(uint8_t));
    xMenuQueue = xQueueCreate(5, sizeof(uint8_t));
    vOLEDMutexCreate();

    xTaskCreate(vKeyTask, "KeyTask", 128, NULL, 3, &xKeyTaskHandle);
    xTaskCreate(vSensorTask, "Sensor", 256, NULL, 1, NULL);
    xTaskCreate(vUITask, "UITask", 768, NULL, 2, &xUITaskHandle);
    xTaskCreate(vWifiTask, "WiFi", 512, NULL, 1, NULL);
    vTaskStartScheduler();
}
