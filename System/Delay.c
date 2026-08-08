#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

void Delay_us(uint32_t xus)
{
    volatile uint32_t i;

    while(xus--)
    {
        for(i = 0; i < 9; i++)
        {
            __NOP();
        }
    }
}

void Delay_ms(uint32_t xms)
{
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        TickType_t ticks = pdMS_TO_TICKS(xms);

        if((ticks == 0) && (xms != 0))
        {
            ticks = 1;
        }

        vTaskDelay(ticks);
        return;
    }

    while(xms--)
    {
        Delay_us(1000);
    }
}

void Delay_s(uint32_t xs)
{
    while(xs--)
    {
        Delay_ms(1000);
    }
}
