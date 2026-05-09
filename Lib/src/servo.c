#include "servo.h"

/* Servo:
 * PA1 – TIM2_CH2
 * 50Hz (20ms), xung ~0.5–2.5ms
 */

void Servo_Init(void)
{
    GPIO_InitTypeDef        gpio;
    TIM_TimeBaseInitTypeDef tim;
    TIM_OCInitTypeDef       oc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PA1: AF_PP */
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = GPIO_Pin_1;
    GPIO_Init(GPIOA, &gpio);

    /* TIM2: 1MHz -> Period=20000 -> 20ms */
    tim.TIM_Prescaler     = 72 - 1;
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    tim.TIM_Period        = 20000 - 1;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);

    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 1500;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC2Init(TIM2, &oc);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

void Servo_SetAngle(uint8_t angle)
{
    uint16_t pulse;

    /* 0..180 -> 500..2500 us */
    pulse = (uint16_t)(500U + ((uint32_t)angle * 2000U) / 180U);
    TIM2->CCR2 = pulse;
}

void Servo_Open(void)
{
    Servo_SetAngle(90);
}

void Servo_Close(void)
{
    Servo_SetAngle(0);
}
