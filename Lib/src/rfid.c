#include "rfid.h"
#include "stm32f10x_flash.h"
#include "delay.h"
#include <string.h>

/* ==========================================================
   RC522 REGISTERS / COMMANDS
   ========================================================== */
#define PCD_RESETPHASE      0x0F
#define CommandReg          0x01
#define CommIEnReg          0x02
#define CommIrqReg          0x04
#define ErrorReg            0x06
#define Status2Reg          0x08
#define FIFODataReg         0x09
#define FIFOLevelReg        0x0A
#define ControlReg          0x0C
#define BitFramingReg       0x0D
#define ModeReg             0x11
#define TxControlReg        0x14

#define TxASKReg            0x15
#define TModeReg            0x2A
#define TPrescalerReg       0x2B
#define TReloadRegL         0x2C
#define TReloadRegH         0x2D
#define RFCfgReg            0x26

#define VersionReg          0x37

/* Command */
#define PCD_IDLE            0x00
#define PCD_AUTHENT         0x0E
#define PCD_RECEIVE         0x08
#define PCD_TRANSMIT        0x04
#define PCD_TRANSCEIVE      0x0C
#define PCD_SOFTRESET       0x0F

/* PICC command */
#define PICC_REQIDL         0x26
#define PICC_ANTICOLL       0x93

/* Chip Select: PA4  / RST: PA3 */
#define CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

#define RST_LOW()  GPIO_ResetBits(GPIOA, GPIO_Pin_3)
#define RST_HIGH() GPIO_SetBits(GPIOA, GPIO_Pin_3)

/* ==========================================================
   EEPROM LAYOUT
   ========================================================== */
#define RFID_EE_BASE_ADDR  ((uint32_t)0x0800FC00u)
#define RFID_EE_MAGIC      0xA5

static uint8_t s_cardCount = 0;
static uint8_t s_cardUID[RFID_MAX_CARDS][RFID_UID_SIZE];

/* ==========================================================
   SPI LOW LEVEL
   ========================================================== */
static uint8_t RC522_SPI_WriteRead(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

static void RC522_Write(uint8_t reg, uint8_t value)
{
    CS_LOW();
    RC522_SPI_WriteRead((reg << 1) & 0x7E);
    RC522_SPI_WriteRead(value);
    CS_HIGH();
}

static uint8_t RC522_Read(uint8_t reg)
{
    uint8_t value;
    CS_LOW();
    RC522_SPI_WriteRead(((reg << 1) & 0x7E) | 0x80);
    value = RC522_SPI_WriteRead(0x00);
    CS_HIGH();
    return value;
}

/* ==========================================================
   ToCard – G?i l?nh PICC
   ========================================================== */
static uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                            uint8_t *backData, uint8_t *backLen)
{
    uint32_t i;
    uint8_t n;
    uint8_t irqEn = 0, waitIRq = 0;
    uint8_t status = 0;

    if (command == PCD_TRANSCEIVE)
    {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    RC522_Write(CommIEnReg, irqEn | 0x80);
    RC522_Write(CommIrqReg, 0x7F);
    RC522_Write(FIFOLevelReg, 0x80);

    for (i = 0; i < sendLen; i++)
        RC522_Write(FIFODataReg, sendData[i]);

    RC522_Write(CommandReg, command);

    if (command == PCD_TRANSCEIVE)
        RC522_Write(BitFramingReg, 0x80);

    i = 2000;
    do
    {
        n = RC522_Read(CommIrqReg);
        i--;
    }
    while (i && !(n & 0x01) && !(n & waitIRq));

    if (i)
    {
        if (!(RC522_Read(ErrorReg) & 0x1B))
        {
            status = 1;

            if (command == PCD_TRANSCEIVE)
            {
                n = RC522_Read(FIFOLevelReg);
                *backLen = n;

                for (i = 0; i < n; i++)
                    backData[i] = RC522_Read(FIFODataReg);
            }
        }
    }

    RC522_Write(BitFramingReg, 0x00);
    return status;
}

/* ==========================================================
   RC522_Config – Tuning RF
   ========================================================== */
static void RC522_Config(void)
{
    RC522_Write(TModeReg,       0x8D);
    RC522_Write(TPrescalerReg,  0x3E);
    RC522_Write(TReloadRegL,    30);
    RC522_Write(TReloadRegH,    0);

    RC522_Write(TxASKReg,       0x40);
    RC522_Write(RFCfgReg,       0x7F);
}

/* ==========================================================
   RC522_Init – SPI + Reset + RF init
   ========================================================== */
static void RC522_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &gpio);

    CS_HIGH();
    RST_HIGH();

    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);

    RC522_Write(CommandReg, PCD_RESETPHASE);
    Delay_Ms(50);

    RC522_Write(ModeReg, 0x3D);

    RC522_Config();

    if (!(RC522_Read(TxControlReg) & 0x03))
        RC522_Write(TxControlReg, RC522_Read(TxControlReg) | 0x03);

    RC522_Write(FIFOLevelReg, 0x80);
}

/* ==========================================================
   API: CHECK CARD (REQ + ANTICOLL)
   ========================================================== */
uint8_t RC522_CheckCard(uint8_t *uid)
{
    uint8_t buf[2];
    uint8_t backData[32];
    uint8_t backLen;
    uint8_t status;

    buf[0] = PICC_REQIDL;
    status = RC522_ToCard(PCD_TRANSCEIVE, buf, 1, backData, &backLen);
    if (!status || backLen != 2)
        return 0;

    buf[0] = PICC_ANTICOLL;
    buf[1] = 0x20;

    status = RC522_ToCard(PCD_TRANSCEIVE, buf, 2, backData, &backLen);
    if (!status)
        return 0;

    uid[0] = backData[0];
    uid[1] = backData[1];
    uid[2] = backData[2];
    uid[3] = backData[3];

    return 1;
}

/* ==========================================================
   EEPROM (FLASH)
   ========================================================== */
static void RFID_LoadFromEEPROM(void)
{
    uint32_t addr = RFID_EE_BASE_ADDR;
    volatile uint8_t *p = (volatile uint8_t*)addr;
    uint8_t i, j;

    if (p[0] != RFID_EE_MAGIC)
    {
        s_cardCount = 0;
        memset(s_cardUID, 0, sizeof(s_cardUID));
        return;
    }

    s_cardCount = p[1];
    if (s_cardCount > RFID_MAX_CARDS) s_cardCount = RFID_MAX_CARDS;

    p = (volatile uint8_t*)(addr + 2);

    for (i = 0; i < s_cardCount; i++)
        for (j = 0; j < RFID_UID_SIZE; j++)
            s_cardUID[i][j] = p[i * RFID_UID_SIZE + j];
}

static void RFID_SaveToEEPROM(void)
{
    uint8_t buf[2 + RFID_MAX_CARDS * RFID_UID_SIZE];
    uint32_t len;
    uint32_t i;
    uint16_t hw;

    buf[0] = RFID_EE_MAGIC;
    buf[1] = s_cardCount;

    for (i = 0; i < s_cardCount; i++)
        memcpy(&buf[2 + i * RFID_UID_SIZE], s_cardUID[i], RFID_UID_SIZE);

    len = 2 + s_cardCount * RFID_UID_SIZE;

    if (len & 1)
    {
        buf[len] = 0;
        len++;
    }

    FLASH_Unlock();
    FLASH_ErasePage(RFID_EE_BASE_ADDR);

    for (i = 0; i < len; i += 2)
    {
        hw = buf[i] | (buf[i+1] << 8);
        FLASH_ProgramHalfWord(RFID_EE_BASE_ADDR + i, hw);
    }

    FLASH_Lock();
}

/* ==========================================================
   CARD LIST MANAGEMENT
   ========================================================== */
uint8_t RFID_IsAuthorized(const uint8_t *uid)
{
    uint8_t i;
    for (i = 0; i < s_cardCount; i++)
        if (memcmp(uid, s_cardUID[i], RFID_UID_SIZE) == 0)
            return 1;
    return 0;
}

uint8_t RFID_AddCard(const uint8_t *uid)
{
    uint8_t i;
    if (RFID_IsAuthorized(uid)) return 1;
    if (s_cardCount >= RFID_MAX_CARDS) return 0;

    for (i = 0; i < RFID_UID_SIZE; i++)
        s_cardUID[s_cardCount][i] = uid[i];

    s_cardCount++;
    RFID_SaveToEEPROM();
    return 1;
}

uint8_t RFID_DeleteCard(const uint8_t *uid)
{
    uint8_t i, j;

    for (i = 0; i < s_cardCount; i++)
    {
        if (memcmp(uid, s_cardUID[i], RFID_UID_SIZE) == 0)
        {
            for (j = 0; j < RFID_UID_SIZE; j++)
                s_cardUID[i][j] = s_cardUID[s_cardCount - 1][j];

            s_cardCount--;
            RFID_SaveToEEPROM();
            return 1;
        }
    }
    return 0;
}

/* ==========================================================
   INIT T?NG
   ========================================================== */
void RFID_Init(void)
{
    RC522_Init();
    RFID_LoadFromEEPROM();
}
