/*****************************************************************************
* | File      	:   EPD_2in9_DES.h
* | Author      :   Waveshare team (Arduino port for the 2.9inch detector)
* | Function    :   2.9inch e-paper DES
* | Info        :   Ported from RaspberryPi_JetsonNano/c/lib/e-Paper/EPD_2in9_DES.c
*----------------
* |	This version:   V1.0
* | Date        :   2026-09-10
* | Info        :   UC8151-class, OTP LUT, single 0x13 image plane.
******************************************************************************/
#ifndef __EPD_2IN9_DES_H_
#define __EPD_2IN9_DES_H_

#include "Debug.h"
#include "DEV_Config.h"

// Display resolution
#define EPD_2IN9_DES_WIDTH       128
#define EPD_2IN9_DES_HEIGHT      296

void EPD_2IN9_DES_Init(void);
void EPD_2IN9_DES_Clear(void);
void EPD_2IN9_DES_Display(UBYTE *Image);
void EPD_2IN9_DES_Sleep(void);

#endif
