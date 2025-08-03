/**
 *  @filename   :   epd3in97g.cpp
 *  @brief      :   3.97inch e-paper (G)
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
#ifdef EPD3IN97G_C

#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480
// Initialization must use this resolution
#define EPD_HEIGHT_INIT  680

#define EPD_STEPS       1
#define EPD_BLOCK_SIZE  96000
// Pixel format for this display
#define EPD_BITS_PER_PIXEL 2    // Color: 4 bits per pixel
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

    SendCommand(0x00);	//0x00
    SendData(0x2B);	
    SendData(0x29);	

    SendCommand(0x06);	//0x06
    SendData(0x0F);	
    SendData(0x8B);	
    SendData(0x93);	
    SendData(0xC1);

    SendCommand(0x50);	//0x50
    SendData(0x37);	

    SendCommand(0x30);	//0x30
    SendData(0x08);	

    SendCommand(0x61);//0x61	
    SendData(EPD_WIDTH/256);	
    SendData(EPD_WIDTH%256);	
    SendData(EPD_HEIGHT_INIT/256);	
    SendData(EPD_HEIGHT_INIT%256);	

    SendCommand(0x62);
    SendData(0x76); 
    SendData(0x76);
    SendData(0x76); 
    SendData(0x5A);
    SendData(0x9D); 
    SendData(0x8A);	
    SendData(0x76); 
    SendData(0x62); 

    SendCommand(0x65);	//0x65
    SendData(0x00);	
    SendData(0x00);	
    SendData(0x00);	
    SendData(0x00);	

    SendCommand(0xE0);	//0xE3
    SendData(0x10);	

    SendCommand(0xE7);	//0xE7
    SendData(0xA4);	

    SendCommand(0xE9);	
    SendData(0x01);

    SendCommand(0x04); //Power on
    WaitUntilIdle();          //waiting for the electronic paper IC to release the idle signal
    
    
    
    
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
