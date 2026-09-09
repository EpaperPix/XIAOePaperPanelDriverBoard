/**
 * EPD Serial Interface
 * 
 * Serial communication interface for Waveshare e-paper displays
 */
// Uncomment the display type you're using:
// #define EPD7IN5B_C      // 7.5" B/W/Red
// #define EPD7IN5_V2_C    // 7.5" Monochrome
// #define EPD7IN3F_C      // 7.3" Color
#include "display_config.h"

#include <SPI.h>
#include <Arduino.h>
#include "epd_base.h"

#define USE_SERIAL Serial

Epd epd;
void setup() {
  USE_SERIAL.begin(115200);

  #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3
    Serial.print("ESP32\r\n");
  #endif

  Serial.print("e-Paper init\r\n");
  
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed\r\n");
    return;
  }
  epd.ShowDebug = true;
 // epd.TurnOnDisplay();
 //Some screens needs the buffer to clear
//  USE_SERIAL.println("ClearFrame");
//  epd.Clear(0xFF);
//  epd.TurnOnDisplay();
//delay(3000);
//epd.ClearFrame();
 //epd.QRset(4);
 epd.TurnOnDisplay();

 USE_SERIAL.println("Ready");
}



void loop() {
  if(USE_SERIAL.available()==0) { delay(1); return; }
  unsigned long started=millis(), lastByte=started;
  unsigned long budget=30000UL+(unsigned long)(epd.steps*epd.blockSize)*10000UL/115200UL;
  for(int plane=0;plane<epd.steps;++plane) {
    epd.SendCommand(epd.stepCommands[plane]); epd.SetToDataMode();
    unsigned long remaining=epd.blockSize;
    while(remaining>0) {
      if(USE_SERIAL.available()>0) {
        unsigned char value=(unsigned char)USE_SERIAL.read();
        epd.SendDataFast(epd.EncodePixelByte(value,plane));
        --remaining; lastByte=millis();
      } else delay(1);
      if(millis()-lastByte>15000UL || millis()-started>budget) {
        USE_SERIAL.println("ERROR incomplete frame. Stop sending; wait for Ready; resend the entire frame.");
        // Drain a late tail for a bounded interval. The sender waits for Ready before restarting.
        unsigned long drainStarted=millis();
        while(millis()-drainStarted<1000UL) { while(USE_SERIAL.available()>0) USE_SERIAL.read(); delay(1); }
        epd.Init();
        USE_SERIAL.println("Ready");
        return;
      }
    }
  }
  epd.TurnOnDisplay();
  USE_SERIAL.println("Display updated");
  USE_SERIAL.println("Ready");
}