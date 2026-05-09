#include "key.h"
#include "delay.h"
#include "power.h"       
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"            

QueueHandle_t qKey;

static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* * Hardware Map:
 * Rows (Output): PB12, PB13, PB14, PB15
 * Cols (Input):  PB8, PB9, PB10, PB11
 * Key '*': Row 3 (PB15) - Col 0 (PB8)
 */

void Key_Init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* 1. ROW: PB12..PB15 Output Push-Pull */
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOB, &gpio);

    /* M?c d?nh kéo Hàng xu?ng 0 d? s?n sàng t?o ng?t */
    GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);

    /* 2. COL: PB8..PB11 Input Pull-up */
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &gpio);
    
    /* 3. C?u hình ng?t cho C?t 0 (PB8) - Noi ch?a phím '*' */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);

    EXTI_ClearITPendingBit(EXTI_Line8);

    exti.EXTI_Line = EXTI_Line8;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling;   // Nh?n -> Kéo t? 1 xu?ng 0
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    /* 4. NVIC Configuration */
    nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 5;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    if (qKey == NULL) {
        qKey = xQueueCreate(8, sizeof(char));
    }
}

/* ============================================================
   HÀM QUAN TR?NG: G?I TRU?C KHI SLEEP
   ============================================================ */
void Key_PrepareSleep(void)
{
    /* * B?T BU?C: Kéo t?t c? các hàng (Rows) xu?ng 0 (GND).
     * Gi?i thích: PB8 (Col) dang Pull-Up (3.3V).
     * Khi nh?n nút '*', PB8 n?i v?i PB15 (Row).
     * N?u PB15 dang là 1 (3.3V) -> PB8 v?n là 3.3V -> KHÔNG CÓ NG?T.
     * N?u PB15 là 0 (GND) -> PB8 t?t xu?ng 0V -> CÓ NG?T (Falling Edge).
     */
    GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    
    /* Xóa c? ng?t cu d? tránh dánh th?c gi? */
    EXTI_ClearITPendingBit(EXTI_Line8);
}

void Key_Task(void *params)
{
    char lastKey = 0;
    char detected;
    uint8_t stableCount = 0;
    int row, col;

    (void)params;

    while (1)
    {
        detected = 0;

        /* Quét phím */
        for (row = 0; row < 4; row++)
        {
            /* Ðua t?t c? hàng lên cao tru?c */
            GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
            /* Kéo hàng hi?n t?i xu?ng th?p */
            GPIO_ResetBits(GPIOB, (uint16_t)(GPIO_Pin_12 << row));
            
            // Delay nh? d? tín hi?u ?n d?nh
            Delay_Us(10); 

            for (col = 0; col < 4; col++)
            {
                if (GPIO_ReadInputDataBit(GPIOB, (uint16_t)(GPIO_Pin_8 << col)) == 0)
                {
                    detected = keymap[row][col];
                    // Break ngay khi tìm th?y phím d? ti?t ki?m th?i gian
                    goto key_found; 
                }
            }
        }

key_found:
        /* X? lý Debounce */
        if (detected == lastKey && detected != 0)
        {
            if (stableCount < 3)
            {
                stableCount++;
            }
            else if (stableCount == 3) // Ch? g?i 1 l?n khi d?t ngu?ng
            {
                /* Nh?n ?n d?nh */
                xQueueSend(qKey, &detected, 0);
                stableCount++; // Tang d? không vào block này n?a
                
                /* Báo ho?t d?ng d? không b? sleep khi dang nh?n gi? */
               Power_UserActivity(); 
            }
        }
        else if (detected != lastKey)
        {
            stableCount = 0;
            lastKey = detected;
        }

        /* * QUAN TR?NG: Delay trong RTOS
         * S? d?ng vTaskDelay thay vì Delay() blocking n?u có th?.
         * N?u Delay() c?a b?n là blocking, Task này s? chi?m CPU.
         */
        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
}