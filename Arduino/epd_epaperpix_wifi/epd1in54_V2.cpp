/**
 *  @filename   :   epd1in54_V2.cpp
 *  @brief      :   Implements for Monochrome e-paper library V2
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

#ifdef EPD1IN54_V2_C

#include <stdlib.h>
#include "epd_base.h"

// EPD1IN54_V2 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define GATE_DRIVING_VOLTAGE_CONTROL                0x03
#define SOURCE_DRIVING_VOLTAGE_CONTROL              0x04
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x18
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_RAM_RED                               0x26
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_WINDOW_X_POSITION                       0x37
#define SET_WINDOW_Y_POSITION                       0x38
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_PARTIAL_WINDOW                          0x3F
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  5000

extern unsigned char WF_Full_1IN54[];
extern unsigned char WF_PARTIAL_1IN54_0[];

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
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();

    WaitUntilIdle();
    SendCommand(SW_RESET);
    WaitUntilIdle();

    SendCommand(DRIVER_OUTPUT_CONTROL);
    SendData(0xC7);
    SendData(0x00);
    SendData(0x01);

    SendCommand(DATA_ENTRY_MODE_SETTING);
    SendData(0x01);

    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    SendData(0x00);
    SendData(0x18);    //0x18-->(24+1)*8=200

    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    SendData(0xC7);   //0xC7-->(199+1)=200
    SendData(0x00);
    SendData(0x00);
    SendData(0x00);

    SendCommand(BORDER_WAVEFORM_CONTROL);
    SendData(0x01);

    SendCommand(TEMPERATURE_SENSOR_CONTROL);
    SendData(0x80);

    SendCommand(DISPLAY_UPDATE_CONTROL_2); 
    SendData(0xB1);
    SendCommand(MASTER_ACTIVATION);

    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    SendData(0x00);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    SendData(0xC7);
    SendData(0x00);
    WaitUntilIdle();

    SetLut();
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
    while(DigitalRead(busy_pin) == 1) {      //LOW: idle, HIGH: busy
        DelayMs(100);
    }
    DelayMs(200);
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
    DelayMs(5);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(20);
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC7);
    SendCommand(MASTER_ACTIVATION);
    WaitUntilIdle();
}

void Epd::SetLut(void) {
    SendCommand(WRITE_LUT_REGISTER);
    for(unsigned char i = 0; i < 153; i++)
        SendData(WF_Full_1IN54[i]);
    WaitUntilIdle();
    
    SendCommand(SET_PARTIAL_WINDOW);
    SendData(WF_Full_1IN54[153]);
    
    SendCommand(GATE_DRIVING_VOLTAGE_CONTROL);
    SendData(WF_Full_1IN54[154]);
    
    SendCommand(SOURCE_DRIVING_VOLTAGE_CONTROL);
    SendData(WF_Full_1IN54[155]);
    SendData(WF_Full_1IN54[156]);
    SendData(WF_Full_1IN54[157]);
    
    SendCommand(WRITE_VCOM_REGISTER);
    SendData(WF_Full_1IN54[158]);
}

void Epd::ClearFrame() {
    int w = (width % 8 == 0)? (width / 8 ): (width / 8 + 1);
    int h = height;
 
    SendCommand(WRITE_RAM);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            SendData(0xff);
        }
    }
    SendCommand(WRITE_RAM_RED);
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            SendData(0xff);
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
void Epd::Sleep() {
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0x01);
    DelayMs(200);
    DigitalWrite(reset_pin, LOW);
}

// waveform full refresh
unsigned char WF_Full_1IN54[159] =
{                                            
0x80,    0x48,    0x40,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,
0x40,    0x48,    0x80,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,
0x80,    0x48,    0x40,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,
0x40,    0x48,    0x80,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,
0xA,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x8,    0x1,    0x0,    0x8,    0x1,    0x0,    0x2,                    
0xA,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x0,    0x0,    0x0,    0x0,    0x0,    0x0,    0x0,                    
0x22,    0x22,    0x22,    0x22,    0x22,    0x22,    0x0,    0x0,    0x0,            
0x22,    0x17,    0x41,    0x0,    0x32,    0x20
};

// waveform partial refresh(fast)
unsigned char WF_PARTIAL_1IN54_0[159] =
{
0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x80,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x40,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0xF,0x0,0x0,0x0,0x0,0x0,0x0,
0x1,0x1,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x22,0x22,0x22,0x22,0x22,0x22,0x0,0x0,0x0,
0x02,0x17,0x41,0xB0,0x32,0x28,
};

#endif

/* END OF FILE */