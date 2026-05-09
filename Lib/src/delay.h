#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "FreeRTOS.h"
#include "task.h"

void Delay_Init(void);
void Delay_Us(uint32_t us);
void Delay_Ms(uint32_t ms);

/* Delay thông minh: n?u FreeRTOS dã ch?y thì dùng vTaskDelay */
void Delay(uint32_t ms);

#endif
