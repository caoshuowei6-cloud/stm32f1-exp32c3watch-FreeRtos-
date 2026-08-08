#ifndef __FREERTOS_PORT_H
#define __FREERTOS_PORT_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

extern SemaphoreHandle_t xOLEDMutex;
extern QueueHandle_t xKeyQueue;
extern QueueHandle_t xMenuQueue;
extern TaskHandle_t xKeyTaskHandle;
extern TaskHandle_t xUITaskHandle;

void vOLEDMutexCreate(void);
BaseType_t xOLEDTake(uint32_t timeout);
void xOLEDGive(void);
void vInitTasks(void);

#ifndef OLED_TAKE
#define OLED_TAKE() xOLEDTake(pdMS_TO_TICKS(100))
#endif

#ifndef OLED_GIVE
#define OLED_GIVE() xOLEDGive()
#endif

#endif
