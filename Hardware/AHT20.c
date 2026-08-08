#include "AHT20.h"
#include "Delay.h"
#include <stdio.h>

#define AHT20_ADDR              0x70
#define AHT20_TIMEOUT           100000UL

volatile int16_t AHT20_TemperatureX10 = 0;
volatile uint16_t AHT20_HumidityX10 = 0;
volatile uint8_t AHT20_DataValid = 0;

static uint8_t AHT20_WaitFlag(uint32_t flag, FlagStatus status)
{
    uint32_t timeout = AHT20_TIMEOUT;

    while(I2C_GetFlagStatus(I2C2, flag) != status)
    {
        if(timeout-- == 0)
        {
            return AHT20_ERROR;
        }
    }

    return AHT20_OK;
}

static uint8_t AHT20_WaitEvent(uint32_t event)
{
    uint32_t timeout = AHT20_TIMEOUT;

    while(I2C_CheckEvent(I2C2, event) != SUCCESS)
    {
        if(timeout-- == 0)
        {
            return AHT20_ERROR;
        }
    }

    return AHT20_OK;
}

static void AHT20_Stop(void)
{
    I2C_GenerateSTOP(I2C2, ENABLE);
    I2C_AcknowledgeConfig(I2C2, ENABLE);
}

static uint8_t AHT20_I2C_Write(const uint8_t *data, uint8_t len)
{
    uint8_t i;

    if(AHT20_WaitFlag(I2C_FLAG_BUSY, RESET) != AHT20_OK)
    {
        return AHT20_ERROR;
    }

    I2C_GenerateSTART(I2C2, ENABLE);
    if(AHT20_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != AHT20_OK)
    {
        AHT20_Stop();
        return AHT20_ERROR;
    }

    I2C_Send7bitAddress(I2C2, AHT20_ADDR, I2C_Direction_Transmitter);
    if(AHT20_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != AHT20_OK)
    {
        AHT20_Stop();
        return AHT20_ERROR;
    }

    for(i = 0; i < len; i++)
    {
        I2C_SendData(I2C2, data[i]);
        if(AHT20_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != AHT20_OK)
        {
            AHT20_Stop();
            return AHT20_ERROR;
        }
    }

    AHT20_Stop();
    return AHT20_OK;
}

static uint8_t AHT20_I2C_Read(uint8_t *data, uint8_t len)
{
    uint8_t i;

    if((data == 0) || (len == 0))
    {
        return AHT20_ERROR;
    }

    if(AHT20_WaitFlag(I2C_FLAG_BUSY, RESET) != AHT20_OK)
    {
        return AHT20_ERROR;
    }

    I2C_AcknowledgeConfig(I2C2, ENABLE);
    I2C_GenerateSTART(I2C2, ENABLE);
    if(AHT20_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != AHT20_OK)
    {
        AHT20_Stop();
        return AHT20_ERROR;
    }

    I2C_Send7bitAddress(I2C2, AHT20_ADDR, I2C_Direction_Receiver);
    if(AHT20_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != AHT20_OK)
    {
        AHT20_Stop();
        return AHT20_ERROR;
    }

    for(i = 0; i < len; i++)
    {
        if(i == (uint8_t)(len - 1))
        {
            I2C_AcknowledgeConfig(I2C2, DISABLE);
            I2C_GenerateSTOP(I2C2, ENABLE);
        }

        if(AHT20_WaitFlag(I2C_FLAG_RXNE, SET) != AHT20_OK)
        {
            AHT20_Stop();
            return AHT20_ERROR;
        }

        data[i] = I2C_ReceiveData(I2C2);
    }

    I2C_AcknowledgeConfig(I2C2, ENABLE);
    return AHT20_OK;
}

void AHT20_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    uint8_t status = 0;
    uint8_t cmd[3] = {0xBE, 0x08, 0x00};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_DeInit(I2C2);
    I2C_SoftwareResetCmd(I2C2, ENABLE);
    I2C_SoftwareResetCmd(I2C2, DISABLE);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);

    Delay_ms(40);

    if(AHT20_I2C_Read(&status, 1) == AHT20_OK)
    {
        if((status & 0x08) == 0)
        {
            (void)AHT20_I2C_Write(cmd, 3);
            Delay_ms(10);
        }
    }
    else
    {
        (void)AHT20_I2C_Write(cmd, 3);
        Delay_ms(10);
    }
}

uint8_t AHT20_ReadInt(int16_t *temperature_x10, uint16_t *humidity_x10)
{
    uint8_t read_buf[6];
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint32_t data;
    int32_t temperature;

    if((temperature_x10 == 0) || (humidity_x10 == 0))
    {
        return AHT20_ERROR;
    }

    if(AHT20_I2C_Write(cmd, 3) != AHT20_OK)
    {
        return AHT20_ERROR;
    }

    Delay_ms(80);

    if(AHT20_I2C_Read(read_buf, 6) != AHT20_OK)
    {
        return AHT20_ERROR;
    }

    if((read_buf[0] & 0x80) != 0)
    {
        return AHT20_ERROR;
    }

    data = (((uint32_t)read_buf[1] << 12) |
            ((uint32_t)read_buf[2] << 4) |
            ((uint32_t)read_buf[3] >> 4));
    *humidity_x10 = (uint16_t)((data * 1000UL) >> 20);

    data = (((uint32_t)(read_buf[3] & 0x0F) << 16) |
            ((uint32_t)read_buf[4] << 8) |
            ((uint32_t)read_buf[5]));
    temperature = (int32_t)((data * 2000UL) >> 20) - 500;
    *temperature_x10 = (int16_t)temperature;

    return AHT20_OK;
}

uint8_t AHT20_Update(void)
{
    int16_t temperature_x10;
    uint16_t humidity_x10;

    if(AHT20_ReadInt(&temperature_x10, &humidity_x10) != AHT20_OK)
    {
        AHT20_DataValid = 0;
        return AHT20_ERROR;
    }

    AHT20_TemperatureX10 = temperature_x10;
    AHT20_HumidityX10 = humidity_x10;
    AHT20_DataValid = 1;

    return AHT20_OK;
}

void AHT20_PrintSerial(void)
{
    int16_t temperature_x10;
    uint16_t humidity_x10;
    int16_t temperature_abs;

    if(AHT20_ReadInt(&temperature_x10, &humidity_x10) != AHT20_OK)
    {
        printf("AHT20 read error\r\n");
        return;
    }

    if(temperature_x10 < 0)
    {
        temperature_abs = (int16_t)(-temperature_x10);
        printf("AHT20 T:-%d.%dC H:%d.%d%%\r\n",
               temperature_abs / 10,
               temperature_abs % 10,
               humidity_x10 / 10,
               humidity_x10 % 10);
    }
    else
    {
        printf("AHT20 T:%d.%dC H:%d.%d%%\r\n",
               temperature_x10 / 10,
               temperature_x10 % 10,
               humidity_x10 / 10,
               humidity_x10 % 10);
    }
}
