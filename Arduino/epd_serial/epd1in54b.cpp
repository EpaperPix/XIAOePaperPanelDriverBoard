/**
 *  @filename   :   epd1in54b.cpp
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

 /* Need this code
 if(i == 0)
        {
         // Bit manipulation 1: Process first 4 bits (bits 0-3)
            uint8_t temp1 = 0x00;
            for (int bit = 0; bit < 4; bit++) {
                if ((byte1 & (0x80 >> bit)) != 0) {
                    temp1 |= 0xC0 >> (bit * 2);
                }
            }
            epd.SendDataFast(temp1);
            
            // Bit manipulation 2: Process last 4 bits (bits 4-7)
            uint8_t temp2 = 0x00;
            for (int bit = 4; bit < 8; bit++) {
                if ((byte1 & (0x80 >> bit)) != 0) {
                    temp2 |= 0xC0 >> ((bit - 4) * 2);
                }
            }
            epd.SendDataFast(temp2);
        }else
 
 */

#ifdef EPD1IN54B_C

#include <stdlib.h>
#include "epd_base.h"

// EPD1IN54B commands
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
#define LUT_FOR_VCOM                                0x20 
#define LUT_WHITE_TO_WHITE                          0x21
#define LUT_BLACK_TO_WHITE                          0x22
#define LUT_WHITE_TO_BLACK                          0x23
#define LUT_BLACK_TO_BLACK                          0x24
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_SENSOR_COMMAND                  0x40
#define TEMPERATURE_SENSOR_CALIBRATION              0x41
#define TEMPERATURE_SENSOR_WRITE                    0x42
#define TEMPERATURE_SENSOR_READ                     0x43
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define LOW_POWER_DETECTION                         0x51
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define SOURCE_AND_GATE_START_SETTING               0x62
#define GET_STATUS                                  0x71
#define AUTO_MEASURE_VCOM                           0x80
#define VCOM_VALUE                                  0x81
#define VCM_DC_SETTING_REGISTER                     0x82

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200
#define EPD_STEPS      2
#define EPD_BLOCK_SIZE  5000

extern const unsigned char lut_vcom0[];
extern const unsigned char lut_w[];
extern const unsigned char lut_b[];
extern const unsigned char lut_g1[];
extern const unsigned char lut_g2[];
extern const unsigned char lut_vcom1[];
extern const unsigned char lut_red0[];
extern const unsigned char lut_red1[];

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
    stepCommands[1] = 0x13;
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();
    SendCommand(POWER_SETTING);
    SendData(0x07);
    SendData(0x00);
    SendData(0x08);
    SendData(0x00);
    SendCommand(BOOSTER_SOFT_START);
    SendData(0x07);
    SendData(0x07);
    SendData(0x07);
    SendCommand(POWER_ON);

    WaitUntilIdle();

    SendCommand(PANEL_SETTING);
    SendData(0xcf);
    SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
    SendData(0x17);
    SendCommand(PLL_CONTROL);
    SendData(0x39);
    SendCommand(TCON_RESOLUTION);
    SendData(0xC8);
    SendData(0x00);
    SendData(0xC8);
    SendCommand(VCM_DC_SETTING_REGISTER);
    SendData(0x0E);

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
    while(DigitalRead(busy_pin) == 0) {      //0: busy, 1: idle
        DelayMs(100);
    }      
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(200);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_REFRESH); 
    WaitUntilIdle();
}



void Epd::SetLut(void) {
     unsigned int count;   
     //bw
   SendCommand(0x20);         //g vcom
    for(count = 0; count < 15; count++) {
        SendData(lut_vcom0[count]);
    } 
    SendCommand(0x21);        //g ww --
    for(count = 0; count < 15; count++) {
        SendData(lut_w[count]);
    } 
    SendCommand(0x22);         //g bw r
    for(count = 0; count < 15; count++) {
        SendData(lut_b[count]);
    } 
    SendCommand(0x23);         //g wb w
    for(count = 0; count < 15; count++) {
        SendData(lut_g1[count]);
    } 
    SendCommand(0x24);         //g bb b
    for(count = 0; count < 15; count++) {
        SendData(lut_g2[count]);
    }
   // Red
    SendCommand(0x25);
    for(count = 0; count < 15; count++) {
        SendData(lut_vcom1[count]);
    } 
    SendCommand(0x26);
    for(count = 0; count < 15; count++) {
        SendData(lut_red0[count]);
    } 
    SendCommand(0x27);
    for(count = 0; count < 15; count++) {
        SendData(lut_red1[count]);
    } 
}

void Epd::ClearFrame() {
    SendCommand(DATA_START_TRANSMISSION_1);
    DelayMs(2);
    SetToDataMode();
    for (int i = 0; i < EPD_BLOCK_SIZE; i++) {
        SendDataFast(0xFF);
         SendDataFast(0xFF);
    }
    DelayMs(2);
    SendCommand(DATA_START_TRANSMISSION_2);
    SetToDataMode();
    DelayMs(2);
    for (int i = 0; i < EPD_BLOCK_SIZE; i++) {
        SendDataFast(0xFF);
    }
    DelayMs(2);
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
    SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
    SendData(0x17);
    SendCommand(VCM_DC_SETTING_REGISTER);         //to solve Vcom drop
    SendData(0x00);
    SendCommand(POWER_SETTING);         //power setting
    SendData(0x02);        //gate switch to external
    SendData(0x00);
    SendData(0x00);
    SendData(0x00);
    WaitUntilIdle();
    SendCommand(POWER_OFF);         //power off
}

const unsigned char lut_vcom0[] =
{
    0x0E, 0x14, 0x01, 0x0A, 0x06, 0x04, 0x0A, 0x0A,
    0x0F, 0x03, 0x03, 0x0C, 0x06, 0x0A, 0x00
};

const unsigned char lut_w[] =
{
    0x0E, 0x14, 0x01, 0x0A, 0x46, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x86, 0x0A, 0x04
};

const unsigned char lut_b[] = 
{
    0x0E, 0x14, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x4A, 0x04
};

const unsigned char lut_g1[] = 
{
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04
};

const unsigned char lut_g2[] = 
{
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04
};

const unsigned char lut_vcom1[] = 
{
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_red0[] = 
{
    0x83, 0x5D, 0x01, 0x81, 0x48, 0x23, 0x77, 0x77,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_red1[] = 
{
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif

/* END OF FILE */