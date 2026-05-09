#include "i2c.h"
#include "delay.h"

#define I2C_PORT    GPIOB
#define SDA_PIN     GPIO_Pin_7
#define SCL_PIN     GPIO_Pin_6

SemaphoreHandle_t xI2C_Mutex;

/* MACRO */
#define SDA_HIGH()  GPIO_SetBits(I2C_PORT, SDA_PIN)
#define SDA_LOW()   GPIO_ResetBits(I2C_PORT, SDA_PIN)
#define SCL_HIGH()  GPIO_SetBits(I2C_PORT, SCL_PIN)
#define SCL_LOW()   GPIO_ResetBits(I2C_PORT, SCL_PIN)
#define SDA_READ()  GPIO_ReadInputDataBit(I2C_PORT, SDA_PIN)

/* =====================================
          LOW LEVEL CONFIG
===================================== */
static void SDA_Output(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Pin   = SDA_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &gpio);
}

static void SDA_Input(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Pin   = SDA_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &gpio);
}

void I2C_InitSoft(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Pin   = SDA_PIN | SCL_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &gpio);

    SDA_HIGH();
    SCL_HIGH();

    /* T?o mutex cho FreeRTOS */
    xI2C_Mutex = xSemaphoreCreateMutex();
}

/* =====================================
          I2C LOW LEVEL
===================================== */
static void I2C_Start(void)
{
    SDA_Output();
    SDA_HIGH();
    SCL_HIGH();
    Delay_Us(5);

    SDA_LOW();
    Delay_Us(5);

    SCL_LOW();
}

static void I2C_Stop(void)
{
    SDA_Output();
    SDA_LOW();
    SCL_HIGH();
    Delay_Us(5);
    SDA_HIGH();
    Delay_Us(5);
}

static uint8_t I2C_WriteByteRaw(uint8_t byte)
{
    uint8_t i;
    uint8_t ack;

    SDA_Output();

    for (i = 0; i < 8; i++)
    {
        if (byte & 0x80U) SDA_HIGH();
        else              SDA_LOW();

        Delay_Us(2);
        SCL_HIGH();
        Delay_Us(3);
        SCL_LOW();

        byte <<= 1;
    }

    SDA_Input();
    Delay_Us(2);
    SCL_HIGH();
    ack = (uint8_t)(!SDA_READ());
    Delay_Us(3);
    SCL_LOW();

    SDA_Output();
    SDA_HIGH();

    return ack;
}

/* =====================================
         API SAFE WITH FreeRTOS
===================================== */
uint8_t I2C_WriteByteTo(uint8_t addr8, uint8_t data)
{
    uint8_t ok;

    xSemaphoreTake(xI2C_Mutex, portMAX_DELAY);

    I2C_Start();
    if (!I2C_WriteByteRaw(addr8)) { ok = 0; goto exit; }
    if (!I2C_WriteByteRaw(data))  { ok = 0; goto exit; }
    ok = 1;

exit:
    I2C_Stop();
    xSemaphoreGive(xI2C_Mutex);
    return ok;
}

uint8_t I2C_WriteBytesTo(uint8_t addr8, const uint8_t *buf, uint8_t len)
{
    uint8_t i, ok = 1;

    xSemaphoreTake(xI2C_Mutex, portMAX_DELAY);

    I2C_Start();
    if (!I2C_WriteByteRaw(addr8)) { ok = 0; goto exit; }

    for (i = 0; i < len; i++)
    {
        if (!I2C_WriteByteRaw(buf[i])) { ok = 0; goto exit; }
    }

exit:
    I2C_Stop();
    xSemaphoreGive(xI2C_Mutex);
    return ok;
}
