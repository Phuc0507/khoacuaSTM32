#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"
#include "i2c.h"
#include "i2c_lcd.h"
#include "key.h"
#include "buzzer.h"
#include "servo.h"
#include "password.h"
#include "door.h"
#include "power.h"

// Fix warning implicit
extern void LCD_Backlight(uint8_t status);

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    SystemInit();      
    Delay_Init();      

    I2C_InitSoft();
    LCD_Init();
    LCD_Backlight(1); 
    
    Key_Init();
    Buzzer_Init();
    Servo_Init();
    Password_Init();
    
    vPower_Init();

    // Task Logic C?a
    xTaskCreate(Door_Task,   "Door",   512, NULL, 3, NULL);
    // Task Quét phím
    xTaskCreate(Key_Task,    "Keypad", 256, NULL, 2, NULL);
    // Task Còi
    xTaskCreate(Buzzer_Task, "Buzzer", 256, NULL, 1, NULL);
    // Task Qu?n lý ngu?n (M?I)
    xTaskCreate(vPower_Task, "Power",  128, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {}
}

// Các hàm Hook d? tr?ng ho?c treo
void vApplicationIdleHook(void) {
    // KHÔNG Ð? __WFI ? ÐÂY N?A
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { while(1); }
void vApplicationMallocFailedHook(void) { while(1); }