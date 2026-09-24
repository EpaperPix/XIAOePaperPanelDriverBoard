/*****************************************************************************
* | File      	:   EPD_2in9_DES.cpp
* | Author      :   Waveshare team (Arduino port for the 2.9inch detector)
* | Function    :   2.9inch e-paper DES
* | Info        :   Ported from RaspberryPi_JetsonNano/c/lib/e-Paper/EPD_2in9_DES.c
*----------------
* |	This version:   V1.0
* | Date        :   2026-09-10
* | Info        :   ReadBusy bounded by DEV_WaitBusy (see DEV_Config.h)
******************************************************************************/
#include "EPD_2in9_DES.h"

/******************************************************************************
function :	Software reset
******************************************************************************/
static void EPD_2IN9_DES_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(100);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(2);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(100);
}

/******************************************************************************
function :	send command
******************************************************************************/
static void EPD_2IN9_DES_SendCommand(UBYTE Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
******************************************************************************/
static void EPD_2IN9_DES_SendData(UBYTE Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

static void EPD_2IN9_DES_Poll71(void)
{
    EPD_2IN9_DES_SendCommand(0x71);    // Get Status: refreshes the BUSY pin
}

/******************************************************************************
function :	Wait until the busy_pin goes HIGH
******************************************************************************/
void EPD_2IN9_DES_ReadBusy(void)
{
    DEV_WaitBusy(0, EPD_2IN9_DES_Poll71);    // UC8151: busy while LOW
    DEV_Delay_ms(50);
}

/******************************************************************************
function :	Turn On Display
******************************************************************************/
static void EPD_2IN9_DES_TurnOnDisplay(void)
{
    EPD_2IN9_DES_SendCommand(0x12);    // Display refresh
    EPD_2IN9_DES_ReadBusy();
}

/******************************************************************************
function :	Initialize the e-Paper register
******************************************************************************/
void EPD_2IN9_DES_Init(void)
{
    EPD_2IN9_DES_Reset();
    DEV_Delay_ms(100);

    EPD_2IN9_DES_SendCommand(0x04);    // Power on
    EPD_2IN9_DES_ReadBusy();

    EPD_2IN9_DES_SendCommand(0x00);    // Panel setting: LUT from OTP, B/W
    EPD_2IN9_DES_SendData(0x1f);

    EPD_2IN9_DES_SendCommand(0x50);    // VCOM and data interval
    EPD_2IN9_DES_SendData(0x97);
}

/******************************************************************************
function :	Clear screen
******************************************************************************/
void EPD_2IN9_DES_Clear(void)
{
    UWORD Width = (EPD_2IN9_DES_WIDTH % 8 == 0)? (EPD_2IN9_DES_WIDTH / 8 ): (EPD_2IN9_DES_WIDTH / 8 + 1);
    UWORD Height = EPD_2IN9_DES_HEIGHT;

    EPD_2IN9_DES_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9_DES_SendData(0xff);
        }
    }
    EPD_2IN9_DES_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
******************************************************************************/
void EPD_2IN9_DES_Display(UBYTE *Image)
{
    UWORD Width = (EPD_2IN9_DES_WIDTH % 8 == 0)? (EPD_2IN9_DES_WIDTH / 8 ): (EPD_2IN9_DES_WIDTH / 8 + 1);
    UWORD Height = EPD_2IN9_DES_HEIGHT;

    EPD_2IN9_DES_SendCommand(0x13);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            EPD_2IN9_DES_SendData(Image[i + j * Width]);
        }
    }
    EPD_2IN9_DES_TurnOnDisplay();
}

/******************************************************************************
function :	Enter sleep mode
******************************************************************************/
void EPD_2IN9_DES_Sleep(void)
{
    EPD_2IN9_DES_SendCommand(0x50);
    EPD_2IN9_DES_SendData(0xf7);
    EPD_2IN9_DES_SendCommand(0x02);    // Power off
    EPD_2IN9_DES_ReadBusy();
    EPD_2IN9_DES_SendCommand(0x07);    // Deep sleep
    EPD_2IN9_DES_SendData(0xa5);
}
