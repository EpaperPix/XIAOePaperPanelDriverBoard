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
//#include "qrset.cpp"

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480
#define EPD_STEPS      2
#define EPD_BLOCK_SIZE 48000 // 96000

// Pixel format for this display
#define EPD_BITS_PER_PIXEL 1    // Monochrome: 1 byte per pixel  
#define EPD_PIXELS_PER_BYTE 8   // Each byte is one pixel

unsigned char Voltage_Frame_7IN5_V2[]={
	0x6, 0x3F, 0x3F, 0x11, 0x24, 0x7, 0x17,
};

unsigned char LUT_VCOM_7IN5_V2[]={	
	0x0,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0xF,	0x1,	0xF,	0x1,	0x2,	
	0x0,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
};						

unsigned char LUT_WW_7IN5_V2[]={	
	0x10,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x84,	0xF,	0x1,	0xF,	0x1,	0x2,	
	0x20,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
};

unsigned char LUT_BW_7IN5_V2[]={	
	0x10,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x84,	0xF,	0x1,	0xF,	0x1,	0x2,	
	0x20,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
};

unsigned char LUT_WB_7IN5_V2[]={	
	0x80,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x84,	0xF,	0x1,	0xF,	0x1,	0x2,	
	0x40,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
};

unsigned char LUT_BB_7IN5_V2[]={	
	0x80,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x84,	0xF,	0x1,	0xF,	0x1,	0x2,	
	0x40,	0xF,	0xF,	0x0,	0x0,	0x1,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
};

Epd::~Epd() {
};

Epd::Epd() {
    reset_pin = RST_PIN;
    dc_pin = DC_PIN;
    cs_pin = CS_PIN;
    busy_pin = BUSY_PIN;
    width = EPD_WIDTH;
    height = EPD_HEIGHT;
    bits_per_pixel = EPD_BITS_PER_PIXEL;
    pixels_per_byte = EPD_PIXELS_PER_BYTE;
    steps=EPD_STEPS;
    blockSize = EPD_BLOCK_SIZE;
    //stepCommands[0]=0x13;
    stepCommands[0]=0x10;
    stepCommands[1]=0x13;
    ShowDebug = false;
    qr_color = 0xFF;
};

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

	SendCommand(0x01);  // power setting
	SendData(0x17);  // 1-0=11: internal power
	SendData(*(Voltage_Frame_7IN5_V2+6));  // VGH&VGL
	SendData(*(Voltage_Frame_7IN5_V2+1));  // VSH
	SendData(*(Voltage_Frame_7IN5_V2+2));  //  VSL
	SendData(*(Voltage_Frame_7IN5_V2+3));  //  VSHR
	
	SendCommand(0x82);  // VCOM DC Setting
	SendData(*(Voltage_Frame_7IN5_V2+4));  // VCOM

	SendCommand(0x06);  // Booster Setting
	SendData(0x27);
	SendData(0x27);
	SendData(0x2F);
	SendData(0x17);
	
	SendCommand(0x30);   // OSC Setting
	SendData(*(Voltage_Frame_7IN5_V2+0));  // 2-0=100: N=4  ; 5-3=111: M=7  ;  3C=50Hz     3A=100HZ

    SendCommand(0x04); //POWER ON
    DelayMs(100);
    WaitUntilIdle();

    SendCommand(0X00);			//PANNEL SETTING
    SendData(0x3F);   //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

    SendCommand(0x61);        	//tres
    SendData(0x03);		//source 800
    SendData(0x20);
    SendData(0x01);		//gate 480
    SendData(0xE0);

    SendCommand(0X15);
    SendData(0x00);

    SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
    SendData(0x10);
    SendData(0x00);

    SendCommand(0X60);			//TCON SETTING
    SendData(0x22);

    SendCommand(0x65);  // Resolution setting
    SendData(0x00);
    SendData(0x00);//800*480
    SendData(0x00);
    SendData(0x00);

    SetLut_by_host(LUT_VCOM_7IN5_V2, LUT_WW_7IN5_V2, LUT_BW_7IN5_V2, LUT_WB_7IN5_V2, LUT_BB_7IN5_V2);

    return 0;
}

void Epd::SetLut_by_host(unsigned char* lut_vcom,  unsigned char* lut_ww, unsigned char* lut_bw, unsigned char* lut_wb, unsigned char* lut_bb)
{
	unsigned char count;

	SendCommand(0x20); //VCOM	
	for(count=0; count<42; count++)
		SendData(lut_vcom[count]);

	SendCommand(0x21); //LUTBW
	for(count=0; count<42; count++)
		SendData(lut_ww[count]);

	SendCommand(0x22); //LUTBW
	for(count=0; count<42; count++)
		SendData(lut_bw[count]);

	SendCommand(0x23); //LUTWB
	for(count=0; count<42; count++)
		SendData(lut_wb[count]);

	SendCommand(0x24); //LUTBB
	for(count=0; count<42; count++)
		SendData(lut_bb[count]);
}
/*
int Epd::Init4G(void) {
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
    
    SendCommand(0x00);      //PANNEL SETTING
    SendData(0xBF);   //KW-3f   KWR-2F  BWROTP 0f BWOTP 1f

    SendCommand(0x30);
    SendData(0x06);
    
    SendCommand(0x61);          //tres
    SendData(0x03);   //source 800
    SendData(0x20);
    SendData(0x01);   //gate 480
    SendData(0xE0);

    SendCommand(0X15);
    SendData(0x00);

    SendCommand(0X60);      //TCON SETTING
    SendData(0x22);

    SendCommand(0X82);    
    SendData(0x12);
    
    SendCommand(0X50);      //VCOM AND DATA INTERVAL SETTING
    SendData(0x10);
    SendData(0x07);

   
    return 0;
}

*/
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
   DigitalWrite(dc_pin, HIGH);
    SpiTransfer(data);
}

void Epd::SendDataFast(unsigned char data) {
  
    SpiTransfer(data);
}

/**
 *  @brief: Wait until the busy_pin goes HIGH
 */
// void Epd::WaitUntilIdle(void) {
//     unsigned char busy;
//     do{
//        SendCommand(0x71);
//         busy = DigitalRead(busy_pin);
//          Serial.print(busy);
//     }while(busy == 0);
//     DelayMs(200);
// }
void Epd::WaitUntilIdle(void) {
    unsigned char busy;
    Serial.print("e-Paper Busy\r\n ");
    do{
        SendCommand(0x71);
        busy = DigitalRead(busy_pin);
    }while(busy == 0);
    Serial.print("e-Paper Busy Release\r\n ");
    DelayMs(20);
}
/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
   Serial.print("e-Paper Reset\r\n ");
   DelayMs(40);
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(4);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(200);    
    Serial.print("e-Paper Reset Release\r\n ");
}
void Epd::TurnOnDisplay(void) {
   Serial.print("e-Paper TurnOnDisplay\r\n ");
   SendCommand(0x12);
    DelayMs(100);
    WaitUntilIdle();
   
  Serial.print("e-Paper TurnOnDisplay  Release\r\n ");
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
    SendData(0xA5);
}

void Epd::Clear( unsigned char color) {
    
    SendCommand(0x10);
    SetToDataMode();
    for(unsigned long i=0; i<height*width / 8; i++) {
        SendData(color);
    }
    SendCommand(0x13);
    SetToDataMode();
    for(unsigned long i=0; i<height*width / 8; i++)	{
        SendData(color);
    }
    SendCommand(0x12);
    DelayMs(100);
    WaitUntilIdle();
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


/* END OF FILE */

#endif


