/**
 *  @filename   :   epd2in13_V3.cpp
 *  @brief      :   2.13inch e-paper V3
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

#include "display_config.h"
#ifdef EPD2IN13_V3_C

#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       122
#define EPD_HEIGHT      250
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  4000  // 16 bytes per 122-px row * 250 rows

// SSD1680 RAM window and address counter (Waveshare EPD_2in13_V3_SetWindows/SetCursor).
// Restored before every WRITE_RAM (see SendCommand) so each frame, and a retry after a
// partial download, starts at RAM 0,0.
static void writeCommandRaw(unsigned char command) {
    EpdIf::DigitalWrite(DC_PIN, LOW);
    EpdIf::SpiTransfer(command);
}
static void writeDataRaw(unsigned char data) {
    EpdIf::DigitalWrite(DC_PIN, HIGH);
    EpdIf::SpiTransfer(data);
}
static void setWindowAndCursor() {
    writeCommandRaw(0x44);              // RAM X start/end, in bytes
    writeDataRaw(0x00);
    writeDataRaw(((EPD_WIDTH - 1) >> 3) & 0xFF);
    writeCommandRaw(0x45);              // RAM Y start/end
    writeDataRaw(0x00);
    writeDataRaw(0x00);
    writeDataRaw((EPD_HEIGHT - 1) & 0xFF);
    writeDataRaw(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    writeCommandRaw(0x4E);              // RAM X counter
    writeDataRaw(0x00);
    writeCommandRaw(0x4F);              // RAM Y counter
    writeDataRaw(0x00);
    writeDataRaw(0x00);
}

Epd::~Epd()
{
};

Epd::Epd()
{
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
    steps = EPD_STEPS;
    blockSize = EPD_BLOCK_SIZE;
    stepCommands[0] = 0x24;
};

void Epd::SendCommand(unsigned char command)
{
    if (command == 0x24) setWindowAndCursor();
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}

void Epd::SendData(unsigned char data)
{
    DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

void Epd::SendDataFast(unsigned char data)
{
    SpiTransfer(data);
}

void Epd::SetToDataMode() {
    DigitalWrite(dc_pin, HIGH);
}

void Epd::WaitUntilIdle(void)
{
    while(1) {      //LOW: idle, HIGH: busy
        if(DigitalRead(busy_pin) == 0)
            break;
        DelayMs(10);
    }
}

int Epd::Init(void)
{
    if (IfInit() != 0) {
        return -1;
    }
    
    Reset();
    
    WaitUntilIdle();
    SendCommand(0x12); // soft reset
    WaitUntilIdle();

    SendCommand(0x01); //Driver output control
    SendData(0xF9);
    SendData(0x00);
    SendData(0x00);

    SendCommand(0x11); //data entry mode
    SendData(0x03);

    setWindowAndCursor();

    SendCommand(0x3C); //BorderWavefrom
    SendData(0x05);	

    SendCommand(0x21); //  Display update control
    SendData(0x00);
    SendData(0x80);	

    SendCommand(0x18); //Read built-in temperature sensor
    SendData(0x80);	

    WaitUntilIdle();

    return 0;
}

void Epd::Reset(void)
{
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);
    DigitalWrite(reset_pin, LOW);                //module reset
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);
}

void Epd::TurnOnDisplay(void) {
    SendCommand(0x22);
    SendData(0xC7);
    SendCommand(0x20);
    WaitUntilIdle();
}

void Epd::ClearFrame()
{
       int w, h;
    w = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    h = EPD_HEIGHT;
    SendCommand(0x24);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            SendData(0xff);
        }
    }
}

void Epd::Clear(unsigned char color)
{
    TurnOnDisplay();
}

void Epd::Sleep()
{
    SendCommand(0x10); //enter deep sleep
    SendData(0x01);
    DelayMs(200);

    DigitalWrite(reset_pin, LOW);
}

#endif

/* END OF FILE */