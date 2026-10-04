/**
 * EPD Serial Interface
 *
 * Serial communication interface for Waveshare e-paper displays.
 * The panel and the XIAO carrier board are selected in display_config.h.
 *
 * Host protocol (115200 baud; the bridge prints "Ready" when it accepts a frame):
 *   Framed (recommended): "EPX1", uint32 LE payload length (= steps * blockSize), the payload
 *   (plane 0, plane 1, ... exactly as the cloud .bin), uint32 LE CRC-32 of the payload (zlib.crc32).
 *   Bytes before the "EPX1" magic are discarded, so a stray byte cannot shift the image.
 *   Legacy: the raw payload with no header, as older hosts send it.
 * Replies, one line each: "Display updated" then "Ready" on success, "ERROR ..." then "Ready" on failure.
 * Wait for "Ready" before sending the next frame.
 */
#include "display_config.h"

#include <SPI.h>
#include <Arduino.h>
#include "epd_base.h"

#define USE_SERIAL Serial

#define FRAME_IDLE_TIMEOUT_MS 15000UL   /* silence inside a frame that abandons it */
#define HEADER_SCAN_BYTES 64            /* bytes searched for the "EPX1" magic before assuming a legacy frame */
#define STRAY_GAP_MS 1000UL             /* silence after a short burst without the magic: discard it as noise */

static const uint8_t FRAME_MAGIC[4] = {'E', 'P', 'X', '1'};

Epd epd;

static unsigned long frameBytes() { return (unsigned long)epd.steps * epd.blockSize; }

static uint32_t crc32Update(uint32_t crc, uint8_t value) {
  crc ^= value;
  for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
  return crc;
}

// Discard whatever the host is still sending (tail of a bad frame, overrun of a shifted legacy frame).
static void drainInput(unsigned long quietMs) {
  unsigned long lastSeen = millis();
  while (millis() - lastSeen < quietMs) {
    while (USE_SERIAL.available() > 0) { USE_SERIAL.read(); lastSeen = millis(); }
    delay(1);
  }
}

static void reportReady() {
  USE_SERIAL.println("Ready");
}

static void printPanelInfo() {
  USE_SERIAL.print("Panel: code=");
  USE_SERIAL.print(epd.screenCode ? epd.screenCode : "?");
  USE_SERIAL.print(" steps=");
  USE_SERIAL.print(epd.steps);
  USE_SERIAL.print(" blockSize=");
  USE_SERIAL.print(epd.blockSize);
  USE_SERIAL.print(" frameBytes=");
  USE_SERIAL.print(frameBytes());
  USE_SERIAL.println(" protocol=EPX1|raw");
  USE_SERIAL.println(EPD_BOARD_NAME);
}

// Recover the panel after an error: a stuck BUSY latch is cleared once and the controller re-initialised.
static void reinitPanel() {
  epd.ClearBusyTimeout();
  epd.Init();
  if (epd.BusyTimedOut()) USE_SERIAL.println("ERROR busy-timeout: the panel does not respond");
}

// Frame state shared by the readers below.
struct FrameReader {
  unsigned long started;
  unsigned long lastByte;
  unsigned long budget;
  bool readByte(uint8_t& value) {
    while (USE_SERIAL.available() == 0) {
      if (millis() - lastByte > FRAME_IDLE_TIMEOUT_MS || millis() - started > budget) return false;
      delay(1);
    }
    value = (uint8_t)USE_SERIAL.read();
    lastByte = millis();
    return true;
  }
  bool readUint32(uint32_t& value) {
    value = 0;
    for (int i = 0; i < 4; ++i) {
      uint8_t b;
      if (!readByte(b)) return false;
      value |= (uint32_t)b << (8 * i);
    }
    return true;
  }
};

static void failFrame(const char* message) {
  USE_SERIAL.println(message);
  // Drain a late tail for a bounded interval. The sender waits for Ready before restarting.
  drainInput(1000UL);
  reinitPanel();
  reportReady();
}

void setup() {
  USE_SERIAL.begin(115200);

  #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
    Serial.print("ESP32\r\n");
  #endif

  Serial.print("e-Paper init\r\n");

  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed\r\n");
    return;
  }
  epd.ShowDebug = true;
  if (epd.BusyTimedOut()) USE_SERIAL.println("ERROR busy-timeout: the panel does not respond");
  if (!epd.PixelFormatValid()) USE_SERIAL.println("ERROR the display driver does not declare its pixel format");
  // No refresh here: the controller RAM holds nothing worth showing until the first frame arrives.
  printPanelInfo();
  reportReady();
}



void loop() {
  if(USE_SERIAL.available()==0) { delay(1); return; }
  if(epd.BusyTimedOut()) reinitPanel();
  unsigned long total=frameBytes();
  // 10 bits per byte at 115200 baud plus 30 s of slack; 64-bit so a large panel cannot overflow it.
  FrameReader reader{millis(), millis(), 30000UL+(unsigned long)((unsigned long long)total*10000ULL/115200ULL)};

  // Look for the "EPX1" magic in the first bytes. A short burst without it is noise; a long run without
  // it is a legacy headerless frame whose first bytes are pixels.
  uint8_t prefix[HEADER_SCAN_BYTES];
  int prefixLen=0;
  bool framed=false;
  unsigned long lastSeen=millis();
  while(prefixLen<HEADER_SCAN_BYTES) {
    if(USE_SERIAL.available()==0) {
      if(millis()-lastSeen>STRAY_GAP_MS) break;
      delay(1);
      continue;
    }
    prefix[prefixLen++]=(uint8_t)USE_SERIAL.read();
    lastSeen=millis();
    if(prefixLen>=4 && memcmp(prefix+prefixLen-4,FRAME_MAGIC,4)==0) { framed=true; break; }
  }
  reader.lastByte=millis();

  if(framed) {
    if(prefixLen>4) {
      USE_SERIAL.print("Discarded ");
      USE_SERIAL.print(prefixLen-4);
      USE_SERIAL.println(" stray bytes before the frame header");
    }
    uint32_t length=0;
    if(!reader.readUint32(length)) { failFrame("ERROR incomplete header. Wait for Ready; resend the entire frame."); return; }
    if(length!=total) {
      USE_SERIAL.print("ERROR length ");
      USE_SERIAL.print(length);
      USE_SERIAL.print(" does not match this panel (expected ");
      USE_SERIAL.print(total);
      USE_SERIAL.println(" bytes)");
      drainInput(1000UL);
      reportReady();
      return;
    }
    uint32_t crc=0xFFFFFFFFUL;
    for(int plane=0;plane<epd.steps;++plane) {
      epd.SendCommand(epd.stepCommands[plane]); epd.SetToDataMode();
      for(unsigned long remaining=epd.blockSize; remaining>0; --remaining) {
        uint8_t value;
        if(!reader.readByte(value)) { failFrame("ERROR incomplete frame. Stop sending; wait for Ready; resend the entire frame."); return; }
        crc=crc32Update(crc,value);
        epd.SendStreamByte(value,plane);
      }
    }
    uint32_t expectedCrc=0;
    if(!reader.readUint32(expectedCrc)) { failFrame("ERROR missing CRC. Wait for Ready; resend the entire frame."); return; }
    if((crc^0xFFFFFFFFUL)!=expectedCrc) { failFrame("ERROR crc mismatch; the display was not refreshed. Resend the frame."); return; }
  } else {
    if(prefixLen<HEADER_SCAN_BYTES) {
      // A short burst followed by silence (terminal newline, a stray command byte): not a frame.
      USE_SERIAL.print("Discarded ");
      USE_SERIAL.print(prefixLen);
      USE_SERIAL.println(" stray bytes (no frame followed)");
      reportReady();
      return;
    }
    // Legacy headerless frame: the scanned bytes are its first pixels.
    unsigned long consumed=0;
    for(int plane=0;plane<epd.steps;++plane) {
      epd.SendCommand(epd.stepCommands[plane]); epd.SetToDataMode();
      for(unsigned long remaining=epd.blockSize; remaining>0; --remaining, ++consumed) {
        uint8_t value;
        if(consumed<(unsigned long)prefixLen) value=prefix[consumed];
        else if(!reader.readByte(value)) { failFrame("ERROR incomplete frame. Stop sending; wait for Ready; resend the entire frame."); return; }
        epd.SendStreamByte(value,plane);
      }
    }
  }
  epd.TurnOnDisplay();
  if(epd.BusyTimedOut()) {
    USE_SERIAL.println("ERROR busy-timeout: the panel did not finish the refresh");
  } else {
    USE_SERIAL.println("Display updated");
  }
  // Anything still arriving belongs to the frame just shown (e.g. the overrun of a shifted legacy frame);
  // dropping it keeps the next frame aligned.
  drainInput(200UL);
  reportReady();
}
