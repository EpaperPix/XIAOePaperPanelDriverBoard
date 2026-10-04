/**
 *  @filename   :   epd7in5.cpp
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

#include "display_config.h"
#ifdef EPD7IN5_C

#include <stdlib.h>
#include "epd_base.h"

// EPD7IN5 commands
#define PANEL_SETTING                               0x00
#define POWER_SETTING                               0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define POWER_ON_MEASURE                            0x05
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP                                  0x07
#define DATA_START_TRANSMISSION_1                   0x10
#define DATA_STOP                                   0x11
#define DISPLAY_REFRESH                             0x12
#define DATA_START_TRANSMISSION_2                   0x13
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_CALIBRATION                     0x40
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define VCM_DC_SETTING                              0x82

// Display resolution
#define EPD_WIDTH       640
#define EPD_HEIGHT      384
#define EPD_STEPS      1
// The cloud sends a 1-bit A plane (640 x 384 / 8 = 30720 bytes); the UC8159 RAM holds 4 bits per pixel
// (122880 bytes, 0x0 black, 0x3 white), so SendStreamByte expands every stream bit into one RAM pixel.
#define EPD_BLOCK_SIZE  30720
#define EPD_BITS_PER_PIXEL 1
EPD_CHECK_PLANE(EPD_WIDTH, EPD_HEIGHT, EPD_BITS_PER_PIXEL, EPD_BLOCK_SIZE);
static_assert((unsigned long)EPD_WIDTH * EPD_HEIGHT / 2 == 4UL * EPD_BLOCK_SIZE, "4-bpp RAM plane is four times the stream plane");

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
    bits_per_pixel = EPD_BITS_PER_PIXEL;
    pixels_per_byte = 8;
    expand_1bpp_to_4bpp = true;
    // Builder code A: 1 bit per pixel, black = 1. The expansion maps 1 to black (0x0) and 0 to white
    // (0x3), the nibble codes Waveshare's epd7in5 DisplayFrame and this ClearFrame (0x33) use.
    // No inversion on top of that.
    screenCode = "A";
    stepInvert[0] = false;
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();

    SendCommand(POWER_SETTING); 
    SendData(0x37);
    SendData(0x00);

    SendCommand(PANEL_SETTING);
    SendData(0xCF);
    SendData(0x08);
    
    SendCommand(BOOSTER_SOFT_START);
    SendData(0xc7);     
    SendData(0xcc);
    SendData(0x28);

    SendCommand(POWER_ON);
    WaitUntilIdle();

    SendCommand(PLL_CONTROL);
    SendData(0x3c);        

    SendCommand(TEMPERATURE_CALIBRATION);
    SendData(0x00);

    SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
    SendData(0x77);

    SendCommand(TCON_SETTING);
    SendData(0x22);

    SendCommand(TCON_RESOLUTION);
    SendData(0x02);     //source 640
    SendData(0x80);
    SendData(0x01);     //gate 384
    SendData(0x80);

    SendCommand(VCM_DC_SETTING);
    SendData(0x1E);      //decide by LUT file

    SendCommand(0xe5);           //FLASH MODE            
    SendData(0x03);  

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
    DigitalWrite(dc_pin, HIGH);         // init parameters are data, not commands
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
    WaitWhileBusy(LOW, 100);            //0: busy, 1: idle
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
    DelayMs(10);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);   
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_REFRESH); 
    WaitUntilIdle();
}

void Epd::Clear(unsigned char color) {
    TurnOnDisplay();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          The only one parameter is a check code, the command would be
 *          executed if check code = 0xA5. 
 *          You can use EPD_Reset() to awaken
 */
void Epd::Sleep(void) {
    SendCommand(POWER_OFF);
    WaitUntilIdle();
    SendCommand(DEEP_SLEEP);
    SendData(0xa5);
}

/**
 *  @brief: fill the frame buffer with white without refreshing (Waveshare Clear())
 */
void Epd::ClearFrame(void) {
    SendCommand(DATA_START_TRANSMISSION_1);
    SetToDataMode();
    for(unsigned long i=0; i<(unsigned long)width/2*height; i++) {
        SendData(0x33);
    }
}

#endif

/* END OF FILE */


