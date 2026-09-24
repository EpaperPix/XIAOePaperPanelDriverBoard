/*****************************************************************************
* | File      	:   DEV_Config.cpp
* | Author      :   Waveshare team (unified multi-board copy for the 2.9inch detector)
* | Function    :   Hardware underlying interface
* | Info        :   Bit-banged SPI, no <SPI.h>. CS is driven by the drivers, not here.
*----------------
* |	This version:   V1.0
* | Date        :   2026-09-10
******************************************************************************/
#include "DEV_Config.h"

uint32_t DEV_BusyTimeoutMs    = 5000;
uint8_t  DEV_BusyTimeoutCount = 0;

/******************************************************************************
function:	GPIO config
******************************************************************************/
static void GPIO_Config(void)
{
    pinMode(EPD_BUSY_PIN, INPUT);
    pinMode(EPD_RST_PIN,  OUTPUT);
    pinMode(EPD_DC_PIN,   OUTPUT);
    pinMode(EPD_SCK_PIN,  OUTPUT);
    pinMode(EPD_MOSI_PIN, OUTPUT);
    pinMode(EPD_CS_PIN,   OUTPUT);
#ifdef EPD_PWR_PIN
    pinMode(EPD_PWR_PIN,  OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
#endif
    digitalWrite(EPD_CS_PIN,  HIGH);
    digitalWrite(EPD_SCK_PIN, LOW);
    digitalWrite(EPD_RST_PIN, HIGH);
}

/******************************************************************************
function:	Module Initialize, the BCM2835 library and initialize the pins, SPI protocol
******************************************************************************/
UBYTE DEV_Module_Init(void)
{
    GPIO_Config();
    Serial.begin(115200);
    return 0;
}

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode)
{
    if(Mode == 0) {
        pinMode(GPIO_Pin, INPUT);
    } else {
        pinMode(GPIO_Pin, OUTPUT);
    }
}

/******************************************************************************
function:	SPI write one byte, MSB first, mode 0, bit-banged
******************************************************************************/
void DEV_SPI_WriteByte(UBYTE data)
{
    for (int i = 0; i < 8; i++) {
        if ((data & 0x80) == 0) digitalWrite(EPD_MOSI_PIN, LOW);
        else                    digitalWrite(EPD_MOSI_PIN, HIGH);
        data <<= 1;
        digitalWrite(EPD_SCK_PIN, HIGH);
        digitalWrite(EPD_SCK_PIN, LOW);
    }
}

void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len)
{
    for (UDOUBLE i = 0; i < len; i++)
        DEV_SPI_WriteByte(pData[i]);
}

/******************************************************************************
function:	Detector helpers
******************************************************************************/
void DEV_ResetBusyStats(void)
{
    DEV_BusyTimeoutCount = 0;
}

uint8_t DEV_WaitBusy(uint8_t busy_level, void (*poll)(void))
{
    uint32_t limit = DEV_BusyTimeoutMs;
    if (DEV_BusyTimeoutCount > 0 && limit > DEV_BUSY_FAILFAST_MS)
        limit = DEV_BUSY_FAILFAST_MS;

    uint32_t start = millis();
    for (;;) {
        if (poll) poll();
        if (digitalRead(EPD_BUSY_PIN) != busy_level)
            return 1;
        if (millis() - start >= limit) {
            if (DEV_BusyTimeoutCount < 255) DEV_BusyTimeoutCount++;
            return 0;
        }
        delay(10);
    }
}

void DEV_HW_Reset(uint16_t high_ms, uint16_t low_ms)
{
    digitalWrite(EPD_RST_PIN, HIGH);
    delay(high_ms);
    digitalWrite(EPD_RST_PIN, LOW);
    delay(low_ms);
    digitalWrite(EPD_RST_PIN, HIGH);
    delay(high_ms);
}

void DEV_SendCommand(UBYTE Reg)
{
    digitalWrite(EPD_DC_PIN, LOW);
    digitalWrite(EPD_CS_PIN, LOW);
    DEV_SPI_WriteByte(Reg);
    digitalWrite(EPD_CS_PIN, HIGH);
}

void DEV_Module_Exit(void)
{
#ifdef EPD_PWR_PIN
    digitalWrite(EPD_PWR_PIN, LOW);
#endif
}
