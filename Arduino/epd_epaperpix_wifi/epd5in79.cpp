/**
 *  @filename   :   epd5in79.cpp
 *  @brief      :   Implements for Monochrome e-paper library
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

#ifdef EPD5IN79_C

#include <stdlib.h>
#include "epd_base.h"

// EPD5IN79 commands
#define DATA_ENTRY_MODE_SETTING                     0x11
#define POWER_ON                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x18
#define TEMPERATURE_SENSOR_WRITE                    0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL                      0x22
#define WRITE_RAM_BW_M                              0x24
#define WRITE_RAM_RED_M                             0x26
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define ANALOG_BLOCK_CONTROL                        0x74
#define DIGITAL_BLOCK_CONTROL                       0x7E
#define DISPLAY_OPTION                              0x91
#define WRITE_RAM_BW_S                              0xA4
#define WRITE_RAM_RED_S                             0xA6
#define SET_RAM_X_ADDRESS_START_END_POSITION_S      0xC4
#define SET_RAM_Y_ADDRESS_START_END_POSITION_S      0xC5
#define SET_RAM_X_ADDRESS_COUNTER_S                 0xCE
#define SET_RAM_Y_ADDRESS_COUNTER_S                 0xCF
#define DEEP_SLEEP_MODE                             0x10

// Display resolution
#define EPD_WIDTH       792
#define EPD_HEIGHT      272
#define EPD_STEPS      2
#define EPD_BLOCK_SIZE  13600 // 6800 //  6732  // 792 * 272 / 8 / 4 (split into 4 parts)

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
    stepCommands[0] = 0x24;
    stepCommands[1] = 0xA4;
   //  stepCommands[2] = 0xA4;
  //  stepCommands[3] = 0XA6;
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
     Serial.print("Before Reset\r\n");
    Reset();
    Serial.print("After Reset\r\n");
    WaitUntilIdle();
    SendCommand(POWER_ON);
    WaitUntilIdle();

    SendCommand(DATA_ENTRY_MODE_SETTING);
    SendData(0x01);

    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    SendData(0x00);
    SendData(0x31); //400/8-1

    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    SendData(0x0f);
    SendData(0x01);  //300-1
    SendData(0x00);
    SendData(0x00);

    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    SendData(0x00);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    SendData(0x0f);
    SendData(0x01);

    WaitUntilIdle();

    SendCommand(DISPLAY_OPTION);
    SendData(0x00);

    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION_S);
    SendData(0x31);
    SendData(0x00); //400/8-1

    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION_S);
    SendData(0x0f);
    SendData(0x01);  //300-1
    SendData(0x00);
    SendData(0x00);

    SendCommand(SET_RAM_X_ADDRESS_COUNTER_S);
    SendData(0x31);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER_S);
    SendData(0x0f);
    SendData(0x01);

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
   while(DigitalRead(busy_pin) == 1) {      //0: busy, 1: idle
        DelayMs(100);
         
    }           
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, LOW);
    DelayMs(200);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL);
    SendData(0xF7);
    SendCommand(MASTER_ACTIVATION);
    WaitUntilIdle();
}

void Epd::SetLut(void) {
    // No LUT setting needed for this display
}

void Epd::ClearFrame() {
    // M part (main section)
    SendCommand(WRITE_RAM_BW_M);
    for(int i = 0; i < 13600; i++) {
        SendData(0xFF);
    }

    SendCommand(WRITE_RAM_RED_M);
    for(int i = 0; i < 13600; i++) {
        SendData(0x00);
    }

    // S part (secondary section)
    SendCommand(WRITE_RAM_BW_S);
    for(int i = 0; i < 13600; i++) {
        SendData(0xFF);
    }

    SendCommand(WRITE_RAM_RED_S);
    for(int i = 0; i < 13600; i++) {
        SendData(0x00);
    }
}

void Epd::Clear(unsigned char color) {
    ClearFrame();
    TurnOnDisplay();
}

/**
 * @brief: After this command is transmitted, the chip would enter the deep-sleep mode to save power. 
 *         The deep sleep mode would return to standby by hardware reset. The only one parameter is a 
 *         check code, the command would be executed if check code = 0xA5. 
 *         You can use Epd::Reset() to awaken and use Epd::Init() to initialize.
 */
void Epd::Sleep(void) {
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0x01);
}

#endif

/* END OF FILE */