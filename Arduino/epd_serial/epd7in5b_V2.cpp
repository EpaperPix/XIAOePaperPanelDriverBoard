/**
 *  @filename   :   epd7in5b_V2.cpp
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
#ifdef EPD7IN5B_C
#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480
#define EPD_STEPS      2
#define EPD_BLOCK_SIZE  48000

// Pixel format for this display
#define EPD_BITS_PER_PIXEL 1    // Monochrome: 1 byte per pixel  
#define EPD_PIXELS_PER_BYTE 8   // Each byte is one pixel

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
    steps=EPD_STEPS;
      bits_per_pixel = EPD_BITS_PER_PIXEL;
    pixels_per_byte = EPD_PIXELS_PER_BYTE;
    stepCommands[0]=0x10;
    stepCommands[1]=0x13;
     blockSize = EPD_BLOCK_SIZE;
     qr_color = 0xFF;
    
};
//unsigned char Epd::StepCommands[] ={0x10,0x13}; 

int Epd::Init(void) {
    if (IfInit() != 0) {
        return -1;
    }
    Reset();

    
    SendCommand(0x01); 
    SendData(0x07);
    SendData(0x07);
    SendData(0x3f);
    SendData(0x3f);

    SendCommand(0x04);
    DelayMs(100);
    WaitUntilIdle();
    
    SendCommand(0X00);			//PANNEL SETTING
    SendData(0x0F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

    SendCommand(0x61);        	//tres
    SendData(0x03);		//source 800
    SendData(0x20);
    SendData(0x01);		//gate 480
    SendData(0xE0);

    SendCommand(0X15);
    SendData(0x00);

    SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
    SendData(0x10);
    SendData(0x07);

    SendCommand(0X60);			//TCON SETTING
    SendData(0x22);

     return 0;

    SendCommand(0x01);			//POWER SETTING
    SendData(0x07);
    SendData(0x07);    //VGH=20V,VGL=-20V
    SendData(0x3f);		//VDH=15V
    SendData(0x3f);		//VDL=-15V

    SendCommand(0x04); //POWER ON
    DelayMs(100);
    WaitUntilIdle();

   // SendCommand(0X00);			//PANNEL SETTING
   // SendData(0xFF);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

    SendCommand(0x61);        	//tres
    SendData(0x03);		//source 800
    SendData(0x20);
    SendData(0x01);		//gate 480
    SendData(0xE0);

    SendCommand(0X15);
    SendData(0x00);

    SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
    SendData(0x11);
    SendData(0x07);

    SendCommand(0X60);			//TCON SETTING
    SendData(0x22);

    SendCommand(0x65);  // Resolution setting
    SendData(0x00);
    SendData(0x00);//800*480
    SendData(0x00);
    SendData(0x00);
	
    return 0;
}

/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
    DigitalWrite(dc_pin, LOW);
    SpiTransfer(command);
}


void Epd::SetToDataMode() {
    DigitalWrite(dc_pin, HIGH);
    
}

/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
   // DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

void Epd::SendDataFast(unsigned char data) {
  
    SpiTransfer(data);
}


/**
 *  @brief: Wait until the busy_pin goes HIGH
 
 */
void Epd::WaitUntilIdle(void) {
	unsigned char busy;
	do	{
		SendCommand(0x71);
		busy = DigitalRead(busy_pin);
		busy =!(busy & 0x01);        
	}while(busy);
	DelayMs(200);      
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
    DelayMs(2);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);
}


void Epd::Clear( unsigned char color) {
       SendCommand(0x10);
    SetToDataMode();
    for(unsigned long i=0; i<height*width / 8; i++) {
        SendData(0xFF);
    }
    SendCommand(0x13);
    SetToDataMode();
    for(unsigned long i=0; i<height*width / 8; i++)	{
        SendData(0x00);
    }
}

void Epd::ClearFrame() {
       SendCommand(0x10);
    SetToDataMode();
    for(unsigned long i=0; i<height*width / 8; i++) {
        SendData(0xFF);
    }
    // SendCommand(0x13);
    // SetToDataMode();
    // for(unsigned long i=0; i<height*width / 8; i++)	{
    //     SendData(0x00);
    // }
}

void Epd::TurnOnDisplay(void) {
       SendCommand(0x12);
        DelayMs(100);
        WaitUntilIdle();
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
    SendCommand(0X02);
    WaitUntilIdle();
    SendCommand(0X07);
    SendData(0xa5);
}
#endif
/* END OF FILE */


