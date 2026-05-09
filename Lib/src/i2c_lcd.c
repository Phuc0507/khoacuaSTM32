#include "i2c_lcd.h"
#include "i2c.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>   // d? tránh warning sprintf

#define LCD_ADDR8       (0x27 << 1)
#define LCD_BACKLIGHT   0x08
#define LCD_EN          0x04
#define LCD_RW          0x02
#define LCD_RS          0x01

static uint8_t lcd_backlight_state = LCD_BACKLIGHT;

/* ============================================================
   LOW LEVEL WRITE
   ============================================================ */
static void LCD_Write4(uint8_t data)
{
    I2C_WriteByteTo(LCD_ADDR8, data | lcd_backlight_state | LCD_EN);
    Delay_Us(50);

    I2C_WriteByteTo(LCD_ADDR8, data | lcd_backlight_state);
    Delay_Us(50);
}

static void LCD_SendCmd(uint8_t cmd)
{
    uint8_t high = (cmd & 0xF0);
    uint8_t low  = (cmd << 4) & 0xF0;

    LCD_Write4(high);
    LCD_Write4(low);
}

static void LCD_SendData(uint8_t data)
{
    uint8_t high = (data & 0xF0) | LCD_RS;
    uint8_t low  = ((data << 4) & 0xF0) | LCD_RS;

    LCD_Write4(high);
    LCD_Write4(low);
}

/* ============================================================
   BASIC LCD FUNCTIONS
   ============================================================ */
void LCD_Init(void)
{
    Delay_Ms(50);

    LCD_Write4(0x30);
    Delay_Ms(5);
    LCD_Write4(0x30);
    Delay_Us(200);
    LCD_Write4(0x20);

    LCD_SendCmd(0x28);
    LCD_SendCmd(0x0C);
    LCD_SendCmd(0x06);

    LCD_Clear();
}

void LCD_Clear(void)
{
    LCD_SendCmd(0x01);
    Delay_Ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    LCD_SendCmd((row == 0 ? 0x80 : 0xC0) + col);
}

void LCD_WriteChar(char c)
{
    LCD_SendData((uint8_t)c);
}

void LCD_Print(char *str)
{
    while (*str)
        LCD_WriteChar(*str++);
}

void LCD_Backlight(uint8_t on)
{
    lcd_backlight_state = on ? LCD_BACKLIGHT : 0;
    I2C_WriteByteTo(LCD_ADDR8, lcd_backlight_state);
}

/* ============================================================
   UI EXTENDED FUNCTIONS
   ============================================================ */

void LCD_ClearLine(uint8_t row)
{
    uint8_t i;

    LCD_SetCursor(row, 0);
    for (i = 0; i < 16; i++)
        LCD_WriteChar(' ');

    LCD_SetCursor(row, 0);
}

void LCD_PrintCenter(uint8_t row, char *text)
{
    uint8_t len = strlen(text);
    uint8_t pos = 0;

    if (len < 16)
        pos = (16 - len) / 2;

    LCD_SetCursor(row, pos);
    LCD_Print(text);
}

void LCD_PrintNumber(int num)
{
    char buf[12];
    sprintf(buf, "%d", num);
    LCD_Print(buf);
}

void LCD_ShowWrongAttempts(uint8_t remain)
{
    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_Print("Sai mat khau!");

    LCD_SetCursor(1, 0);
    LCD_Print("Con lai: ");
    LCD_PrintNumber(remain);
		Delay_Ms(0);
}

void LCD_ShowCountdown(uint32_t sec)
{
    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_Print("Vui long doi");

    LCD_SetCursor(1, 0);
    LCD_Print("Con ");
    LCD_PrintNumber(sec);
    LCD_Print(" giay");
}


void LCD_ShowCorrect(void)
{
    LCD_Clear();
    LCD_PrintCenter(0, "Mat khau dung");
		Delay_Ms(0);
}

void LCD_ShowLocked(void)
{
    LCD_Clear();
    LCD_PrintCenter(0, "Vui long cho 30s!");
}
