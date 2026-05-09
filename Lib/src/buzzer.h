#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef enum
{
    BUZZ_NONE = 0,
    BUZZ_SHORT,
    BUZZ_DOUBLE,
    BUZZ_LONG
} BUZZ_PATTERN;

extern QueueHandle_t qBuzz;

void Buzzer_Init(void);
void Buzzer_Task(void *params);
void Buzzer_Play(BUZZ_PATTERN p);

#endif
