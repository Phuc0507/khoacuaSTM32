#include "stm32f10x.h"
#include "I2C_lcd.h"
#include "delay.h"
#include "key.h"
#include <stdio.h>

int main(void)
{
	GPIO_InitTypeDef gpioInit;
	uint32_t key;
	char szKey[20];
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	gpioInit.GPIO_Mode = GPIO_Mode_Out_PP;
	gpioInit.GPIO_Pin = GPIO_Pin_13;
	gpioInit.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOC, &gpioInit);
	
	Delay_Init();
	Key_Init();
	
	I2C_LCD_Init();
	I2C_LCD_Clear();
	I2C_LCD_BackLight(1);
	I2C_LCD_Puts("STM32F103C8T6");
	I2C_LCD_NewLine();
	I2C_LCD_Puts("Test");
	
	while (1) {
		key = Key_Scan();
		if (key) {
			I2C_LCD_Clear();
			I2C_LCD_Puts("Phim duoc nhan");
			I2C_LCD_NewLine();
			sprintf(szKey, "%d", key);
			I2C_LCD_Puts(szKey);
		}
	}
}
