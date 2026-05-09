#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);
void Servo_Open(void);
void Servo_Close(void);

#endif
