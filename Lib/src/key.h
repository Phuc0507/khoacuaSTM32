#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t qKey;

/* Kh?i t?o keypad */
void Key_Init(void);

/* Task d?c keypad */
void Key_Task(void *params);

/* H? tr? d?c 1 phím (n?u dùng polling)
   nhung trong code chính b?n dang dùng queue */
char Key_GetKey(void);

#endif
