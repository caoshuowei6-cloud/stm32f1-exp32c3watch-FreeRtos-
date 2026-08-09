#include "ESP32.h"
#include "Delay.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static uint8_t ESP32_DmaRxBuf[ESP32_RX_DMA_SIZE];
static uint8_t ESP32_FrameBuf[ESP32_RX_FRAME_SIZE];
static volatile uint16_t ESP32_FrameLen;
static volatile uint8_t ESP32_FrameReady;
static volatile uint8_t ESP32_FrameOverflow;

static void ESP32_USART2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_DeInit(USART2);
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

static void ESP32_DMA_RxStart(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_Cmd(DMA1_Channel6, DISABLE);
    DMA_DeInit(DMA1_Channel6);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ESP32_DmaRxBuf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ESP32_RX_DMA_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);

    DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_HT6 | DMA1_FLAG_TE6);
    DMA_Cmd(DMA1_Channel6, ENABLE);
}

static void ESP32_DMA_RxRestart(void)
{
    DMA_Cmd(DMA1_Channel6, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel6, ESP32_RX_DMA_SIZE);
    DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_HT6 | DMA1_FLAG_TE6);
    DMA_Cmd(DMA1_Channel6, ENABLE);
}

void ESP32_Init(void)
{
    ESP32_FrameLen = 0;
    ESP32_FrameReady = 0;
    ESP32_FrameOverflow = 0;
    memset(ESP32_DmaRxBuf, 0, sizeof(ESP32_DmaRxBuf));
    memset(ESP32_FrameBuf, 0, sizeof(ESP32_FrameBuf));

    ESP32_USART2_Init();
    ESP32_DMA_RxStart();
}

void ESP32_SendBytes(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++)
    {
        USART_SendData(USART2, data[i]);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

void ESP32_SendString(const char *str)
{
    ESP32_SendBytes((const uint8_t *)str, strlen(str));
}

void ESP32_ClearRx(void)
{
    NVIC_DisableIRQ(USART2_IRQn);
    ESP32_FrameLen = 0;
    ESP32_FrameReady = 0;
    ESP32_FrameOverflow = 0;
    memset(ESP32_FrameBuf, 0, sizeof(ESP32_FrameBuf));
    NVIC_EnableIRQ(USART2_IRQn);
}

uint8_t ESP32_ReadFrame(uint8_t *buffer, uint16_t buffer_size, uint16_t *len)
{
    uint16_t copy_len;

    if(buffer == NULL || len == NULL || buffer_size == 0)
    {
        return 0;
    }

    if(ESP32_FrameReady == 0)
    {
        *len = 0;
        return 0;
    }

    NVIC_DisableIRQ(USART2_IRQn);
    copy_len = ESP32_FrameLen;
    if(copy_len > buffer_size)
    {
        copy_len = buffer_size;
    }
    memcpy(buffer, ESP32_FrameBuf, copy_len);
    *len = copy_len;
    ESP32_FrameLen = 0;
    ESP32_FrameReady = 0;
    NVIC_EnableIRQ(USART2_IRQn);

    return 1;
}

uint8_t ESP32_WaitFor(const char *ack, uint32_t timeout_ms)
{
    static char response[ESP32_CMD_RESP_SIZE];
    uint8_t frame[ESP32_RX_FRAME_SIZE];
    uint16_t frame_len;
    uint16_t used = 0;
    uint32_t elapsed = 0;
    uint16_t copy_len;

    memset(response, 0, sizeof(response));

    while(elapsed < timeout_ms)
    {
        if(ESP32_ReadFrame(frame, sizeof(frame), &frame_len))
        {
            printf("ESP32 RX: %.*s\r\n", frame_len, frame);

            if(used < (sizeof(response) - 1))
            {
                copy_len = frame_len;
                if(copy_len > (sizeof(response) - 1 - used))
                {
                    copy_len = sizeof(response) - 1 - used;
                }
                memcpy(response + used, frame, copy_len);
                used += copy_len;
                response[used] = '\0';
            }

            if((ack != NULL) && (strstr(response, ack) != NULL))
            {
                return 1;
            }

            if(strstr(response, "ERROR") != NULL ||
               strstr(response, "FAIL") != NULL ||
               strstr(response, "busy") != NULL)
            {
                return 0;
            }
        }

        Delay_ms(10);
        elapsed += 10;
    }

    return 0;
}

uint8_t ESP32_SendCmd(const char *cmd, const char *ack, uint32_t timeout_ms)
{
    ESP32_ClearRx();
    printf("ESP32 TX: %s", cmd);
    ESP32_SendString(cmd);

    if(ack == NULL)
    {
        return 1;
    }

    return ESP32_WaitFor(ack, timeout_ms);
}

void ESP32_USART2_IRQHandler(void)
{
    uint16_t len;
    volatile uint16_t clear;

    if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
    {
        clear = USART2->SR;
        clear = USART2->DR;
        (void)clear;

        DMA_Cmd(DMA1_Channel6, DISABLE);
        len = ESP32_RX_DMA_SIZE - DMA_GetCurrDataCounter(DMA1_Channel6);

        if(len > 0)
        {
            if(ESP32_FrameReady == 0)
            {
                if(len > ESP32_RX_FRAME_SIZE)
                {
                    len = ESP32_RX_FRAME_SIZE;
                    ESP32_FrameOverflow = 1;
                }
                memcpy(ESP32_FrameBuf, ESP32_DmaRxBuf, len);
                ESP32_FrameLen = len;
                ESP32_FrameReady = 1;
            }
            else
            {
                ESP32_FrameOverflow = 1;
            }
        }

        ESP32_DMA_RxRestart();
    }
}
