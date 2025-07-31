/**
 *  @filename   :   epd2in13_V2.cpp
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

#ifdef EPD2IN13_V2_C

#include <stdlib.h>
#include "epd_base.h"

// EPD2IN13_V2 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define GATE_DRIVING_VOLTAGE_CONTROL                0x03
#define SOURCE_DRIVING_VOLTAGE_CONTROL              0x04
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_RAM_RED                               0x26
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_WINDOW_X_POSITION                       0x37
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define SET_ANALOG_BLOCK_CONTROL                    0x74
#define SET_DIGITAL_BLOCK_CONTROL                   0x7E

// Display resolution
#define EPD_WIDTH       128
#define EPD_HEIGHT      250
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  4000  // 128 * 250 / 8

extern const unsigned char lut_full_update[];
extern const unsigned char lut_partial_update[];

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

    SendCommand(SET_ANALOG_BLOCK_CONTROL);
    SendData(0x54);
    SendCommand(SET_DIGITAL_BLOCK_CONTROL);
    SendData(0x3B);

    SendCommand(DRIVER_OUTPUT_CONTROL);
    SendData(0xF9);
    SendData(0x00);
    SendData(0x00);

    SendCommand(DATA_ENTRY_MODE_SETTING);
    SendData(0x01);

    SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    SendData(0x00);
    SendData(0x0F);    //0x0F-->(15+1)*8=128

    SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    SendData(0xF9);   //0xF9-->(249+1)=250
    SendData(0x00);
    SendData(0x00);
    SendData(0x00);

    SendCommand(BORDER_WAVEFORM_CONTROL);
    SendData(0x03);

    SendCommand(WRITE_VCOM_REGISTER);
    SendData(0x55);

    SendCommand(GATE_DRIVING_VOLTAGE_CONTROL);
    SendData(lut_full_update[70]);

    SendCommand(SOURCE_DRIVING_VOLTAGE_CONTROL);
    SendData(lut_full_update[71]);
    SendData(lut_full_update[72]);
    SendData(lut_full_update[73]);

    SendCommand(SET_DUMMY_LINE_PERIOD);
    SendData(lut_full_update[74]);
    SendCommand(SET_GATE_TIME);
    SendData(lut_full_update[75]);

    SetLut();

    SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    SendData(0x00);
    SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    SendData(0xF9);
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
    while(1) {      //LOW: idle, HIGH: busy
        if(DigitalRead(busy_pin) == 0)
            break;
        DelayMs(100);
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
    DelayMs(10);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC7);
    SendCommand(MASTER_ACTIVATION);
    WaitUntilIdle();
}

void Epd::SetLut(void) {
    SendCommand(WRITE_LUT_REGISTER);
    for(int count = 0; count < 70; count++) {
        SendData(lut_full_update[count]);
    }
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

const unsigned char lut_full_update[]= {
    0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
    0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
    0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
    0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7

    0x03,0x03,0x00,0x00,0x02,                       // TP0 A~D RP0
    0x09,0x09,0x00,0x00,0x02,                       // TP1 A~D RP1
    0x03,0x03,0x00,0x00,0x02,                       // TP2 A~D RP2
    0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
    0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
    0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
    0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6

    0x15,0x41,0xA8,0x32,0x30,0x0A,
};

const unsigned char lut_partial_update[]= { //20 bytes
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
    0x80,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7

    0x0A,0x00,0x00,0x00,0x00,                       // TP0 A~D RP0
    0x00,0x00,0x00,0x00,0x00,                       // TP1 A~D RP1
    0x00,0x00,0x00,0x00,0x00,                       // TP2 A~D RP2
    0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
    0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
    0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
    0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6

    0x15,0x41,0xA8,0x32,0x30,0x0A,
};

#endif

/* END OF FILE */