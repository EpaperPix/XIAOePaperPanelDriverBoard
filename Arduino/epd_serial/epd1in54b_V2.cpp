/**
 *  @filename   :   epd1in54b_V2.cpp
 *  @brief      :   Implements for Dual-color e-paper library
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

#ifdef EPD1IN54B_V2_C

#include <stdlib.h>
#include "epd_base.h"

// EPD1IN54B_V2 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
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
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200
#define EPD_STEPS      2
#define EPD_BLOCK_SIZE  5000

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
    stepCommands[1] = 0x26;
};

int Epd::Init(void) {
     Serial.print("IfInit before \r\n");
    if (IfInit() != 0) {
        return -1;
    }
    Serial.print("Reset before \r\n");
    Reset();
    Reset();
    Reset();
    Serial.print("Reset after \r\n");

    WaitUntilIdle();   
    Serial.print("SW_RESET before \r\n");
    SendCommand(SW_RESET);
    Serial.print("SW_RESET after \r\n");
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
    SendData(0xC7);    //0xC7-->(199+1)=200
    SendData(0x00);
    SendData(0x00);
    SendData(0x00); 

    SendCommand(BORDER_WAVEFORM_CONTROL);
    SendData(0x05);

    SendCommand(TEMPERATURE_SENSOR_CONTROL);
    SendData(0x80);

    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    SendData(0x00);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);    
    SendData(0xC7);
    SendData(0x00);
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

    while(1) {
        if(DigitalRead(busy_pin) == 0)
            break;
        DelayMs(100);
    }  
    /*
    while(DigitalRead(busy_pin) == 0) {      //0: busy, 1: idle
        DelayMs(1000);
         Serial.print("WaitUntilIdle\r\n");

    } 
    */     
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {

    DigitalWrite(reset_pin, HIGH);
    DelayMs(200); 
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(10);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    

      
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xF7);
    SendCommand(MASTER_ACTIVATION);
    WaitUntilIdle();
}

void Epd::SetLut(void) {
    // No LUT setting needed for V2
}

void Epd::ClearFrame() {
    SendCommand(WRITE_RAM);
    for(int i = 0; i < width * height / 8; i++) {               
        SendData(0xff);
    }
    SendCommand(WRITE_RAM_RED);
    for(int i = 0; i < width * height / 8; i++) {               
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
void Epd::Sleep() {
    SendCommand(DEEP_SLEEP_MODE);
    SendData(0x01);
    DelayMs(100);
}

#endif

/* END OF FILE */