/**
 *  @filename   :   epdif.h
 *  @brief      :   Header file of epdif.cpp providing EPD interface functions
 *                  Users have to implement all the functions in epdif.cpp
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

#ifndef EPDIF_H
#define EPDIF_H

#include <Arduino.h>
#include <SPI.h>

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#endif

// Pin definitions for different platforms
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
  // ESP32-S2/S3 specific definitions
 // #define VSPI FSPI

 // Pin definition SeeedStudio ePaper Breakout XIAO  
// #define RST_PIN         D0
// #define DC_PIN          D3
// #define CS_PIN          D1
// #define BUSY_PIN        D5

// Pin definition SeeedStudio ePaper Driver XIAO
  #define RST_PIN         D0
  #define DC_PIN          D3
  #define CS_PIN          D1
  #define BUSY_PIN        D2



  
  
  // SPI pin mappings
  #define PIN_SPI_SCK     SCK
  #define PIN_SPI_DIN     MOSI
  #define PIN_SPI_CS      CS_PIN
  #define MISO_PIN        D9
  #define PIN_SPI_BUSY    BUSY_PIN
  #define PIN_SPI_RST     RST_PIN
  #define PIN_SPI_DC      DC_PIN
  
  #define VSPI_MISO       MISO
  #define VSPI_MOSI       MOSI
  #define VSPI_SCLK       SCK
  #define VSPI_SS         CS_PIN
#else
  // Default pin definitions (Arduino/ESP32)
  #define RST_PIN         8
  #define DC_PIN          9
  #define CS_PIN          10
  #define BUSY_PIN        7
  #define PWR_PIN         6
#endif





class EpdIf {
public:
    EpdIf(void);
    ~EpdIf(void);

      
    static int  IfInit(void);
    static void DigitalWrite(int pin, int value); 
    static int  DigitalRead(int pin);
    static void DelayMs(unsigned int delaytime);
    static void SpiTransfer(unsigned char data);
};

#endif
