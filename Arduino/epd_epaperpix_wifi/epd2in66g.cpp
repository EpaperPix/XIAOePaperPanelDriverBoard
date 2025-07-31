/**
 *  @filename   :   epd2in66g.cpp
 *  @brief      :   Implements for 4-color e-paper library
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

#ifdef EPD2IN66G_C

#include <stdlib.h>
#include "epd_base.h"

// EPD2IN66G commands
#define PANEL_SETTING_REGISTER                      0x00
#define POWER_SETTING_REGISTER                      0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP_MODE                             0x07
#define DATA_START_TRANSMISSION                     0x10
#define DISPLAY_REFRESH                             0x12
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_CALIBRATION                     0x40
#define TEMPERATURE_SENSOR_ENABLE                   0x41
#define VCOM_DATA_INTERVAL_SETTING                  0x50
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define SPI_FLASH_CONTROL                           0x65
#define REVISION                                    0x70
#define GET_STATUS                                  0x71
#define AUTO_MEASUREMENT_VCOM                       0x80
#define READ_VCOM_VALUE                             0x81
#define VCM_DC_SETTING                              0x82
#define PARTIAL_WINDOW                              0x90
#define PARTIAL_IN                                  0x91
#define PARTIAL_OUT                                 0x92
#define PROGRAM_MODE                                0xA0
#define ACTIVE_PROGRAMMING                          0xA1
#define READ_OTP                                    0xA2
#define POWER_SAVING                                0xE3
#define LUT_FOR_VCOM                                0xE7
#define FORCE_TEMPERATURE                           0xE5

// Display resolution
#define EPD_WIDTH       184
#define EPD_HEIGHT      360
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  16560  // 152 * 296 / 4 (4 colors per byte)

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
    if (IfInit() != 0) {
        return -1;
    }
    Reset();
    WaitUntilIdle();

    SendCommand(0x4D);
    SendData(0x78);

    SendCommand(PANEL_SETTING_REGISTER);
    SendData(0x0F);
    SendData(0x29);

    SendCommand(POWER_SETTING_REGISTER);
    SendData(0x07);
    SendData(0x00);

    SendCommand(POWER_OFF_SEQUENCE_SETTING);
    SendData(0x10);
    SendData(0x54);
    SendData(0x44);

    SendCommand(BOOSTER_SOFT_START);
    SendData(0x05);
    SendData(0x00);
    SendData(0x3F);
    SendData(0x0A);
    SendData(0x25);
    SendData(0x12);
    SendData(0x1A); 

    SendCommand(VCOM_DATA_INTERVAL_SETTING);
    SendData(0x37);

    SendCommand(TCON_SETTING);
    SendData(0x02);
    SendData(0x02);

    SendCommand(TCON_RESOLUTION);
    SendData(width/256);
    SendData(width%256);
    SendData(height/256);
    SendData(height%256);

    SendCommand(LUT_FOR_VCOM);
    SendData(0x1C);

    SendCommand(POWER_SAVING);
    SendData(0x22);

    SendCommand(0xB4);
    SendData(0xD0);
    SendCommand(0xB5);
    SendData(0x03);

    SendCommand(0xE9);
    SendData(0x01); 

    SendCommand(PLL_CONTROL);
    SendData(0x08);  

    SendCommand(POWER_ON);
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
 *  @brief: Wait until the busy_pin goes LOW
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
    DigitalWrite(reset_pin, LOW);
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);     
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_REFRESH);
    SendData(0x00);
    WaitUntilIdle();
}

void Epd::SetLut(void) {
    // No LUT setting needed for this display
}

void Epd::ClearFrame() {
    unsigned int Width, Height;
    Width = (width % 4 == 0)? (width / 4 ): (width / 4 + 1);
    Height = height;

    SendCommand(DATA_START_TRANSMISSION);
    for (unsigned int j = 0; j < Height; j++) {
        for (unsigned int i = 0; i < Width; i++) {
            SendData(0x55);  // White color (01 01 01 01)
        }
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
    SendCommand(POWER_OFF);
    SendData(0x00);
    WaitUntilIdle();
    
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0xA5);
}

#endif

/* END OF FILE */