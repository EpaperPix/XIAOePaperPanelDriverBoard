
//#include "HWCDC.h"
/*****************************************************************************
* | File      	:   EPD_7in3f.c
* | Author      :   Waveshare team
* | Function    :   7.3inch e-Paper (F)
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2022-10-21
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#ifdef EPD7IN3F_C

#include <stdlib.h>
#include "epd_base.h"

// Display resolution
#define EPD_WIDTH       800
#define EPD_HEIGHT      480
#define EPD_STEPS      1
#define EPD_BLOCK_SIZE  192000

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
    blockSize = EPD_BLOCK_SIZE;
    stepCommands[0]=0x10;
    
};

/******************************************************************************
function :  Initialize the e-Paper register
parameter:
******************************************************************************/

int Epd::Init(void) {
    
     if (IfInit() != 0) {
        return -1;
    }
    Reset();
   SendCommand(0x04);  // POWER_ON
    WaitUntilIdle();
       
    SendCommand(0xAA);    // CMDH
    SendData(0x49);
    SendData(0x55);
    SendData(0x20);
    SendData(0x08);
    SendData(0x09);
    SendData(0x18);

    SendCommand(0x01);
    SendData(0x3F);
    SendData(0x00);
    SendData(0x32);
    SendData(0x2A);
    SendData(0x0E);
    SendData(0x2A);

    SendCommand(0x00);
    SendData(0x5F);
    SendData(0x69);

    SendCommand(0x03);
    SendData(0x00);
    SendData(0x54);
    SendData(0x00);
    SendData(0x44); 

    SendCommand(0x05);
    SendData(0x40);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x2C);

    SendCommand(0x06);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x16);
    SendData(0x25);

    SendCommand(0x08);
    SendData(0x6F);
    SendData(0x1F);
    SendData(0x1F);
    SendData(0x22);

    SendCommand(0x13);    // IPC
    SendData(0x00);
    SendData(0x04);

    SendCommand(0x30);
    SendData(0x02);

    SendCommand(0x41);     // TSE
    SendData(0x00);

    SendCommand(0x50);
    SendData(0x3F);

    SendCommand(0x60);
    SendData(0x02);
    SendData(0x00);

    SendCommand(0x61);
    SendData(0x03);
    SendData(0x20);
    SendData(0x01); 
    SendData(0xE0);

    SendCommand(0x82);
    SendData(0x1E); 

    SendCommand(0x84);
    SendData(0x00);

    SendCommand(0x86);    // AGID
    SendData(0x00);

    SendCommand(0xE3);
    SendData(0x2F);

    SendCommand(0xE0);   // CCSET
    SendData(0x00); 

    SendCommand(0xE6);   // TSSET
    SendData(0x00);


    
        DelayMs(2000);  
     
    
    return 0;

}
/**
 *  @brief: basic function for sending commands
 */
void Epd::SendCommand(unsigned char command) {
  DelayMs(200);
    DigitalWrite(dc_pin, LOW);
    DelayMs(2);
    SpiTransfer(command);
     DelayMs(20);
      DigitalWrite(dc_pin, HIGH);
}
void Epd::SetToDataMode() {
   DigitalWrite(dc_pin, HIGH);
  
}
/**
 *  @brief: basic function for sending data
 */
void Epd::SendData(unsigned char data) {
 
    SpiTransfer(data);
}

void Epd::WaitUntilIdle(void)// If BUSYN=0 then waiting
{
    while(!DigitalRead(BUSY_PIN)) {
        DelayMs(1);
    }
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void Epd::Reset(void) {
   DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(1);
    DigitalWrite(reset_pin, HIGH);
    DelayMs(1);   
    DigitalWrite(reset_pin, LOW);                //module reset    
    DelayMs(1);
    DigitalWrite(reset_pin, HIGH);
      

    WaitUntilIdle();
}

void Epd::TurnOnDisplay(void) {
  
  
    SendCommand(0x04);  // POWER_ON
    WaitUntilIdle();
    
    SendCommand(0x12);  // DISPLAY_REFRESH
    SendData(0x01);
 
 // Serial.print("DISPLAY_REFRESH=");
    WaitUntilIdle();
  
}

/******************************************************************************
function : 
      Clear screen
******************************************************************************/
void Epd::Clear(unsigned char color) {
    SendCommand(0x10);
    SetToDataMode();
    for(int i=0; i<width/2; i++) {
        for(int j=0; j<height; j++) {
           SendData((color<<4)|color);  //SendData(color);  //  
		}
	}
    TurnOnDisplay();
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          The only one parameter is a check code, the command would be
 *          You can use EPD_Reset() to awaken
 */
void Epd::Sleep(void) {
    SendCommand(0x07);
    SendData(0xA5);
    DelayMs(1000);
	  DigitalWrite(RST_PIN, 0); // Reset
}

#endif

/* END OF FILE */
