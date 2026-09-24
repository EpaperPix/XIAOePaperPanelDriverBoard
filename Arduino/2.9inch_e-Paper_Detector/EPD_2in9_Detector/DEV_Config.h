/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :   Waveshare team (unified multi-board copy for the 2.9inch detector)
* | Function    :   Hardware underlying interface
* | Info        :   One pin map per board, selected by the Arduino core's macros.
*                   Bit-banged SPI on every board so the timing is identical.
*----------------
* |	This version:   V1.1
* | Date        :   2026-09-13
* | Info        :   V1.0 adds DEV_WaitBusy (bounded busy wait), DEV_HW_Reset, DEV_SendCommand.
*                   V1.1 adds the SeeedStudio XIAO ePaper Driver / Breakout pin map
*                   (XIAO ESP32-S3/C3/C6), same wiring as epdif.h in the sibling sketches.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <Arduino.h>      /* on ESP32 this pulls in sdkconfig.h, needed for the target macros */
#include <stdint.h>
#include <stdio.h>

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * Board / pin map selection
 *
 * Primary target: SeeedStudio XIAO (ESP32-S3 / S3 Plus / C3 / C6 / C5) seated on the
 * SeeedStudio "ePaper Driver Board for XIAO" or "ePaper Breakout Board for XIAO".
 * Same wiring as epdif.h in ../epd_epaperpix_wifi and ../epd_serial:
 *     RST=D0  CS=D1  DC=D3  SCK=SCK(D8)  DIN=MOSI(D10)
 *     BUSY=D2 on the ePaper Driver board, BUSY=D5 on the ePaper Breakout board.
 * The two carrier boards differ only in BUSY. Select yours below (Driver is the default,
 * matching epdif.h). Neither board has a PWR pin; the panel is powered permanently.
 *
 * The Waveshare UNO R4 / ESP32 / ESP32-S3 dev-board maps are kept as a fallback for
 * anyone still using those boards.
**/

/* --- SeeedStudio carrier board: uncomment for the ePaper Breakout XIAO (BUSY=D5) --- */
// #define EPD_XIAO_BREAKOUT

#if defined(ARDUINO_XIAO_ESP32S3) || defined(ARDUINO_XIAO_ESP32S3_PLUS) || \
    defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_XIAO_ESP32C6) || defined(ARDUINO_XIAO_ESP32C5)
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
        #define EPD_MCU_NAME    "XIAO ESP32S3"
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        #define EPD_MCU_NAME    "XIAO ESP32C3"
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define EPD_MCU_NAME    "XIAO ESP32C6"
    #elif defined(CONFIG_IDF_TARGET_ESP32C5)
        #define EPD_MCU_NAME    "XIAO ESP32C5"
    #else
        #define EPD_MCU_NAME    "XIAO ESP32"
    #endif
    #ifdef EPD_XIAO_BREAKOUT
        #define EPD_BOARD_NAME  EPD_MCU_NAME " ePaper Breakout"
        #define EPD_BUSY_PIN    D5
    #else
        #define EPD_BOARD_NAME  EPD_MCU_NAME " ePaper Driver"
        #define EPD_BUSY_PIN    D2
    #endif
    #define EPD_SCK_PIN     SCK     /* D8  */
    #define EPD_MOSI_PIN    MOSI    /* D10 */
    #define EPD_CS_PIN      D1
    #define EPD_DC_PIN      D3
    #define EPD_RST_PIN     D0
    /* no EPD_PWR_PIN on the SeeedStudio boards */
#elif defined(ARDUINO_ARCH_RENESAS_UNO) || defined(ARDUINO_MINIMA) || defined(ARDUINO_UNOWIFIR4)
    #define EPD_BOARD_NAME  "Arduino UNO R4"
    #define EPD_SCK_PIN     13
    #define EPD_MOSI_PIN    11
    #define EPD_CS_PIN      10
    #define EPD_DC_PIN      9
    #define EPD_RST_PIN     8
    #define EPD_BUSY_PIN    7
    #define EPD_PWR_PIN     6
#elif defined(ARDUINO_ARCH_ESP32)
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
        #define EPD_BOARD_NAME  "ESP32-S3 (Waveshare)"
        #define EPD_SCK_PIN     12
        #define EPD_MOSI_PIN    11
        #define EPD_CS_PIN      10
        #define EPD_DC_PIN      13
        #define EPD_RST_PIN     14
        #define EPD_BUSY_PIN    4
        #define EPD_PWR_PIN     5       /* 9-pin FPC boards only; comment out for 8-pin */
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        #define EPD_BOARD_NAME  "ESP32 (Waveshare)"
        #define EPD_SCK_PIN     13
        #define EPD_MOSI_PIN    14
        #define EPD_CS_PIN      15
        #define EPD_DC_PIN      27
        #define EPD_RST_PIN     26
        #define EPD_BUSY_PIN    25
        #define EPD_PWR_PIN     33
    #else
        #error "No pin map for this ESP32 variant. For a SeeedStudio XIAO pick the XIAO_ESP32S3 / XIAO_ESP32C3 / XIAO_ESP32C6 board in Tools > Board (not a generic Dev Module), otherwise add a map in DEV_Config.h."
    #endif
#else
    #error "Supported boards: SeeedStudio XIAO ESP32-S3/C3/C6 (ePaper Driver or Breakout board), Arduino UNO R4 Minima/WiFi, ESP32, ESP32-S3. Add a pin map in DEV_Config.h for other boards."
#endif

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0

/**
 * GPIO read and write
**/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0? LOW:HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms) delay(__xms)

UBYTE DEV_Module_Init(void);
void  DEV_Module_Exit(void);
void  GPIO_Mode(UWORD GPIO_Pin, UWORD Mode);
void  DEV_SPI_WriteByte(UBYTE data);
void  DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len);

/**
 * Detector additions
**/
#define DEV_BUSY_FAILFAST_MS 300      /* cap on every wait after the first timeout in an attempt */

extern uint32_t DEV_BusyTimeoutMs;     /* per-wait limit, set by the sketch around Init / Display */
extern uint8_t  DEV_BusyTimeoutCount;  /* expired waits since DEV_ResetBusyStats() */

void    DEV_ResetBusyStats(void);
/* Wait while digitalRead(BUSY) == busy_level. poll() (may be NULL) is called before every read.
   Returns 1 when released, 0 on timeout. */
uint8_t DEV_WaitBusy(uint8_t busy_level, void (*poll)(void));
/* Generic RST pulse: high, low for low_ms, high, then wait high_ms each side. */
void    DEV_HW_Reset(uint16_t high_ms, uint16_t low_ms);
/* Send one command byte with DC low and CS framed (same as every driver's SendCommand). */
void    DEV_SendCommand(UBYTE Reg);

#endif
