/**
 *  @filename   :   epd2in9.cpp
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

#include "display_config.h"
#ifdef EPD2IN9_C

#include <stdlib.h>
#include "epd_base.h"

// EPD2IN9 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
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
#define EPD_WIDTH       128
#define EPD_HEIGHT      296
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  4736  // 128 * 296 / 8
#define EPD_BITS_PER_PIXEL 1
EPD_CHECK_PLANE(EPD_WIDTH, EPD_HEIGHT, EPD_BITS_PER_PIXEL, EPD_BLOCK_SIZE);

extern const unsigned char lut_full_update[];
extern const unsigned char lut_partial_update[];

// SSD1608 (2.9inch V1) RAM window and address counter. The sketch streams a frame with
// SendCommand(0x24) followed by raw data, so the window and cursor are restored before
// every WRITE_RAM (see SendCommand) exactly as the Waveshare EPD_2IN9_Display() does.
static void writeCommandRaw(unsigned char command) {
    EpdIf::DigitalWrite(DC_PIN, LOW);
    EpdIf::SpiTransfer(command);
}
static void writeDataRaw(unsigned char data) {
    EpdIf::DigitalWrite(DC_PIN, HIGH);
    EpdIf::SpiTransfer(data);
}
static void setMemoryArea(int x_start, int y_start, int x_end, int y_end) {
    writeCommandRaw(SET_RAM_X_ADDRESS_START_END_POSITION);
    writeDataRaw((x_start >> 3) & 0xFF);
    writeDataRaw((x_end >> 3) & 0xFF);
    writeCommandRaw(SET_RAM_Y_ADDRESS_START_END_POSITION);
    writeDataRaw(y_start & 0xFF);
    writeDataRaw((y_start >> 8) & 0xFF);
    writeDataRaw(y_end & 0xFF);
    writeDataRaw((y_end >> 8) & 0xFF);
}
static void setMemoryPointer(int x, int y) {
    writeCommandRaw(SET_RAM_X_ADDRESS_COUNTER);
    writeDataRaw((x >> 3) & 0xFF);
    writeCommandRaw(SET_RAM_Y_ADDRESS_COUNTER);
    writeDataRaw(y & 0xFF);
    writeDataRaw((y >> 8) & 0xFF);
}
// Waveform for the next refresh: lut_full_update (every pixel driven, the screen flashes) or
// lut_partial_update (fast refresh: only pixels that differ from the other RAM bank change).
static void writeLut(const unsigned char* lut) {
    writeCommandRaw(WRITE_LUT_REGISTER);
    for (int i = 0; i < 30; i++) {
        writeDataRaw(lut[i]);
    }
}

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
    bits_per_pixel = EPD_BITS_PER_PIXEL;
    pixels_per_byte = 8;
    // Builder code AR: 1 bit per pixel, white = 1, rotated for this portrait panel. SSD1608 RAM: 1 = white
    // (ClearFrame writes 0xFF). Same polarity: sent unchanged.
    screenCode = "AR";
    stepInvert[0] = false;
};

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();
    SendCommand(DRIVER_OUTPUT_CONTROL);
    SendData((EPD_HEIGHT - 1) & 0xFF);
    SendData(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    SendData(0x00);                     // GD = 0; SM = 0; TB = 0;
    SendCommand(BOOSTER_SOFT_START_CONTROL);
    SendData(0xD7);
    SendData(0xD6);
    SendData(0x9D);
    SendCommand(WRITE_VCOM_REGISTER);
    SendData(0xA8);                     // VCOM 7C
    SendCommand(SET_DUMMY_LINE_PERIOD);
    SendData(0x1A);                     // 4 dummy lines per gate
    SendCommand(SET_GATE_TIME);
    SendData(0x08);                     // 2us per line
    SendCommand(BORDER_WAVEFORM_CONTROL);
    SendData(0x03);
    SendCommand(DATA_ENTRY_MODE_SETTING);
    SendData(0x03);                     // X increment; Y increment
    SetLut();
    setMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    setMemoryPointer(0, 0);
    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    if (command == WRITE_RAM) {
        // Every frame (download stream, retry, QRset, ClearFrame) starts at RAM 0,0
        setMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
        setMemoryPointer(0, 0);
    }
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
    WaitWhileBusy(HIGH, 100);           //LOW: idle, HIGH: busy
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
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);
}

void Epd::TurnOnDisplay(void) {
    SendCommand(DISPLAY_UPDATE_CONTROL_2);
    SendData(0xC4);
    SendCommand(MASTER_ACTIVATION);
    SendCommand(TERMINATE_FRAME_READ_WRITE);
    WaitUntilIdle();
}

/**
 *  @brief: set the look-up table register
 */
void Epd::SetLut(void) {
    writeLut(lut_full_update);
}

// Fast refresh (EPD_FAST_REFRESH): same refresh command, partial waveform. The RAM must already hold the
// picture on screen, which Epd::ShowFrame guarantees by writing every frame twice.
void Epd::SetPartialLut(void) {
    writeLut(lut_partial_update);
}

void Epd::ClearFrame() {
    WaitUntilIdle();
    SendCommand(WRITE_RAM);             // window and cursor reset by SendCommand
    for (int i = 0; i < width / 8 * height; i++) {
        SendData(0xFF);                 // 1 = white on this panel
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
}

const unsigned char lut_full_update[] = {
    0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif

/* END OF FILE */