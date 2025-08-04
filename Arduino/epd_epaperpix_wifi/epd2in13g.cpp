/**
 *  @filename   :   epd2in13g.cpp
 *  @brief      :   2.13inch e-paper (G) - 4 color display
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

#ifdef EPD2IN13G_C

#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       128
#define EPD_HEIGHT      250

#define EPD_STEPS       1
#define EPD_BLOCK_SIZE  8000 // 7625  // (122 * 250) / 4 (4 pixels per byte for 2-bit color)

// Pixel format for this display
#define EPD_BITS_PER_PIXEL 2    // 4 colors: 2 bits per pixel
#define EPD_PIXELS_PER_BYTE 4   // Each byte contains 4 pixels

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
    bits_per_pixel = EPD_BITS_PER_PIXEL;
    pixels_per_byte = EPD_PIXELS_PER_BYTE;
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
    
    SendCommand(0x4D);
    SendData(0x78);

    SendCommand(0x00);	//PSR
    SendData(0x0F);
    SendData(0x29);

    SendCommand(0x01);	//PWRR
    SendData(0x07);
    SendData(0x00);

    SendCommand(0x03);	//POFS
    SendData(0x10);
    SendData(0x54);
    SendData(0x44);

    SendCommand(0x06);	//BTST_P
    SendData(0x05);
    SendData(0x00);
    SendData(0x3F);
    SendData(0x0A);
    SendData(0x25);
    SendData(0x12);
    SendData(0x1A); 

    SendCommand(0x50);	//CDI
    SendData(0x37);

    SendCommand(0x60);	//TCON
    SendData(0x02);
    SendData(0x02);

    SendCommand(0x61); //TRES - resolution setting
    SendData(width/256);
    SendData(width%256);
    SendData(height/256);
    SendData(height%256);

    SendCommand(0xE7);
    SendData(0x1C);

    SendCommand(0xE3);	
    SendData(0x22);

    SendCommand(0xB4);
    SendData(0xD0);
    SendCommand(0xB5);
    SendData(0x03);

    SendCommand(0xE9);
    SendData(0x01); 

    SendCommand(0x30);
    SendData(0x08);  

    SendCommand(0x04);
    WaitUntilIdle(); 
    
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
    DelayMs(200);
    DigitalWrite(reset_pin, LOW);
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
void Epd::TurnOnDisplay(void)
{
    SendCommand(0x12); // DISPLAY_REFRESH
    SendData(0x00);
    WaitUntilIdle();
}

void Epd::ClearFrame()
{
  
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

    SendCommand(0x10);
    SetToDataMode();
    for (int j = 0; j < Height; j++) {
        for (int i = 0; i < Width; i++) {
            SendData((color << 6) | (color << 4) | (color << 2) | color);
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
    SendCommand(0x02); //power off
    WaitUntilIdle();       //waiting for the electronic paper IC to release the idle signal
    DelayMs(100);           //!!!The delay here is necessary,100mS at least!!! 
    
    SendCommand(0x07);  //deep sleep
    SendData(0xA5);
}

#endif