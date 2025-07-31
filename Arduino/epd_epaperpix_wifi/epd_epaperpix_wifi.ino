/**
 * ESP32 E-Paper Display WiFi Client
 *  Important #ifndef the display you want to use other displays must be all ifndef
 */

/**
************Currently set to EPD7IN5_V2_C********************
*/



// Type definitions
#define UBYTE unsigned char

#include <SPI.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "epd_base.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>



#include <WiFiClientSecure.h>

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  180        /* Time ESP32 will go to sleep (in seconds) */
#define USE_SERIAL Serial

// EEPROM size and addresses
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 50
#define DEV_ID 100
#define SUB_ID 110
#define USR_ID 145
#define SCR_NAME 170
#define GET_INFO_ADDR 199
#define CONFIG_FLAG_ADDR 200

// Network and timing constants
#define WIFI_TIMEOUT 10000        /* WiFi connection timeout in milliseconds */
#define RETRY_DELAY 500           /* Delay between retries in milliseconds */
#define LOOP_DELAY 10000          /* Main loop delay in milliseconds */
#define DOWNLOAD_BUFFER_SIZE 128  /* Buffer size for downloading data */
#define LARGE_BUFFER_SIZE 1024    /* Large buffer size for data processing */
#define DEFAULT_SLEEP_TIME 60     /* Default sleep time in seconds */
#define RETRY_SLEEP_TIME 120      /* Sleep time after retry in seconds */
#define LOOP_TIMEOUT 5            /* Maximum loop count before sleep */
#define TIMEOUT_SLEEP 600         /* Sleep time when loop times out */
#define MAX_RETRIES 5             /* Maximum number of download retries */
#define DISPLAY_RETRY_COUNT 10    /* Maximum display retry attempts */
#define DISPLAY_THRESHOLD 145     /* Display readiness threshold */
#define DISPLAY_RETRY_DELAY 2000  /* Delay between display retries */
#define DOWNLOAD_DELAY 100        /* Delay after download completion */
#define EEPROM_STRING_SIZE 32     /* Maximum size for EEPROM strings */
#define EEPROM_PASS_SIZE 64       /* Maximum size for EEPROM password */
#define JSON_DOC_SIZE 200         /* JSON document size for parsing */
#define TIMEOUT_COUNTER 20000     /* Timeout counter for operations */

// API endpoints
#define API_BASE_URL "https://api.epaperpix.app"
// TODO: Move API keys to EEPROM or secure storage instead of hardcoding
#define API_START_ENDPOINT "/epaper2/DSlideShowStart/"   
#define API_FILE_ENDPOINT "/epaper2/DSlideShow/"     
#define API_DEVICE_ENDPOINT "/epaper3/GetDeviceInfo/id/"
#define BLOB_URL_PRIMARY "https://blob.epaperpix.app/epaperfiles/"
#define BLOB_URL_SECONDARY "https://epaperstore.blob.core.windows.net/epaperfiles/"

// Access Point settings for configuration mode
const char* AP_SSID = "epaperpix_WiFi_Setup";
const char* AP_PASSWORD = "configme";  // You can change this or leave it empty for open AP

// HTML templates stored in PROGMEM to save RAM
const char HTML_STYLE[] PROGMEM = R"(
  body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }
  .container { max-width: 400px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
  h1 { color: #333; text-align: center; }
  label { display: block; margin-top: 10px; font-weight: bold; }
  input[type=text], input[type=password] { width: 100%; padding: 8px; margin-top: 5px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 3px; box-sizing: border-box; }
  input[type=submit], button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 3px; cursor: pointer; }
  input[type=submit]:hover, button:hover { background-color: #45a049; }
  button.red { background-color: #f44336; }
  button.red:hover { background-color: #d32f2f; }
  .buttons { display: flex; justify-content: space-between; margin-top: 20px; }
  .status { margin-top: 20px; padding: 10px; border-radius: 3px; }
  .success { background-color: #dff0d8; color: #3c763d; }
  .error { background-color: #f2dede; color: #a94442; }
  .info { background-color: #d9edf7; color: #31708f; }
)";

const char HTML_HEADER[] PROGMEM = R"(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 WiFi Setup</title>
<style>%s</style></head>
<body><div class="container">
)";

const char HTML_FOOTER[] PROGMEM = R"(
</div></body></html>
)";

// Web server on port 80
WebServer server(80);


// Variables to store WiFi credentials
String storedSSID = "";
String storedPassword = "";
String storedUserId = "";
String storedDeviceId = "";
String storedSubId = "";
String storedScreenName = "";
bool configComplete = false;
bool apconnected =false;

struct SlideShowStatus
{
  const char *filename;
  bool gotosleep;
  long secondsdelay;
  bool didcall;
  bool retry;
} ;

 StaticJsonDocument<JSON_DOC_SIZE> doc;

void setClock() {
  configTime(0, 0, "pool.ntp.org");

  USE_SERIAL.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    USE_SERIAL.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  USE_SERIAL.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  USE_SERIAL.print(F("Current time: "));
  USE_SERIAL.print(asctime(&timeinfo));
}


WiFiMulti WiFiMulti;
uint8_t screenbuf1[LARGE_BUFFER_SIZE];
Epd epd;
int loopCount=0;
int needDeviceIfo=0;


esp_sleep_wakeup_cause_t wakeup_reason;

void print_wakeup_reason(){
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0 : USE_SERIAL.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : USE_SERIAL.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : USE_SERIAL.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : USE_SERIAL.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : USE_SERIAL.println("Wakeup caused by ULP program"); break;
    case ESP_SLEEP_WAKEUP_GPIO : USE_SERIAL.println("Wakeup caused by GPIO (light sleep only)"); break;
    case ESP_SLEEP_WAKEUP_UART : USE_SERIAL.println("Wakeup caused by UART (light sleep only)"); break;
    default : USE_SERIAL.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void setup() {
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
 //  clearCredentials();
 

   USE_SERIAL.begin(115200);
  //  clearEEPROM();
  //   for(;;)
  //   {
  //     USE_SERIAL.print("*");
  //     delay(2000);

  //   }
  needDeviceIfo = loadCredentials();
  USE_SERIAL.print("needDeviceIfo =");
  USE_SERIAL.println(needDeviceIfo);
  
  if (epd.Init() != 0) {
    USE_SERIAL.println("Failed to initialize EPD");
    return;
  }
  
  // Enable debug output for troubleshooting
  epd.ShowDebug = true;
  
  //epd.TurnOnDisplay();

  //epd.Reset();
   
  


  print_wakeup_reason();
  if (storedSSID.length() > 0 && EEPROM.read(CONFIG_FLAG_ADDR) == 1) {
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(storedSSID.c_str(), storedPassword.c_str());
    
    // wait for WiFi connection
    USE_SERIAL.print("Waiting for WiFi to connect...");
    return;
  }
  
  epd.ClearFrame();
  //epd.Clear(0xFF);      // Clear to white background (handles dual-buffer prep)
  epd.QRset(6);    // Draw QR code (single step)
  epd.TurnOnDisplay(); // Refresh display
  startConfigPortal();
  
  while(apconnected == false) {
    USE_SERIAL.print("a");
    delay(200);
  }
  //epd.Clear(0x2);

}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    USE_SERIAL.println("WiFiEvent");
      USE_SERIAL.println(event);
  switch(event) {
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
     
      USE_SERIAL.println("Station connected:");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
         USE_SERIAL.println("Station passed:");
         apconnected = true;
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
         USE_SERIAL.println("disconned");
      break;
       case ARDUINO_EVENT_WIFI_STA_GOT_IP:
         USE_SERIAL.println("got_ip");
         configComplete = true;
      break;
      case ARDUINO_EVENT_WIFI_STA_START:
         USE_SERIAL.println("sta start");
         configComplete = true;
      break;
    
    default:
      break;
  }
}


HTTPClient https;

SlideShowStatus GetStart()
{
  SlideShowStatus slideshowstatus;
  String result = "";
 String message = "{}";
  
  //default if errors
  slideshowstatus.retry =true;
  slideshowstatus.gotosleep = true;
  slideshowstatus.secondsdelay = DEFAULT_SLEEP_TIME;
 
      USE_SERIAL.print("[HTTPS] begin...\n");
      String url =String(API_BASE_URL) + String(API_START_ENDPOINT) + String(storedUserId)+String("/") +String(storedScreenName)+String("/")+String("?subscription-key=")+String(storedSubId) ;
       USE_SERIAL.println(url);
      if (  https.begin(url ) ) {  
        USE_SERIAL.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
         https.addHeader("Content-Type", "application/json");
        int httpCode = https.POST(message);
      
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          USE_SERIAL.printf("[HTTPS] POST epaper2... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            result = https.getString();
             USE_SERIAL.println(result.c_str());
            DeserializationError err = deserializeJson(doc, result.c_str());
            if (err) {
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                  USE_SERIAL.println(err.c_str());
                   slideshowstatus.gotosleep = true;
                   slideshowstatus.secondsdelay = RETRY_SLEEP_TIME;

            }else
            {
                slideshowstatus.retry = false;
            
                slideshowstatus.gotosleep = doc["gotoSleep"];
                slideshowstatus.secondsdelay = doc["secondsDelay"];
            
               
                USE_SERIAL.println(slideshowstatus.gotosleep);
                USE_SERIAL.println(slideshowstatus.secondsdelay);
            }


            return slideshowstatus;
          }
        } else {
          USE_SERIAL.printf("[HTTPS] GET... failed, code: %d  error: %s\n",httpCode, https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        USE_SERIAL.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    
    return slideshowstatus;
}

SlideShowStatus GetFile()
{
  SlideShowStatus slideshowstatus;
  String result = "";
 String message = "{}";
  
  //default if errors
  slideshowstatus.retry =true;
  slideshowstatus.gotosleep = true;
  slideshowstatus.secondsdelay = DEFAULT_SLEEP_TIME;
 
      
      USE_SERIAL.print("[HTTPS] begin...\n");
      String url =String(API_BASE_URL) + String(API_FILE_ENDPOINT) + String(storedUserId)+String("/") +String(storedScreenName)+String("/")+String("?subscription-key=")+String(storedSubId) ;
      USE_SERIAL.println(url);
      if (  https.begin(url) ) {  // HTTPS  https://epaper2.azurewebsites.net/api/DSync?code=KVkxT7cdOSTEe-SJ6GDuXmwNkIJvBO9ZKajMgnAm4qyfAzFuutuBWQ==
        USE_SERIAL.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
         https.addHeader("Content-Type", "application/json");
    int httpCode = https.POST(message);
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          USE_SERIAL.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            result = https.getString();
             USE_SERIAL.println(result.c_str());
            DeserializationError err = deserializeJson(doc, result.c_str());
            if (err) {
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                  USE_SERIAL.println(err.c_str());
                   slideshowstatus.gotosleep = true;
                   slideshowstatus.secondsdelay = RETRY_SLEEP_TIME;

            }else
            {
                slideshowstatus.retry = false;
                slideshowstatus.filename = doc["fileName"];
                slideshowstatus.gotosleep = doc["gotoSleep"];
                slideshowstatus.secondsdelay = doc["secondsDelay"];
                slideshowstatus.didcall = true;
                USE_SERIAL.println(slideshowstatus.filename);
                USE_SERIAL.println(slideshowstatus.gotosleep);
                USE_SERIAL.println(slideshowstatus.secondsdelay);
            }


            return slideshowstatus;
          }
        } else {
          USE_SERIAL.printf("[HTTPS] GET... failed, code: %d  error: %s\n",httpCode, https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        USE_SERIAL.printf("[HTTPS] Unable to connect\n");
      }

    return slideshowstatus;
}

void GoToSleep(long seconds)
{
  
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                epd.Sleep();
                esp_sleep_enable_timer_wakeup(seconds * uS_TO_S_FACTOR);
                USE_SERIAL.println("Setup ESP32 to sleep for every " + String(seconds) +  " Seconds");
                USE_SERIAL.println("Going to sleep now");
                USE_SERIAL.flush(); 
                esp_deep_sleep_start();

}

int loopcount=0;
bool once=false;
SlideShowStatus slideShowStatus;
int dstatus=0;


void loop() {
  if (!configComplete) {
    // Handle client requests in configuration mode
    server.handleClient();
    return;
  } 
 

  loopcount++;
   USE_SERIAL.print("Loop = ");
   USE_SERIAL.println(loopcount);
    if(loopcount > LOOP_TIMEOUT) {
    GoToSleep(TIMEOUT_SLEEP);  
  }  
  if(WiFiMulti.run() == WL_CONNECTED) {
      setClock();  
      delay(200);
      USE_SERIAL.print("Device Id = ");
      USE_SERIAL.println(storedDeviceId);
      if(needDeviceIfo)
      {
        delay(2000);
        for(int retryd = 0;retryd < 3;retryd++) {
           dstatus=getDeviceInfo(storedDeviceId);
           if(dstatus > 0) {
            wakeup_reason = ESP_SLEEP_WAKEUP_UNDEFINED ; // reset to zero to forece screen refresh;
             break;
           }
           else
             delay(2000);
        }
      }

      USE_SERIAL.print("User Id = ");
      USE_SERIAL.println(storedUserId);
      USE_SERIAL.print("Screen Name = ");
      USE_SERIAL.println(storedScreenName);
      if (storedUserId.length() == 0 || storedScreenName.length() == 0 || storedSubId.length() == 0) {
        USE_SERIAL.println(" empty  info");
          delay(2000);
          GoToSleep(TIMEOUT_SLEEP);  
        return;
      }

      if(slideShowStatus.didcall == false)
          slideShowStatus = GetStart();
      while (slideShowStatus.retry)
      {
        USE_SERIAL.println("Retry...");
        delay(500);
        slideShowStatus= GetStart();
      }
      USE_SERIAL.println();
      USE_SERIAL.println("Waiting 10s before the next round...");
      delay(500);
      //MustRefresh();

    if(slideShowStatus.gotosleep == true && wakeup_reason != 0)
      {
        GoToSleep(slideShowStatus.secondsdelay);  
      }  
      slideShowStatus= GetFile(); 
      if(slideShowStatus.filename != "")
      {
         for(int i=0;i< MAX_RETRIES ; i++)
        {
           dstatus = DownloadAndDisplay(slideShowStatus.filename,slideShowStatus.secondsdelay,i);
           if(dstatus > 0)
             break;
            else{
                USE_SERIAL.print("Retry DownloadAndDisplay, status: ");
        USE_SERIAL.println(dstatus);
        delay(5000);
            }
             
        }
       
      }else
      {
          epd.Clear(0x1);
         delay(500);
      }
  } else {
    delay(1000);
  }

  delay(LOOP_DELAY);
}
void Download2(String filename)
{
      String fullPath ="https://epaperstore.blob.core.windows.net/epaperfiles/"+filename;
      USE_SERIAL.println("[fullPath]");
      USE_SERIAL.println(fullPath);
}      

int DownloadAndDisplay(String filename, long sleepseconds, int retrycnt) {
  int displaycnt = 0;
      String fullPath = String(BLOB_URL_PRIMARY) + filename;
      String fullPath2 = String(BLOB_URL_SECONDARY) + filename;

      if(retrycnt > 1)
        fullPath = fullPath2;
      USE_SERIAL.println("[fullPath]");
      USE_SERIAL.println(fullPath);

      https.begin(fullPath);

        USE_SERIAL.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
          USE_SERIAL.printf("[HTTPS] A GET... code: %d\n", httpCode);
        if(httpCode > 0) {
             USE_SERIAL.printf("[HTTPS] B GET... code: %d\n", httpCode);
               if(httpCode == HTTP_CODE_OK) {

                // get length of document (is -1 when Server sends no Content-Length header)
                int len = https.getSize();
                USE_SERIAL.printf("[HTTPS] Len: %d\n", len);
 
               
                int offset1=0;
                // create buffer for read
                uint8_t buff[DOWNLOAD_BUFFER_SIZE] = { 0 };
                int maxread = sizeof(buff);
                int timeoutcnt = TIMEOUT_COUNTER;
                // get tcp stream
                WiFiClient * stream = https.getStreamPtr();
                  if(!https.connected())
                    return -2;
                 USE_SERIAL.println("Starting display update");
                 USE_SERIAL.print("Steps: ");
                 USE_SERIAL.println(epd.steps);
                 USE_SERIAL.print("Block size: ");
                 USE_SERIAL.println(epd.blockSize);
                 
                 // Use step-based approach like working serial version
                 for(int step = 0; step < epd.steps; step++) {
                   long stepLen = epd.blockSize;
                   epd.SendCommand(epd.stepCommands[step]);
                   epd.SetToDataMode();
                   
                   USE_SERIAL.print("Processing step  BBB ");
                   USE_SERIAL.print(step);
                   USE_SERIAL.print(" with ");
                   USE_SERIAL.print(stepLen);
                   USE_SERIAL.println(" bytes");
                   
                   if (len == -1) {
                     // Unknown length - read all available data
                     while (https.connected() && (stream->available() || stream->connected()) && stepLen > 0) {
                       size_t size = stream->available();
                       if (size > 0) {
                         size_t readBytes = stream->readBytes(buff, min(size, min((size_t)sizeof(buff), (size_t)stepLen)));
                         for(int i = 0; i < readBytes; i++) {
                            if(step==0) 
                             
                              epd.SendDataFast(buff[i]);
                            
                            else
                             epd.SendDataFast(buff[i]);
                         }
                         stepLen -= readBytes;
                         offset1 += readBytes;
                       }
                       delay(1);
                     }
                   } else {
                     // Known length - read exactly what we need
                     while(https.connected() && len > 0 && stepLen > 0) {
                       size_t size = stream->available();
                       if(size) {
                         int c = stream->readBytes(buff, min(size, min(sizeof(buff), min((size_t)len, (size_t)stepLen))));
                         for(int i = 0; i < c; i++) {
                           if(step==0) 
                              
                              epd.SendDataFast(buff[i]);
                            
                            else
                             epd.SendDataFast(buff[i]);
                         
                         }
                         offset1 += c;
                         stepLen -= c;
                         if(len > 0) {
                           len -= c;
                         }
                       } else {
                         timeoutcnt--;
                         delay(2);
                       }
                       delay(1);
                     }
                   }
                 }
          


                USE_SERIAL.println();
                USE_SERIAL.printf("Total bytes processed: %d\n", offset1);
                USE_SERIAL.print("[HTTP] connection closed or file end.\n");
                delay(DOWNLOAD_DELAY);
                
                // Turn on display to show the downloaded image
                USE_SERIAL.println("Turning on display...");
                epd.TurnOnDisplay();
                USE_SERIAL.println("Display updated successfully");
                
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                USE_SERIAL.println("GoToSleep");
                GoToSleep(sleepseconds);
                  
             
          return 1;
        }  
      }
        return 0;
  }        

int getDeviceInfo(String deviceId) {
     String result = "";

       USE_SERIAL.print("[HTTPS] begin...\n");
      if (  https.begin(String(API_BASE_URL) + String(API_DEVICE_ENDPOINT)+String(deviceId) ) ) {  
        USE_SERIAL.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        // https.addHeader("Content-Type", "application/json");
        int httpCode = https.GET();
      
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          USE_SERIAL.printf("[HTTPS] GET epaper3... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            result = https.getString();
             USE_SERIAL.println(result.c_str());
            DeserializationError err = deserializeJson(doc, result.c_str());
            if (err) {
                  USE_SERIAL.print(F("deserializeJson() failed: "));
                  USE_SERIAL.println(err.c_str());
                  return 0;

            } else {
                // Check if required fields exist in JSON response
                if (doc.containsKey("AccountID") && doc.containsKey("SlideShowId") && doc.containsKey("subscriptionKey")) {
                    // Safely extract values with null checks
                    const char* accountId = doc["AccountID"];
                    const char* slideShowId = doc["SlideShowId"];
                    const char* subscriptionKey = doc["subscriptionKey"];
                    
                    if (accountId != nullptr && slideShowId != nullptr && subscriptionKey != nullptr) {
                        storedUserId = String(accountId);
                        storedScreenName = String(slideShowId);
                        storedSubId = String(subscriptionKey);
                        
                        USE_SERIAL.println("Device info extracted successfully:");
                        USE_SERIAL.print("AccountID: ");
                        USE_SERIAL.println(storedUserId);
                        USE_SERIAL.print("SlideShowId: ");
                        USE_SERIAL.println(storedScreenName);
                        USE_SERIAL.print("SubscriptionKey: ");
                        USE_SERIAL.println(storedSubId);
                        
                        saveDeviceInfo(storedSubId, storedUserId, storedScreenName);
                    } else {
                        USE_SERIAL.println("Error: One or more fields contain null values");
                    }
                } else {
                    USE_SERIAL.println("Error: Missing required fields in JSON response");
                    USE_SERIAL.println("Expected fields: AccountID, SlideShowId, subscriptionKey");
                }
            }


            return 1;
          }
        } else {
          USE_SERIAL.printf("[HTTPS] GET... failed, code: %d  error: %s\n",httpCode, https.errorToString(httpCode).c_str());
          return -2;
        }
  
        https.end();
      } else {
        USE_SERIAL.printf("[HTTPS] Unable to connect\n");
         return -1;
      }

}

void startConfigPortal() {
  // Set up Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
   WiFi.onEvent(WiFiEvent);
  IPAddress IP = WiFi.softAPIP();
  USE_SERIAL.print("AP IP address: ");
  USE_SERIAL.println(IP);
  USE_SERIAL.println("WiFi configuration portal started");
  USE_SERIAL.print("Connect to WiFi network: ");
  USE_SERIAL.println(AP_SSID);
  if (strlen(AP_PASSWORD) > 0) {
    USE_SERIAL.print("Password: ");
    USE_SERIAL.println(AP_PASSWORD);
  } else {
    USE_SERIAL.println("No password required");
  }
  USE_SERIAL.print("Then access the configuration page at: http://");
  USE_SERIAL.println(IP);
  
  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_GET, handleReset);
  server.onNotFound(handleNotFound);
  
  // Start the server
  server.begin();
  USE_SERIAL.println("HTTP server started");
}

int loadCredentials() {
  // Check if we have saved credentials (marked by flag at CONFIG_FLAG_ADDR)
  if (EEPROM.read(CONFIG_FLAG_ADDR) == 1) {
    // Read SSID
    storedSSID = "";
    for (int i = 0; i < EEPROM_STRING_SIZE; i++) {
      char c = EEPROM.read(SSID_ADDR + i);
      if (c == 0) break;
      storedSSID += c;
    }
    
    // Read Password
    storedPassword = "";
    for (int i = 0; i < 50; i++) {
      char c = EEPROM.read(PASS_ADDR + i);
      if (c == 0) break;
      storedPassword += c;
    }
     // Read DeviceId 
    storedDeviceId = "";
    for (int i = 0; i < 50; i++) {
      char c = EEPROM.read(DEV_ID + i);
      if (c == 0) break;
      storedDeviceId += c;
    }

      if (EEPROM.read(GET_INFO_ADDR) == 1) {
          return 1; // need to read device info;
      }
 // Read Subcription id 
    storedSubId = "";
    for (int i = 0; i < 50; i++) {
      char c = EEPROM.read(SUB_ID + i);
      if (c == 0) break;
      storedSubId += c;
    }

    // Read UserID
    storedUserId = "";
    for (int i = 0; i < 50; i++) {
      char c = EEPROM.read(USR_ID + i);
      if (c == 0) break;
      storedUserId += c;
    }

    // Read UserID
    storedScreenName = "";
    for (int i = 0; i < 50; i++) {
      char c = EEPROM.read(SCR_NAME + i);
      if (c == 0) break;
      storedScreenName += c;
    }


    USE_SERIAL.println("Loaded credentials from EEPROM:");
    USE_SERIAL.print("SSID: ");
    USE_SERIAL.println(storedSSID);
    USE_SERIAL.print("storedUserId: ");
    USE_SERIAL.println(storedUserId);
    USE_SERIAL.println("Password: [hidden]");

    return 0;
  } else {
    USE_SERIAL.println("No saved credentials found in EEPROM");
  }
  return -1;
}


void saveCredentials(String ssid, String password,String deviceId) {
  // Input validation
  if (ssid.length() == 0 || ssid.length() > EEPROM_STRING_SIZE - 1) {
    USE_SERIAL.println("Invalid SSID length");
    return;
  }
  
  if (password.length() > EEPROM_PASS_SIZE - 1) {
    USE_SERIAL.println("Password too long");
    return;
  }
  
  // Clear areas
  for (int i = 0; i < EEPROM_STRING_SIZE; i++)
    EEPROM.write(SSID_ADDR + i, 0);
  for (int i = 0; i < EEPROM_PASS_SIZE; i++)
    EEPROM.write(PASS_ADDR + i, 0);
  
  // Write SSID
  for (int i = 0; i < ssid.length(); i++)
    EEPROM.write(SSID_ADDR + i, ssid[i]);
  
  // Write Password
  for (int i = 0; i < password.length(); i++)
    EEPROM.write(PASS_ADDR + i, password[i]);

  // Write deviceId
  for (int i = 0; i < deviceId.length(); i++)
    EEPROM.write(DEV_ID + i, deviceId[i]);
  
  
  // Set  flag to get device info
  EEPROM.write(GET_INFO_ADDR, 1);
  // Set configuration flag
  EEPROM.write(CONFIG_FLAG_ADDR, 1);
  
  // Commit changes to EEPROM
  EEPROM.commit();
  
  USE_SERIAL.println("Saved credentials to EEPROM:");
  USE_SERIAL.print("SSID: ");
  USE_SERIAL.println(ssid);
  USE_SERIAL.println("Password: [hidden]");
  
  storedSSID = ssid;
  storedPassword = password;
  storedDeviceId = deviceId;
}


void saveDeviceInfo(String subscriptionId, String userId, String screenName) {
  // Input validation
  if (subscriptionId.length() == 0 || userId.length() == 0 || screenName.length() == 0) {
    USE_SERIAL.println("Error: Cannot save empty device info");
    return;
  }
  
  if (subscriptionId.length() > 50 || userId.length() > 50 || screenName.length() > 50) {
    USE_SERIAL.println("Error: Device info fields too long");
    return;
  }
  
  // Clear existing data first
  for (int i = 0; i < 50; i++) {
    EEPROM.write(SUB_ID + i, 0);
    EEPROM.write(USR_ID + i, 0);
    EEPROM.write(SCR_NAME + i, 0);
  }
  
  // Write subscriptionId
  for (int i = 0; i < subscriptionId.length(); i++)
    EEPROM.write(SUB_ID + i, subscriptionId[i]);
  
  // Write userId
  for (int i = 0; i < userId.length(); i++)
    EEPROM.write(USR_ID + i, userId[i]);

  // Write screenName
  for (int i = 0; i < screenName.length(); i++)
    EEPROM.write(SCR_NAME + i, screenName[i]);
  
  
  // Reset  flag to get device info
  EEPROM.write(GET_INFO_ADDR,0);
  // Set configuration flag
  EEPROM.write(CONFIG_FLAG_ADDR, 1);
  
  // Commit changes to EEPROM
  EEPROM.commit();
  
  USE_SERIAL.println("Saved device info to EEPROM:");
  USE_SERIAL.print("subscriptionId: ");
  USE_SERIAL.println(subscriptionId);
   USE_SERIAL.print("userId: ");
  USE_SERIAL.println(userId);
   USE_SERIAL.print("screenName: ");
  USE_SERIAL.println(screenName);
  USE_SERIAL.println("Password: [hidden]");
  
  storedSubId = subscriptionId;
  storedUserId = userId;
  storedScreenName = screenName;

}

void clearCredentials() {
  // Clear the configuration flag
  EEPROM.write(CONFIG_FLAG_ADDR, 0);
  EEPROM.commit();

  storedSSID = "";
  storedPassword = "";
  // storedSubId= "";
  // storedScreenName ="";
  // storedDeviceId = "";
  
  USE_SERIAL.println("Cleared  Wifi Credentials");
}


void clearEEPROM() {
  // Clear the configuration flag
  //EEPROM.write(CONFIG_FLAG_ADDR, 0);
  //EEPROM.commit();

  // Clear existing data first
  for (int i = 0; i < CONFIG_FLAG_ADDR+5; i++) {
    EEPROM.write(i, 0);
  }

  EEPROM.commit();
  storedSSID = "";
  storedPassword = "";
  storedSubId= "";
  storedScreenName ="";
  storedDeviceId = "";
  
  USE_SERIAL.println("Cleared  EEPROM");
}

void handleRoot() {
  String html;
  char buffer[2048];
  
  // Build HTML header with style
  sprintf(buffer, HTML_HEADER, HTML_STYLE);
  html = String(buffer);
  
  html += "<h1>WiFi Setup</h1>";
  
  if (storedSSID.length() > 0 && EEPROM.read(CONFIG_FLAG_ADDR) == 1) {
    html += "<div class=\"status success\">Currently configured WiFi: <strong>" + storedSSID + "</strong></div>";
  }
  
  html += "<form action=\"/save\" method=\"POST\">";
  html +="<label for=\"wifi-ssid\">Wi-Fi Name (SSID)</label>";
  html +="<input type=\"text\" id=\"wifi-ssid\" name=\"username\" autocomplete=\"username\" placeholder=\"MyHomeWiFi\">";
  html +="<label for=\"wifi-password\">Wi-Fi Password</label>";
  html +="<input type=\"password\" id=\"wifi-password\" name=\"wifi-password\" autocomplete=\"current-password\" placeholder=\"*******\">";
 // html += "<label for=\"password\">WiFi Password:</label>";
 // html += "<input type=\"password\" id=\"password\" name=\"password\" value=\"" + storedPassword + "\">";
 // html += "<label for=\"ssid\">SSID:</label>";
 // html += "<input type=\"text\" id=\"ssid1\" name=\"ssid\" value=\"" + storedSSID + "\">";
  html += "<label for=\"device-id\">Device Id:</label>";
  html += "<input type=\"text\" id=\"device-id\" name=\"device-id\" autocomplete=\"off\" value=\"" + storedDeviceId + "\">";
  html += "<div class=\"buttons\">";
  html += "<input type=\"submit\" value=\"Save & Connect\">";
  html += "<button type=\"button\" class=\"red\" onclick=\"window.location.href='/reset'\">Reset</button>";
  html += "</div></form>";
  html += "<p><small>After saving, this device will attempt to connect to your WiFi network.</small></p>";
  html += FPSTR(HTML_FOOTER);
  
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("username") && server.hasArg("wifi-password")  && server.hasArg("device-id")) {
    String ssid = server.arg("username");
    String password = server.arg("wifi-password");
    String deviceid = server.arg("device-id");
    
    // Input validation and sanitization
    ssid.trim();
    password.trim();
    deviceid.trim();
    
    if (ssid.length() > 0 && ssid.length() <= EEPROM_STRING_SIZE - 1 && 
        password.length() <= EEPROM_PASS_SIZE - 1) {
      saveCredentials(ssid, password,deviceid);
      
      String html;
      char buffer[2048];
      
      // Build HTML header with style
      sprintf(buffer, HTML_HEADER, HTML_STYLE);
      html = String(buffer);
      
      html += "<meta http-equiv=\"refresh\" content=\"10;url=/\">";
      html += "<h1>WiFi Setup</h1>";
      html += "<div class=\"status success\">WiFi credentials saved! Attempting to connect...</div>";
      html += "<p>If connection is successful, this page will no longer be available.</p>";
      html += "<p>If connection fails, the configuration portal will remain active.</p>";
      html += "<p>This page will refresh in 10 seconds...</p>";
      html += FPSTR(HTML_FOOTER);
      
      server.send(200, "text/html", html);
      
      // Attempt to connect with new credentials
      delay(500);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());
      
      long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
        delay(500);
        USE_SERIAL.print(".");
      }
      USE_SERIAL.println();
      
      if (WiFi.status() == WL_CONNECTED) {
        USE_SERIAL.println("Connected to WiFi!");
        USE_SERIAL.print("IP address: ");
        USE_SERIAL.println(WiFi.localIP());
        configComplete = true;
         USE_SERIAL.print("Reboot: ");
         GoToSleep(2);

      } else {
        USE_SERIAL.println("Failed to connect with new credentials");
        // Restart AP mode
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
      }
    } else {
      server.send(400, "text/plain", "Invalid SSID or password length");
    }
  } else {
    server.send(400, "text/plain", "Missing required parameters");
  }
}

void handleReset() {
  clearCredentials();
  
  String html;
  char buffer[2048];
  
  // Build HTML header with style
  sprintf(buffer, HTML_HEADER, HTML_STYLE);
  html = String(buffer);
  
  html += "<meta http-equiv=\"refresh\" content=\"3;url=/\">";
  html += "<h1>WiFi Setup</h1>";
  html += "<div class=\"status info\">WiFi credentials have been reset.</div>";
  html += "<p>Redirecting back to setup page in 3 seconds...</p>";
  html += FPSTR(HTML_FOOTER);
  
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page Not Found");
}