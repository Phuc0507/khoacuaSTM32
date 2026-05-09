#ifndef __I2C_LCD_H__
#define __I2C_LCD_H__

#include <stdint.h>

/* ============================================================
   API CO B?N CHO LCD
   ============================================================ */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_WriteChar(char c);
void LCD_Print(char *str);
void LCD_Backlight(uint8_t on);

/* ============================================================
   CÁC HÀM UI M? R?NG (DÙNG CHO KHÓA C?A)
   ============================================================ */

/* xóa 1 dòng */
void LCD_ClearLine(uint8_t row);

/* in ch? can gi?a */
void LCD_PrintCenter(uint8_t row, char *text);

/* in s? nguyên */
void LCD_PrintNumber(int num);

/* giao di?n d?m ngu?c 30s */
void LCD_ShowCountdown(uint32_t sec);

/* giao di?n báo sai m?t kh?u còn l?i N l?n */
void LCD_ShowWrongAttempts(uint8_t remain);

/* giao di?n báo dúng m?t kh?u */
void LCD_ShowCorrect(void);

/* giao di?n báo dã b? khóa 30s */
void LCD_ShowLocked(void);

#endif
