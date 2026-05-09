#include "password.h"
#include <string.h>

/* M?t kh?u m?c d?nh luu trong Flash (const) */
static const char g_default_password[] = "1234";

/* M?t kh?u hi?n t?i trong RAM */
char g_current_password[MAX_PASS_LEN + 1];

void Password_Init(void)
{
    memset(g_current_password, 0, sizeof(g_current_password));
    strncpy(g_current_password, g_default_password, MAX_PASS_LEN);
    g_current_password[MAX_PASS_LEN] = '\0';
}

uint8_t Password_Check(const char *input)
{
    if (input == 0)
    {
        return 0;
    }
    if (strncmp(input, g_current_password, MAX_PASS_LEN) == 0)
    {
        return 1;
    }
    return 0;
}

void Password_SetNew(const char *new_pass)
{
    size_t len;
    size_t i;

    if (new_pass == 0) return;

    len = strlen(new_pass);
    if (len > MAX_PASS_LEN)
    {
        len = MAX_PASS_LEN;
    }

    memset(g_current_password, 0, sizeof(g_current_password));

    for (i = 0; i < len; i++)
    {
        g_current_password[i] = new_pass[i];
    }
    g_current_password[len] = '\0';
}
