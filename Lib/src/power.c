#include "power.h"
#include "i2c_lcd.h"
#include "key.h"
#include "delay.h"
#include "stm32f10x_pwr.h"
#include "task.h"
#include "servo.h"
// --- BI?N TOÀN C?C ---
static TickType_t last_active_tick = 0; //luu thoi diem cuoi co tuong tac
#define POWER_IDLE_TIME_MS    10000 // 10s

extern void LCD_Backlight(uint8_t status);

// Hàm này g?i t? key.c d? báo có ngu?i dùng
void Power_UserActivity(void) {
    last_active_tick = xTaskGetTickCount();
}

void Power_Reset_Timer(void) {
    last_active_tick = xTaskGetTickCount();
}

static void vSystem_GoToSleep(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 1. Báo hi?u s?p ng? (T?t dèn n?n)
    LCD_Backlight(0); 
    
    // 2. NGUNG HO?T Ð?NG C?A FREERTOS
    // Ð? d?m b?o không ai làm phi?n khi dang setup ng?
    vTaskSuspendAll();

    // 3. C?U HÌNH GPIO Ð? NG?
    // B?t Clock c?n thi?t (KHÔNG ÐU?C T?T AFIO VÀ PORT B)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    // --- Port A: T?t h?t tr? chân n?p (PA13, PA14) ---
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_All & ~(GPIO_Pin_13 | GPIO_Pin_14);
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // --- Port C: T?t h?t ---
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_All;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // --- Port B: C?u hình m?ch dánh th?c ---
    // PB15 (Row): Output Low (0V)
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB, GPIO_Pin_15); // Kéo xu?ng Mass

    // PB8 (Col): Input Pull-Up (3.3V)
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_8); // Kích tr? treo lên

    // 4. C?U HÌNH EXTI (Ng?t ngoài)
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);

    EXTI_InitStructure.EXTI_Line    = EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // B?t su?n xu?ng
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // 5. C?U HÌNH NVIC (Uu tiên ng?t)
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. Xóa c? ng?t cu (Quan tr?ng)
    EXTI_ClearITPendingBit(EXTI_Line8);

    // 7. C?u hình ch? d? ng? (Sleep Normal)
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; 
    
    // T?t SysTick d? nó không dánh th?c chip m?i 1ms
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // >>>>> NG? T?I ÐÂY <<<<<
    // Chip s? d?ng ? dòng này cho d?n khi b?n b?m nút
    __WFI(); 
    // >>>>> ÐÃ T?NH D?Y <<<<<

    // 8. KHÔI PH?C H? TH?NG
    // B?t l?i SysTick
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    // Khôi ph?c bàn phím v? ch? d? quét ma tr?n
    Key_Init(); 
		
		Servo_Init();
		Servo_Close();
    
    // Cho phép FreeRTOS ch?y l?i
    xTaskResumeAll();
    
    // B?t dèn n?n
    LCD_Backlight(1);
    
    // Reset th?i gian d?m
    last_active_tick = xTaskGetTickCount();
}

void vPower_Task(void *pvParameters) {
    last_active_tick = xTaskGetTickCount();

    for (;;) {
        // Ki?m tra th?i gian r?nh
        if ((xTaskGetTickCount() - last_active_tick) > pdMS_TO_TICKS(POWER_IDLE_TIME_MS)) {
            // G?i hàm ng? tr?c ti?p
            vSystem_GoToSleep();
        }
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

void vPower_Init(void) {
    // Không c?n Init gì ph?c t?p
}

// Hàm x? lý ng?t (B?t bu?c ph?i có d? dánh th?c)
void EXTI9_5_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line8) != RESET) {
        // Ch? c?n xóa c? ng?t là d?, CPU t? d?ng thoát __WFI()
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
}