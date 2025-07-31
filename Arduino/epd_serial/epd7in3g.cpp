/**
 *  @filename   :   epd7in3g.cpp
 *  @brief      :   Implements for e-paper library
 *  
 *  Based on Waveshare e-paper library
 *  Modified for XIAOePaperPanelDriverBoard
 *
 *  MIT License
 *  
 *  Copyright (c) 2025 EpaperPix
 *  Original code Copyright (c) 2017-2024 Waveshare
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifdef EPD7IN3G_C

#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480
#define EPD_STEPS       1
#define EPD_BLOCK_SIZE  96000

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
    steps = EPD_STEPS;
    blockSize = EPD_BLOCK_SIZE;
    stepCommands[0] = 0x10;
};

int Epd::Init(void) {
    /* this calls the peripheral hardware interface, see epdif */
    if (IfInit() != 0) {
        return -1;
    }
    Reset();
    WaitUntilIdle();

    SendCommand(0xAA);
    SendData(0x49);
    SendData(0x55);
    SendData(0x20);
    SendData(0x08);
    SendData(0x09);
    SendData(0x18);

    SendCommand(0x01);
    SendData(0x3F);

    SendCommand(0x00);
    SendData(0x4F);
    SendData(0x69);

    SendCommand(0x05);
    SendData(0x40);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x2C);

    SendCommand(0x08);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x22);

    //===================
    //20211212
    //First setting
    SendCommand(0x06);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x14);
    SendData(0x14);
    //===================

    SendCommand(0x03);
    SendData(0x00);
    SendData(0x54);
    SendData(0x00);
    SendData(0x44);

    SendCommand(0x60);
    SendData(0x02);
    SendData(0x00);
    //Please notice that PLL must be set for version 2 IC
    SendCommand(0x30);
    SendData(0x08);

    SendCommand(0x50);
    SendData(0x3F);

    SendCommand(0x61);
    SendData(0x03);
    SendData(0x20);
    SendData(0x01); 
    SendData(0xE0); 

    SendCommand(0xE3);
    SendData(0x2F);

    SendCommand(0x84);
    SendData(0x01);
    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

void Epd::SendDataFast(unsigned char data) {
    SpiTransfer(data);
}

void Epd::SetToDataMode() {
    DigitalWrite(dc_pin, HIGH);
}

/**
 *  @brief: Wait until the busy_pin goes HIGH
 */
void Epd::WaitUntilIdle(void) {
    while(DigitalRead(busy_pin) == LOW) {      //LOW: busy, HIGH: idle
        DelayMs(5);
    }
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);    
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);     
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
void Epd::TurnOnDisplay(void)
{
    SendCommand(0x12); // DISPLAY_REFRESH
    SendData(0x01);
    WaitUntilIdle();

    SendCommand(0x02); // POWER_OFF
    SendData(0X00);
    WaitUntilIdle();
}


void Epd::Clear()
{
    int Width, Height;
    Width = (width % 4 == 0)? (width / 4 ): (width / 4 + 1);
    Height = height;
    
    SendCommand(0x04);
    WaitUntilIdle();

    SendCommand(0x10);
    SetToDataMode();
    for (int j = 0; j < Height; j++) {
        for (int i = 0; i < Width; i++) {
            SendData(0);
        }
    }

    TurnOnDisplay();
}
/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void Epd::Clear(unsigned char color)
{
    int Width, Height;
    Width = (width % 4 == 0)? (width / 4 ): (width / 4 + 1);
    Height = height;
    
    SendCommand(0x04);
    WaitUntilIdle();

    SendCommand(0x10);
    SetToDataMode();
    for (int j = 0; j < Height; j++) {
        for (int i = 0; i < Width; i++) {
            SendData((color<<6) | (color<<4) | (color<<2) | color);
        }
    }

    TurnOnDisplay();
}


/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void Epd::Sleep(void)
{
    SendCommand(0x02); // POWER_OFF
    SendData(0X00);
    SendCommand(0x07); // DEEP_SLEEP
    SendData(0XA5);
}

#endif

/* END OF FILE */