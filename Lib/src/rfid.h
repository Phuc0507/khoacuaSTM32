#ifndef __RFID_H
#define __RFID_H

#include "stm32f10x.h"
#include <stdint.h>

#define RFID_UID_SIZE      4
#define RFID_MAX_CARDS     16

void RFID_Init(void);
uint8_t RC522_CheckCard(uint8_t *uid);

uint8_t RFID_IsAuthorized(const uint8_t *uid);
uint8_t RFID_AddCard(const uint8_t *uid);
uint8_t RFID_DeleteCard(const uint8_t *uid);

#endif
