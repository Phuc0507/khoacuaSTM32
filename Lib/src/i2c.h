#ifndef __I2C_H
#define __I2C_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>

extern SemaphoreHandle_t xI2C_Mutex;

void I2C_InitSoft(void);

uint8_t I2C_WriteByteTo(uint8_t addr8, uint8_t data);
uint8_t I2C_WriteBytesTo(uint8_t addr8, const uint8_t *buf, uint8_t len);

#endif
