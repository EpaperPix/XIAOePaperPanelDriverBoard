/**
 * EPD Serial Interface
 * 
 * Serial communication interface for Waveshare e-paper displays
 */
// Uncomment the display type you're using:
// #define EPD7IN5B_C      // 7.5" B/W/Red
// #define EPD7IN5_V2_C    // 7.5" Monochrome
// #define EPD7IN3F_C      // 7.3" Color
#define EPD4IN01F_C     // 4.01" Color

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
  char byte1;
  unsigned long len;
  unsigned long cnt = 0;

  
  USE_SERIAL.println("Waiting for data...");
  USE_SERIAL.print("Steps: ");
  USE_SERIAL.println(epd.steps);
  USE_SERIAL.print("Block size: ");
  USE_SERIAL.println(epd.blockSize);
  epd.SendCommand(0x24); 
  for(int i = 0; i < epd.steps; i++) {
    len = epd.blockSize;
    epd.SendCommand(epd.stepCommands[i]);
    epd.SetToDataMode();
    
    while (len > 0) {
      if (USE_SERIAL.available() > 0) {
        byte1 = USE_SERIAL.read();
        if(i==0)
          byte1 = ~byte1;

        epd.SendDataFast(byte1);
        len--;
        cnt++;
      } else {
        if (cnt == 0) {
          USE_SERIAL.print(".");
          delay(10);
        }
      }
    }
  }
 
 epd.TurnOnDisplay();
  
 // epd.TurnOnDisplay();
  USE_SERIAL.println("\nDisplay updated");
}