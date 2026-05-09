#ifndef __PASSWORD_H__
#define __PASSWORD_H__

#include <stdint.h>

#define MAX_PASS_LEN        8
#define PASS_MAX_ATTEMPTS   5
#define PASS_LOCK_TIME_MS   30000    // 30s

/* ============================================================
   API KH?I T?O – KI?M TRA – THAY Ð?I M?T KH?U
   ============================================================ */
void    Password_Init(void);

/* 
   Password_Check() tr? v?:
   1 ? dúng
   0 ? sai
   2 ? dang b? khóa không du?c nh?p
*/
uint8_t Password_Check(const char *input);

/* d?i m?t kh?u */
void    Password_SetNew(const char *new_pass);

/* ki?m tra có b? khóa hay không */
uint8_t Password_IsLocked(void);

/* th?i gian khóa còn l?i (giây) */
uint32_t Password_LockRemaining(void);

/* l?y s? l?n nh?p sai hi?n t?i (dùng trong keypad task n?u c?n) */
extern uint8_t g_wrong_attempts;

#endif
