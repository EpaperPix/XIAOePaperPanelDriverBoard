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
#include "FrameTransfer.h"
#include <WiFiClientSecure.h>
#include "TrustedRoots.h"
void GoToSleep(long seconds);
bool validDeviceId(const String& value);



#include <WiFiClientSecure.h>

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  180        /* Time ESP32 will go to sleep (in seconds) */
#define USE_SERIAL Serial

// EEPROM size and addresses
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 33
#define DEV_ID 98
#define SUB_ID 163
#define USR_ID 228
#define SCR_NAME 293
#define GET_INFO_ADDR 498
#define CONFIG_FLAG_ADDR 499

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
#define EEPROM_STRING_SIZE 33     /* Maximum size for EEPROM strings */
#define EEPROM_PASS_SIZE 65       /* Maximum size for EEPROM password */
#define JSON_DOC_SIZE 1024         /* JSON document size for parsing */
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
  String filename;
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
  unsigned long ntpStarted=millis();
  while (nowSecs < 8 * 3600 * 2) {
    if(millis()-ntpStarted>=15000UL) { GoToSleep(600); return; }
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

   // Do this to clear eeprom
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

  unsigned long portalWait=millis();
  while(apconnected == false && millis()-portalWait<300000UL) {
    server.handleClient();
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

// Device-scoped bearer authorization; legacy provisioned devices can still poll with their existing key.
SlideShowStatus fetchSlide(bool advance) {
  SlideShowStatus status{};
  status.retry=true; status.gotosleep=true; status.secondsdelay=60;
  WiFiClientSecure secure;
  secure.setCACert(EPAPER_ROOT_CA);
  HTTPClient request;
  request.setConnectTimeout(10000); request.setTimeout(15000);
  String endpoint;
  bool deviceAuth=validDeviceId(storedDeviceId);
  if(deviceAuth) endpoint=String(API_BASE_URL)+(advance?"/epaper3/DeviceSlideShow":"/epaper3/DeviceSlideShowStart");
  else endpoint=String(API_BASE_URL)+(advance?String(API_FILE_ENDPOINT):String(API_START_ENDPOINT))+storedUserId+"/"+storedScreenName;
  if(!request.begin(secure,endpoint)) return status;
  request.addHeader("Content-Type","application/json");
  if(deviceAuth) request.addHeader("Authorization",String("Bearer ")+storedDeviceId);
  else request.addHeader("Ocp-Apim-Subscription-Key",storedSubId);
  int code=request.POST("{}");
  if(code==HTTP_CODE_OK && request.getSize()<=4096) {
    StaticJsonDocument<1024> result;
    auto error=deserializeJson(result,request.getString());
    if(!error && result["gotoSleep"].is<bool>() && result["secondsDelay"].is<long>()) {
      status.secondsdelay=constrain(result["secondsDelay"].as<long>(),1L,604800L);
      status.gotosleep=result["gotoSleep"].as<bool>();
      if(!advance || result["fileName"].is<const char*>()) {
        status.filename=result["fileName"].as<String>();
        if(!advance || (status.filename.indexOf('/')<0 && status.filename.indexOf('\\')<0 && status.filename.indexOf("..")<0)) {
          status.retry=false; status.didcall=true;
        }
      }
    }
  }
  request.end();
  return status;
}
SlideShowStatus GetStart() { return fetchSlide(false); }



SlideShowStatus GetFile() { return fetchSlide(true); }

void GoToSleep(long seconds)
{

                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                epd.Sleep();
                esp_sleep_enable_timer_wakeup(constrain(seconds,1L,604800L) * uS_TO_S_FACTOR);
                USE_SERIAL.println("Setup ESP32 to sleep for every " + String(seconds) +  " Seconds");
                USE_SERIAL.println("Going to sleep now");
                USE_SERIAL.flush();
                esp_deep_sleep_start();

}

int loopcount=0;
bool once=false;
SlideShowStatus slideShowStatus{};
int dstatus=0;


void loop() {
  if (!configComplete) {
    static unsigned long recoveryStarted=millis();
    if(millis()-recoveryStarted>300000UL) { GoToSleep(600); return; }
    delay(1);
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
      if (storedUserId.length() == 0 || storedScreenName.length() == 0 || (!validDeviceId(storedDeviceId) && storedSubId.length() == 0)) {
        USE_SERIAL.println(" empty  info");
          delay(2000);
          GoToSleep(TIMEOUT_SLEEP);
        return;
      }

      if(slideShowStatus.didcall == false)
          slideShowStatus = GetStart();
      for(int apiAttempt=0; slideShowStatus.retry && apiAttempt<3; ++apiAttempt)
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
      if(slideShowStatus.retry) { GoToSleep(600); return; }
      slideShowStatus = GetFile();
      if(slideShowStatus.retry) { GoToSleep(600); return; }
      if(!slideShowStatus.retry && slideShowStatus.filename.length()>0)
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
  WiFiClientSecure secure;
  secure.setCACert(EPAPER_ROOT_CA);
  HTTPClient request;
  request.setConnectTimeout(10000); request.setTimeout(15000);
  String url=String(retrycnt>1?BLOB_URL_SECONDARY:BLOB_URL_PRIMARY)+filename;
  if(!request.begin(secure,url)) return 0;
  if(request.GET()!=HTTP_CODE_OK) { request.end(); return 0; }
  long expected=(long)epd.steps*epd.blockSize;
  if(expected<=0 || request.getSize()!=expected) { request.end(); return -3; }
  for(int step=0;step<epd.steps;++step) {
    epd.SendCommand(epd.stepCommands[step]); epd.SetToDataMode();
    bool complete=readFramePlane(request,epd.blockSize,[&](uint8_t* data,int count) {
      for(int i=0;i<count;++i) epd.SendDataFast(data[i]);
    });
    if(!complete) { request.end(); return -2; }
  }
  request.end(); epd.TurnOnDisplay();
  GoToSleep(sleepseconds); return 1;
}


int getDeviceInfo(String deviceId) {
  if(!validDeviceId(deviceId)) return 0;
  WiFiClientSecure secure;
  secure.setCACert(EPAPER_ROOT_CA);
  HTTPClient request;
  request.setConnectTimeout(10000); request.setTimeout(15000);
  if(!request.begin(secure,String(API_BASE_URL)+"/epaper3/DeviceInfo")) return 0;
  request.addHeader("Authorization",String("Bearer ")+deviceId);
  request.addHeader("Content-Type","application/json");
  int code=request.POST("{}");
  bool valid=false;
  if(code==HTTP_CODE_OK && request.getSize()<=4096) {
    StaticJsonDocument<1024> result;
    if(!deserializeJson(result,request.getString()) && result["AccountID"].is<const char*>() && result["SlideShowId"].is<const char*>()) {
      String user=result["AccountID"].as<String>(), screen=result["SlideShowId"].as<String>();
      valid=user.length()>0 && user.length()<=64 && screen.length()>0 && screen.length()<=64;
      if(valid) { saveDeviceInfo("",user,screen); needDeviceIfo=0; }
    }
  }
  request.end(); return valid?1:0;
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

// Version 2 uses disjoint, null-terminated slots. Read legacy data before writing any new slot.
String readSetting(int address, int capacity) {
  String value;
  for (int i=0; i<capacity; ++i) {
    byte c=EEPROM.read(address+i);
    if (c==0) return value;
    if (c==255 || c<32) return String();
    value += char(c);
  }
  return String(); // No terminator: legacy overlap or invalid record.
}
void writeSetting(int address, int capacity, const String& value) {
  for(int i=0;i<capacity;++i) EEPROM.write(address+i, i<value.length()?value[i]:0);
}
bool validDeviceId(const String& value) {
  if(value.length()!=32) return false;
  for(unsigned i=0;i<value.length();++i) if(!isxdigit(value[i])) return false;
  return true;
}
void migrateLegacySettings() {
  if(EEPROM.read(500)==0xE2 && EEPROM.read(501)==2) return;
  bool valid=EEPROM.read(200)==1;
  bool needInfo=EEPROM.read(199)==1;
  String ssid=readSetting(0,33), pass=readSetting(50,50), device=readSetting(100,10);
  String sub=readSetting(110,35), user=readSetting(145,25), screen=readSetting(170,29);
  // Corrupted legacy fields cannot be reconstructed. Require provisioning in that case.
  valid=valid && (pass.length()>0 || EEPROM.read(50)==0) && ssid.length()>0 && device.length()>0 && !needInfo && sub.length()>0 && user.length()>0 && screen.length()>0;
  writeSetting(SSID_ADDR,33,ssid); writeSetting(PASS_ADDR,65,pass); writeSetting(DEV_ID,65,device);
  writeSetting(SUB_ID,65,sub); writeSetting(USR_ID,65,user); writeSetting(SCR_NAME,65,screen);
  EEPROM.write(GET_INFO_ADDR,0); EEPROM.write(CONFIG_FLAG_ADDR,valid?1:0);
  EEPROM.write(500,0xE2); EEPROM.write(501,2); EEPROM.commit();
}
int loadCredentials() {
  migrateLegacySettings();
  storedSSID=readSetting(SSID_ADDR,33); storedPassword=readSetting(PASS_ADDR,65);
  storedDeviceId=readSetting(DEV_ID,65); storedSubId=readSetting(SUB_ID,65);
  storedUserId=readSetting(USR_ID,65); storedScreenName=readSetting(SCR_NAME,65);
  if(EEPROM.read(CONFIG_FLAG_ADDR)!=1) return -1;
  return EEPROM.read(GET_INFO_ADDR)==1?1:0;
}



void saveCredentials(String ssid, String password, String deviceId) {
  if(ssid.length()==0 || ssid.length()>32 || password.length()>64 || !validDeviceId(deviceId)) return;
  writeSetting(SSID_ADDR,33,ssid); writeSetting(PASS_ADDR,65,password); writeSetting(DEV_ID,65,deviceId);
  writeSetting(SUB_ID,65,""); writeSetting(USR_ID,65,""); writeSetting(SCR_NAME,65,"");
  EEPROM.write(GET_INFO_ADDR,1); EEPROM.write(CONFIG_FLAG_ADDR,1);
  EEPROM.write(500,0xE2); EEPROM.write(501,2); EEPROM.commit();
  storedSSID=ssid; storedPassword=password; storedDeviceId=deviceId;
  storedUserId=""; storedScreenName=""; storedSubId="";
}



void saveDeviceInfo(String subscriptionId, String userId, String screenName) {
  if(userId.length()==0 || screenName.length()==0 || userId.length()>64 || screenName.length()>64 || subscriptionId.length()>64) return;
  writeSetting(SUB_ID,65,subscriptionId); writeSetting(USR_ID,65,userId); writeSetting(SCR_NAME,65,screenName);
  EEPROM.write(GET_INFO_ADDR,0); EEPROM.write(CONFIG_FLAG_ADDR,1); EEPROM.commit();
  storedSubId=subscriptionId; storedUserId=userId; storedScreenName=screenName;
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
    deviceid.trim();
    if(!validDeviceId(deviceid)) { server.send(400,"text/plain","Enter the 32-character device code from the portal"); return; }

    // Input validation and sanitization
    ssid.trim();
    // Password whitespace is significant.
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