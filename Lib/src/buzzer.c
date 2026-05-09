#include "buzzer.h"
#include "delay.h"

QueueHandle_t qBuzz;

/* Buzzer: PB5 active HIGH */

static void buzzer_on(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_5);
}

static void buzzer_off(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = GPIO_Pin_5;
    GPIO_Init(GPIOB, &gpio);

    buzzer_off();

    qBuzz = xQueueCreate(8, sizeof(BUZZ_PATTERN));
}

void Buzzer_Play(BUZZ_PATTERN p)
{
    if (qBuzz != NULL)
    {
        xQueueSend(qBuzz, &p, 0);
    }
}

static void beep(uint16_t on_ms, uint16_t off_ms)
{
    buzzer_on();
    Delay(on_ms);
    buzzer_off();
    Delay(off_ms);
}

void Buzzer_Task(void *params)
{
    BUZZ_PATTERN p;

    while (1)
    {
        if (xQueueReceive(qBuzz, &p, portMAX_DELAY) == pdPASS)
        {
            switch (p)
            {
            case BUZZ_SHORT:
                beep(50, 20);
                break;

            case BUZZ_DOUBLE:
                beep(50, 50);
                beep(50, 20);
                break;

            case BUZZ_LONG:
                beep(500, 50);
                break;

            default:
                break;
            }
        }
    }
}
