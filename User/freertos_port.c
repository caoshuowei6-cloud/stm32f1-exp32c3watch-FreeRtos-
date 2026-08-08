#include "freertos_port.h"
#include "stm32f10x.h"
#include "Key.h"
#include "menu.h"
#include "AHT20.h"

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
    vTaskStartScheduler();
}
