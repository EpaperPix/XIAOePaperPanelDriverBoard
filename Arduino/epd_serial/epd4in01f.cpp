/**
 *  @filename   :   epd4in01f.cpp
 *  @brief      :   Implements for 4.01inch e-paper library
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
#ifdef EPD4IN01F_C
#include <stdlib.h>
#include "epd_base.h"
#include "qrset.cpp"

// Display resolution
#define EPD_WIDTH       640
#define EPD_HEIGHT      400
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  128000

// Pixel format for this display
#define EPD_BITS_PER_PIXEL 4    // Color: 4 bits per pixel
#define EPD_PIXELS_PER_BYTE 2   // Each byte contains 2 pixels

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
    stepCommands[0] = 0x10;
    blockSize = EPD_BLOCK_SIZE;
    ShowDebug = false;
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();
    WaitUntilIdle();
    SendCommand(0x00);
    SendData(0x2f);
    SendData(0x00);
    SendCommand(0x01);
    SendData(0x37);
    SendData(0x00);
    SendData(0x05);
    SendData(0x05);
    SendCommand(0x03);
    SendData(0x00);
    SendCommand(0x06);
    SendData(0xC7);
    SendData(0xC7);
    SendData(0x1D);
    SendCommand(0x41);
    SendData(0x00);
    SendCommand(0x50);
    SendData(0x37);
    SendCommand(0x60);
    SendData(0x22);
    SendCommand(0x61);
    SendData(0x02);
    SendData(0x80);
    SendData(0x01);
    SendData(0x90);
    SendCommand(0xE3);
    SendData(0xAA);
    return 0;
}

void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

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

void Epd::WaitUntilIdle(void) {
    while(!(DigitalRead(busy_pin)));
}

void Epd::Reset(void) {
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);   
    DigitalWrite(reset_pin, LOW);
    DelayMs(1);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
}

void Epd::TurnOnDisplay(void) {
    SendCommand(0x04);
    WaitUntilIdle();
    SendCommand(0x12);
    WaitUntilIdle();
    SendCommand(0x02);
    while(DigitalRead(busy_pin));
    DelayMs(200);
}

void Epd::Clear(unsigned char color) {
    SendCommand(0x61);
    SendData(0x02);
    SendData(0x80);
    SendData(0x01);
    SendData(0x90);
    SendCommand(0x10);
    for(int i=0; i<height; i++) {
        for(int j=0; j<width/2; j++) {
            SendData((color<<4)|color);
        }
    }
    TurnOnDisplay();
}

void Epd::Sleep(void) {
    DelayMs(100);
    SendCommand(0x07);
    SendData(0xA5);
    DelayMs(100);
    DigitalWrite(reset_pin, 0);
}

void Epd::ClearFrame(void) {
    SendCommand(0x61);
    SendData(0x02);
    SendData(0x80);
    SendData(0x01);
    SendData(0x90);
    SendCommand(0x10);
    for(int i=0; i<height; i++) {
        for(int j=0; j<width/2; j++) {
            SendData(0x11);
        }
    }
}

void Epd::SetLut(void) {
}

void Epd::SetLut_by_host(unsigned char *lut_vcom, unsigned char *lut_ww, unsigned char *lut_bw, unsigned char *lut_wb, unsigned char *lut_bb) {
}

void Epd::SetFrameStart(char mode, unsigned char command) {
    SendCommand(command);
}

void Epd::SendBuffer(unsigned char* buffer, int size) {
    for(int i = 0; i < size; i++) {
        SendDataFast(buffer[i]);
    }
}


#endif

/* END OF FILE */