#include "door.h"
#include "key.h"
#include "password.h"
#include "servo.h"
#include "buzzer.h"
#include "i2c_lcd.h"
#include "delay.h" 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>

/* ===============================
      DOOR STATES
================================= */
typedef enum
{
    DOOR_STATE_IDLE = 0, 
    DOOR_STATE_ENTER_PIN_OPEN, 
    DOOR_STATE_ENTER_PIN_ADMIN,  
    DOOR_STATE_SET_NEW_PIN				
} DOOR_STATE;

/* ===============================
 DISPLAY HELPERS
================================= */
static void door_display_idle(void) // Màn hình cho 
{
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("*: Mo cua (PIN)");
    LCD_SetCursor(1, 0);
    LCD_Print("A: Doi PIN");
}
/* MAN HINH CHO */
static void door_open_success(void) 
{
    Buzzer_Play(BUZZ_DOUBLE);
    taskYIELD(); 

    LCD_ShowCorrect(); 
    LCD_SetCursor(1, 0);
    LCD_Print("Dang mo cua..."); 

    Servo_Open();
    vTaskDelay(pdMS_TO_TICKS(3000));

    Servo_Close();
    LCD_Clear();
    LCD_PrintCenter(0, "Cua da dong");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    door_display_idle();
}

/* ===============================
          DOOR TASK
================================= */
void Door_Task(void *params)
{
    DOOR_STATE state = DOOR_STATE_IDLE;

    char key;
    char temp[2];
    char input_buf[MAX_PASS_LEN + 1];
    uint8_t input_len = 0;
    
    uint8_t attempts_remain = 3; 
    int i; 

    (void)params;

    LCD_Init();
    LCD_Backlight(1);
    door_display_idle();
    Servo_Close();

    while (1)
    {
        /* ===========================================
                KEY INPUT
        ============================================ */
        if (xQueueReceive(qKey, &key, pdMS_TO_TICKS(50)) == pdPASS)
        {
            Buzzer_Play(BUZZ_SHORT);
            taskYIELD();

            switch (state)
            {
            case DOOR_STATE_IDLE:
                if (key == '*')
                {
                    state = DOOR_STATE_ENTER_PIN_OPEN;
                    input_len = 0;
                    input_buf[0] = '\0';
                    LCD_Clear();
                    LCD_Print("Nhap PIN:");
                    LCD_SetCursor(1, 0);
                }
                else if (key == 'A')
                {
                    state = DOOR_STATE_ENTER_PIN_ADMIN;
                    input_len = 0;
                    input_buf[0] = '\0';
                    LCD_Clear();
                    LCD_Print("PIN cu:");
                    LCD_SetCursor(1, 0);
                }
                break;

            /* =========================
               STATE: ENTER PIN TO OPEN
            ========================== */
            case DOOR_STATE_ENTER_PIN_OPEN:

                if (key == '#')
                {
                    input_buf[input_len] = '\0';

                    if (Password_Check(input_buf))
                    {
                        door_open_success();
                        attempts_remain = 3; 
                        state = DOOR_STATE_IDLE; // V? màn hình chính sau khi xong
                        input_len = 0;
                    }
                    else
                    {
                        /* --- SAI M?T KH?U --- */
                        Buzzer_Play(BUZZ_LONG);
                        taskYIELD();

                        if (attempts_remain > 0) attempts_remain--;
                        
                        /* TRU?NG H?P 1: B? KHÓA DO SAI 3 L?N */
                        if (attempts_remain == 0)
                        {
                            for (i = 30; i > 0; i--)
                            {
                                Power_Reset_Timer(); // Chong ng?
                                
                                LCD_ShowCountdown(i); 
                                vTaskDelay(pdMS_TO_TICKS(1000));
                            }

                            attempts_remain = 3;
                            xQueueReset(qKey); // Xóa phím rác b?m lúc ch?

                            // Sau khi ph?t xong, v? màn hình ch?
                            state = DOOR_STATE_IDLE;
                            door_display_idle();
                        }
                        /* TRU?NG H?P 2: SAI NHUNG V?N CÒN LU?T */
                        else
                        {
                            LCD_ShowWrongAttempts(attempts_remain); 
                            vTaskDelay(pdMS_TO_TICKS(2000)); 
                            
                            /* --- LOGIC M?I: ? L?I MÀN HÌNH NH?P PIN --- */
                            // 1. Xóa màn hình
                            LCD_Clear();
                            // 2. In l?i yêu c?u nh?p
                            LCD_Print("Nhap PIN:");
                            LCD_SetCursor(1, 0);
                            
                            // 3. Reset b? d?m nh?p li?u
                            input_len = 0;
                            input_buf[0] = '\0';
                        }
                    }
                }
                else if (key == '*') // Nút xóa / Back
                {
                    input_len = 0;
                    LCD_Clear();
                    LCD_Print("Nhap PIN:");
                    LCD_SetCursor(1, 0);
                }
                else if (key >= '0' && key <= '9')
                {
                    if (input_len < MAX_PASS_LEN)
                    {
                        input_buf[input_len++] = key;
                        temp[0] = '*'; 
                        temp[1] = '\0';
                        LCD_Print(temp);
                    }
                }
                break;

            case DOOR_STATE_ENTER_PIN_ADMIN:
                if (key == '#')
                {
                    input_buf[input_len] = '\0';
                    if (Password_Check(input_buf)) {
                         Buzzer_Play(BUZZ_DOUBLE);
                         taskYIELD();
                         state = DOOR_STATE_SET_NEW_PIN;
                         input_len = 0;
                         LCD_Clear(); LCD_Print("PIN moi:"); LCD_SetCursor(1, 0);
                    } else {
                         Buzzer_Play(BUZZ_LONG);
                         taskYIELD();
                         LCD_Clear(); LCD_PrintCenter(0, "Sai PIN Admin!");
                         vTaskDelay(pdMS_TO_TICKS(2000));
                         state = DOOR_STATE_IDLE; 
                         door_display_idle();
                    }
                    input_len = 0;
                }
                // ... (các phím khác gi? nguyên) ...
                else if (key == '*') { input_len = 0; LCD_Clear(); LCD_Print("PIN cu:"); LCD_SetCursor(1, 0); }
                else if (key >= '0' && key <= '9') {
                    if (input_len < MAX_PASS_LEN) {
                        input_buf[input_len++] = key;
                        temp[0] = '*'; temp[1] = '\0'; LCD_Print(temp);
                    }
                }
                break;

            case DOOR_STATE_SET_NEW_PIN:
                // ... (Logic d?i pin gi? nguyên) ...
                if (key == '#') {
                    input_buf[input_len] = '\0';
                    if (input_len < 4) {
                        Buzzer_Play(BUZZ_LONG); taskYIELD();
                        LCD_Clear(); LCD_Print("Qua ngan!"); vTaskDelay(pdMS_TO_TICKS(1500));
                        LCD_Clear(); LCD_Print("PIN moi:"); LCD_SetCursor(1, 0); input_len = 0;
                    } else {
                        Password_SetNew(input_buf);
                        Buzzer_Play(BUZZ_DOUBLE); taskYIELD();
                        LCD_Clear(); LCD_PrintCenter(0, "Doi thanh cong"); vTaskDelay(pdMS_TO_TICKS(2000));
                        state = DOOR_STATE_IDLE; door_display_idle();
                    }
                    input_len = 0;
                }
                else if (key == '*') { input_len = 0; LCD_Clear(); LCD_Print("PIN moi:"); LCD_SetCursor(1, 0); }
                else if (key >= '0' && key <= '9') {
                     if (input_len < MAX_PASS_LEN) {
                        input_buf[input_len++] = key;
                        temp[0] = key; temp[1] = '\0'; LCD_Print(temp);
                    }
                }
                break;

            default:
                state = DOOR_STATE_IDLE;
                input_len = 0;
                door_display_idle();
                break;
            }
        }
    }
}