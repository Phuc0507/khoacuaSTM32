#include "delay.h"

/* TIM4 ch?y 1MHz: 1 tick = 1us */

void Delay_Init(void)
{
    TIM_TimeBaseInitTypeDef tim;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    tim.TIM_Prescaler     = 72 - 1;      /* 72MHz / 72 = 1MHz */
    tim.TIM_CounterMode   = TIM_CounterMode_Up;
    tim.TIM_Period        = 0xFFFF;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM4, &tim);

    TIM_Cmd(TIM4, ENABLE);
}

void Delay_Us(uint32_t us)
{
    uint16_t start;

    if (us == 0) return;

    start = TIM4->CNT;   //Luu giá tri hien tai cua bo dem Timer.
    while ((uint16_t)(TIM4->CNT - start) < us)
    {
        /* busy-wait */
    }
}

void Delay_Ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_Us(1000);
    }
}

void Delay(uint32_t ms) // Tuong thich voi RTOS 
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        vTaskDelay(pdMS_TO_TICKS(ms)); // OS Ðang chay
    }
    else
    {
        Delay_Ms(ms); // Chua chay thi g?i 
    }
}
