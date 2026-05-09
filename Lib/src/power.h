#ifndef __POWER_H
#define __POWER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

// Hàm kh?i t?o (G?i trong main, tru?c vTaskStartScheduler)
void vPower_Init(void);

// Task qu?n lý ngu?n (Ðua vào xTaskCreate)
void vPower_Task(void *pvParameters);

// Hàm báo hi?u có ho?t d?ng (G?i t? key.c khi có phím nh?n)
void Power_UserActivity(void);

void Power_Reset_Timer(void);
#ifdef __cplusplus
 }
#endif

#endif /* __POWER_H */