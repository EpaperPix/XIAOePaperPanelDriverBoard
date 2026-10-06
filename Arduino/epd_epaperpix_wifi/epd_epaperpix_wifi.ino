/**
 * ESP32 E-Paper Display WiFi Client for epaperpix.app
 * The panel and the XIAO carrier board are selected in display_config.h.
 */

/**
************Currently set to EPD2IN9_C (2.9inch V1, SSD1608) via display_config.h on the SeeedStudio ePaper Driver board; XIAO ESP32-S3/C3/C6********************
*/

#include "display_config.h"

// Type definitions
#define UBYTE unsigned char

#include <SPI.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "epd_base.h"
#include <WiFi.h>
#include "WiFiEventLog.h"
#include "ClockSync.h"
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
long nextFailureSleep();


#include <WiFiClientSecure.h>

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
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
#define WIFI_TIMEOUT 20000        /* Portal credential test timeout in milliseconds */
#define WIFI_FAIL_CYCLES_FOR_PORTAL 3 /* Consecutive wakes without WiFi before the portal reopens */
#define AUTH_FAIL_WAKES_FOR_PORTAL 3  /* Consecutive wakes answered HTTP 401 before the portal reopens */
#define LOOP_DELAY 10000          /* Main loop delay in milliseconds */
#define DEFAULT_SLEEP_TIME 60     /* Default sleep time in seconds */
#define LOOP_TIMEOUT 5            /* WiFi attempts (loop passes) per wake before giving up */
#define TIMEOUT_SLEEP 600         /* Sleep time after an unattended portal times out */
#define MIN_SECONDS_DELAY 30      /* Floor on the server's secondsDelay for normal responses */
#define BACKOFF_FIRST_SLEEP 60    /* First sleep after a failed wake; doubles per consecutive failure */
#define BACKOFF_MAX_SLEEP 3600    /* Longest failure back-off */
#define DOWNLOAD_ATTEMPTS 3       /* Attempts per wake for transient download errors (the last uses the fallback host) */
#define EEPROM_STRING_SIZE 33     /* Maximum size for EEPROM strings */
#define EEPROM_PASS_SIZE 65       /* Maximum size for EEPROM password */
#ifndef EPD_CHECK_SCREEN_CODE
#define EPD_CHECK_SCREEN_CODE 1   /* Refuse a .bin whose name says it was built for another screen code */
#endif

// ArduinoJson 7 sizes documents on the heap and deprecates StaticJsonDocument; 6.x needs the fixed size.
#if ARDUINOJSON_VERSION_MAJOR >= 7
typedef JsonDocument ResponseJson;
#else
typedef StaticJsonDocument<1024> ResponseJson;
#endif

// API endpoints
#define API_BASE_URL "https://api.epaperpix.app"
#define API_START_ENDPOINT "/epaper2/DSlideShowStart/"
#define API_FILE_ENDPOINT "/epaper2/DSlideShow/"
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
String portalToken = "";   // per-boot token the reset form must echo
bool configComplete = false;
bool apconnected =false;
// Survive deep sleep, cleared by a power cycle.
RTC_DATA_ATTR int wifiFailCycles = 0;      // consecutive wakes that never reached WiFi
RTC_DATA_ATTR uint8_t apiFailCycles = 0;   // consecutive failed wakes: drives the 60 s .. 1 h back-off
RTC_DATA_ATTR uint8_t authFailCycles = 0;  // consecutive wakes answered HTTP 401 (unknown or revoked code)
RTC_DATA_ATTR bool screenStale = false;    // the setup QR replaced the picture: redraw on the next good poll
RTC_DATA_ATTR uint8_t portalReason = 0;    // why the portal reopened, shown on its page
#define PORTAL_REASON_WIFI 1
#define PORTAL_REASON_AUTH 2
bool wifiConnectedThisWake = false;

struct SlideShowStatus
{
  String filename;
  bool gotosleep;
  long secondsdelay;
  bool retry;          // true: the call failed
  int httpCode;        // HTTP status, or a negative HTTPClient transport error
  long serverDelay;    // > 0: a 503 said how long to wait (15..3600 s)
} ;

// Download results
#define DL_OK 1
#define DL_TRANSIENT 0     // transport error, 5xx or interrupted body: may retry in this wake
#define DL_TERMINAL -3     // size mismatch or HTTP 4xx: retrying in this wake cannot help
#define DL_PANEL -4        // the panel kept BUSY asserted



WiFiMulti WiFiMulti;
Epd epd;
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
 //  wipeSettings();
 

   USE_SERIAL.begin(115200);
  needDeviceIfo = loadCredentials();
  USE_SERIAL.println(needDeviceIfo == 1 ? "Device info: fetching it from the server this wake"
                     : needDeviceIfo == 0 ? "Device info: stored" : "Device info: not set up yet");
  USE_SERIAL.println(EPD_BOARD_NAME);
  
  if (epd.Init() != 0) {
    USE_SERIAL.println("Failed to initialize EPD");
    return;
  }
  
  // Enable debug output for troubleshooting
  epd.ShowDebug = true;
  if (!epd.PixelFormatValid()) {
    USE_SERIAL.println("ERROR: the display driver does not declare its pixel format; nothing will be drawn");
  }

  print_wakeup_reason();
  // Stored WiFi keeps failing (router or password changed), or the server keeps rejecting the device code:
  // reopen the portal so it can be fixed without a reflash. The counters restart, so an unattended portal
  // timeout goes back to normal polling (with the back-off still in force).
  bool authReopen = authFailCycles >= AUTH_FAIL_WAKES_FOR_PORTAL;
  bool reopenPortal = authReopen || wifiFailCycles >= WIFI_FAIL_CYCLES_FOR_PORTAL;
  if (reopenPortal) {
    USE_SERIAL.println(authReopen ? "Device code rejected on repeated wakes; reopening the config portal"
                                  : "WiFi failed on repeated wakes; reopening the config portal");
    portalReason = authReopen ? PORTAL_REASON_AUTH : PORTAL_REASON_WIFI;
    wifiFailCycles = 0;
    authFailCycles = 0;
  }
  if (!reopenPortal && storedSSID.length() > 0 && EEPROM.read(CONFIG_FLAG_ADDR) == 1) {
    if (epd.BusyTimedOut()) {
      // The panel never released BUSY during Init (wrong carrier board, loose FPC, dead panel).
      USE_SERIAL.println("Panel not responding; sleeping");
      GoToSleep(nextFailureSleep());
    }
    configComplete = true;
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(storedSSID.c_str(), storedPassword.c_str());
    
    // wait for WiFi connection
    USE_SERIAL.println("Waiting for WiFi to connect...");
    return;
  }
  if (!reopenPortal) portalReason = 0;
  
  epd.ClearFrame();
  epd.QRset(6);    // Draw QR code; the scale is reduced to fit smaller panels
  epd.TurnOnDisplay(); // Refresh display
  screenStale = true;  // the next successful poll redraws the picture even in holding mode
  startConfigPortal();
  
  unsigned long portalWait=millis();
  while(apconnected == false && millis()-portalWait<300000UL) {
    server.handleClient();
    USE_SERIAL.print("a");
    delay(200);
  }

}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  logWiFiEvent(event, info);
  // The setup portal waits for a phone or laptop to get an address on the setup network.
  if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) apconnected = true;
}


// Contract C6: a 503 from the device API carries {"status":"pending","secondsDelay":n} and Retry-After: n.
// Returns n when it is 15..3600, else 0 (fall back to the back-off).
long retryDelayFrom503(HTTPClient& request) {
  long seconds=0;
  int size=request.getSize();
  if(size>0 && size<=1024) {
    ResponseJson body;
    if(!deserializeJson(body,request.getString()) && body["secondsDelay"].is<long>()) seconds=body["secondsDelay"].as<long>();
  }
  if(seconds==0 && request.hasHeader("Retry-After")) seconds=request.header("Retry-After").toInt();
  return (seconds>=15 && seconds<=3600) ? seconds : 0;
}

// Device-scoped bearer authorization; legacy provisioned devices can still poll with their existing key.
SlideShowStatus fetchSlide(bool advance) {
  SlideShowStatus status{};
  status.retry=true; status.gotosleep=true; status.secondsdelay=DEFAULT_SLEEP_TIME; status.httpCode=-1;
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
  const char* responseHeaders[]={"Retry-After","Date"};
  request.collectHeaders(responseHeaders,2);
  int code=request.POST("{}");
  setClockFromHttpDate(request.header("Date"));   // keeps the clock right without NTP
  status.httpCode=code;
  if(code==HTTP_CODE_OK && request.getSize()<=4096) {
    ResponseJson result;
    auto error=deserializeJson(result,request.getString());
    if(!error && result["gotoSleep"].is<bool>() && result["secondsDelay"].is<long>()) {
      status.secondsdelay=constrain(result["secondsDelay"].as<long>(),(long)MIN_SECONDS_DELAY,604800L);
      status.gotosleep=result["gotoSleep"].as<bool>();
      if(advance && result["fileName"].isNull()) {
        status.retry=false;   // empty slideshow: nothing to show, sleep secondsDelay
      } else if(!advance || result["fileName"].is<const char*>()) {
        status.filename=result["fileName"].as<String>();
        if(!advance || (status.filename.indexOf('/')<0 && status.filename.indexOf('\\')<0 && status.filename.indexOf("..")<0)) {
          status.retry=false;
        }
      }
    }
  } else if(code==HTTP_CODE_SERVICE_UNAVAILABLE) {
    status.serverDelay=retryDelayFrom503(request);
  }
  request.end();
  return status;
}
SlideShowStatus GetStart() { return fetchSlide(false); }



SlideShowStatus GetFile() { return fetchSlide(true); }

void GoToSleep(long seconds)
{
                if (configComplete && !wifiConnectedThisWake) wifiFailCycles++;
                WiFi.disconnect(true);
                WiFi.mode(WIFI_OFF);
                epd.Sleep();
                esp_sleep_enable_timer_wakeup(constrain(seconds,1L,604800L) * uS_TO_S_FACTOR);
                USE_SERIAL.println("Sleeping for " + String(seconds) + " s");
                USE_SERIAL.flush(); 
                esp_deep_sleep_start();

}

// Exponential back-off for consecutive failed wakes (WiFi, NTP, API, unresponsive panel):
// 60, 120, 240, ... capped at 3600 s. Reset by any successful poll.
long nextFailureSleep() {
  long seconds=(long)BACKOFF_FIRST_SLEEP << (apiFailCycles<6 ? apiFailCycles : 6);
  if(seconds>BACKOFF_MAX_SLEEP) seconds=BACKOFF_MAX_SLEEP;
  if(apiFailCycles<255) apiFailCycles++;
  return seconds;
}
void failureSleep() { GoToSleep(nextFailureSleep()); }

// HTTP 401: the device code is unknown or was revoked. After AUTH_FAIL_WAKES_FOR_PORTAL consecutive wakes
// like this the device restarts into the setup portal so a new code can be entered.
void authFailureSleep() {
  if(authFailCycles<255) authFailCycles++;
  USE_SERIAL.print("Device code rejected (HTTP 401), wake ");
  USE_SERIAL.println(authFailCycles);
  if(authFailCycles>=AUTH_FAIL_WAKES_FOR_PORTAL) GoToSleep(1);
  failureSleep();
}

// A failed API call ends the wake: 401 counts toward the portal, a 503 with a delay is obeyed, anything
// else backs off. Returns only when the call succeeded.
void endWakeIfFailed(const SlideShowStatus& status) {
  if(!status.retry) return;
  USE_SERIAL.print("API call failed, HTTP ");
  USE_SERIAL.println(status.httpCode);
  if(status.httpCode==HTTP_CODE_UNAUTHORIZED) authFailureSleep();
  if(status.serverDelay>0) GoToSleep(status.serverDelay);
  failureSleep();
}
bool worthRetrying(const SlideShowStatus& status) {
  return status.retry && status.serverDelay==0 && (status.httpCode<0 || status.httpCode>=500 || status.httpCode==HTTP_CODE_OK);
}

// XS-14: cloud .bin names start with the screen type they were built for ("{type}_{hash}.bin"; the type may
// carry a panel prefix such as 730G and a flip suffix T). Refuse a file built for another base code; names
// that do not follow the pattern pass (the Content-Length check still applies).
bool screenCodeMatches(const String& fileName) {
#if EPD_CHECK_SCREEN_CODE
  int underscore=fileName.indexOf('_');
  int query=fileName.indexOf('?');
  if(underscore<=0 || underscore>16 || (query>=0 && query<underscore) || epd.screenCode==nullptr) return true;
  String type=fileName.substring(0,underscore);
  for(unsigned i=0;i<type.length();++i) if(!isalnum((unsigned char)type[i])) return true;
  type.toUpperCase();
  if(type.length()>1 && type.endsWith("T")) type.remove(type.length()-1);
  static const char* const bases[]={"ARS","ERS","V2A","AR","AS","BR","ER","ES","FR","GR","A","B","E","F","G"};
  for(const char* base: bases) {
    if(type.endsWith(base)) return strcmp(base,epd.screenCode)==0;
  }
#endif
  return true;
}


void loop() {
  if (!configComplete) {
    static unsigned long recoveryStarted=millis();
    if(millis()-recoveryStarted>300000UL) { GoToSleep(TIMEOUT_SLEEP); return; }
    delay(1);
    // Handle client requests in configuration mode
    server.handleClient();
    return;
  } 
 
  static int loopcount=0;
  loopcount++;
  if(loopcount > LOOP_TIMEOUT) {
    USE_SERIAL.println("WiFi: no connection this wake");
    failureSleep();   // no WiFi this wake
  }
  USE_SERIAL.printf("WiFi attempt %d of %d\n", loopcount, LOOP_TIMEOUT);
  if(WiFiMulti.run() != WL_CONNECTED) {
    delay(1000);
    delay(LOOP_DELAY);
    return;
  }
  wifiConnectedThisWake = true;
  wifiFailCycles = 0;
  syncClock();   // ClockSync.h: NTP only when the clock was never set, and never fatal
  delay(200);
  pollOnce();
}

// One poll per wake. Every path ends in deep sleep, and the slideshow advances at most once.
void pollOnce() {
  if(needDeviceIfo)
  {
    delay(2000);
    int info=0;
    for(int retryd = 0;retryd < 3;retryd++) {
      info=getDeviceInfo(storedDeviceId);
      if(info!=0) break;
      delay(2000);
    }
    if(info==-HTTP_CODE_UNAUTHORIZED) authFailureSleep();
    if(info>0) wakeup_reason = ESP_SLEEP_WAKEUP_UNDEFINED ; // reset to zero to force screen refresh
  }

  USE_SERIAL.print("User Id = ");
  USE_SERIAL.println(storedUserId);
  USE_SERIAL.print("Screen Name = ");
  USE_SERIAL.println(storedScreenName);
  if (storedUserId.length() == 0 || storedScreenName.length() == 0 || (!validDeviceId(storedDeviceId) && storedSubId.length() == 0)) {
    USE_SERIAL.println(" empty  info");
    failureSleep();
  }

  SlideShowStatus start = GetStart();
  for(int apiAttempt=0; worthRetrying(start) && apiAttempt<3; ++apiAttempt)
  {
    USE_SERIAL.println("Retry...");
    delay(500);
    start = GetStart();
  }
  endWakeIfFailed(start);
  apiFailCycles = 0;      // successful poll: back-off and 401 count start over
  authFailCycles = 0;

  // On a timer wake the server may say nothing changed. A cold boot, fresh device info, or a picture
  // overwritten by the setup QR always fetches and draws the current item.
  bool mustRefresh = wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED || screenStale;
  if(start.gotosleep && !mustRefresh) GoToSleep(start.secondsdelay);

  SlideShowStatus slide = GetFile();   // advances the server-side cursor: called exactly once per wake
  endWakeIfFailed(slide);
  if(slide.filename.length()==0) {
    USE_SERIAL.println("Nothing to show");
    GoToSleep(slide.secondsdelay);
  }
  if(!screenCodeMatches(slide.filename)) {
    USE_SERIAL.print("Skipping an image built for another screen type; this panel shows ");
    USE_SERIAL.println(epd.screenCode);
    GoToSleep(slide.secondsdelay);
  }
  for(int attempt=0; attempt<DOWNLOAD_ATTEMPTS; attempt++)
  {
    int result = DownloadAndDisplay(slide.filename, attempt);
    if(result == DL_OK) { screenStale = false; GoToSleep(slide.secondsdelay); }
    if(result == DL_PANEL) { long backoff=nextFailureSleep(); GoToSleep(max(slide.secondsdelay, backoff)); }
    USE_SERIAL.print("DownloadAndDisplay failed, status: ");
    USE_SERIAL.println(result);
    if(result == DL_TERMINAL) break;
    delay(5000);
  }
  // A failed download ends the wake; the next wake moves on instead of advancing again now.
  GoToSleep(slide.secondsdelay);
}

int DownloadAndDisplay(const String& filename, int attempt) {
  if(!epd.PixelFormatValid()) return DL_TERMINAL;
  WiFiClientSecure secure;
  secure.setCACert(EPAPER_ROOT_CA);
  HTTPClient request;
  request.setConnectTimeout(10000); request.setTimeout(15000);
  String url=String(attempt>=DOWNLOAD_ATTEMPTS-1?BLOB_URL_SECONDARY:BLOB_URL_PRIMARY)+filename;
  if(!request.begin(secure,url)) return DL_TRANSIENT;
  int code=request.GET();
  if(code!=HTTP_CODE_OK) { request.end(); return (code>=400 && code<500) ? DL_TERMINAL : DL_TRANSIENT; }
  long expected=(long)epd.steps*epd.blockSize;
  if(expected<=0 || request.getSize()!=expected) { request.end(); return DL_TERMINAL; }
  for(int step=0;step<epd.steps;++step) {
    epd.SendCommand(epd.stepCommands[step]); epd.SetToDataMode();
    bool complete=readFramePlane(request,epd.blockSize,[&](uint8_t* data,int count) {
      for(int i=0;i<count;++i) epd.SendStreamByte(data[i],step);
    });
    if(!complete) { request.end(); return DL_TRANSIENT; }
  }
  request.end(); epd.TurnOnDisplay();
  return epd.BusyTimedOut() ? DL_PANEL : DL_OK;
}


// Returns 1 on success, -401 when the server rejects the device code, 0 on any other failure.
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
    ResponseJson result;
    if(!deserializeJson(result,request.getString()) && result["AccountID"].is<const char*>() && result["SlideShowId"].is<const char*>()) {
      String user=result["AccountID"].as<String>(), screen=result["SlideShowId"].as<String>();
      valid=user.length()>0 && user.length()<=64 && screen.length()>0 && screen.length()<=64;
      if(valid) { saveDeviceInfo("",user,screen); needDeviceIfo=0; }
    }
  }
  request.end();
  if(code==HTTP_CODE_UNAUTHORIZED) return -HTTP_CODE_UNAUTHORIZED;
  return valid?1:0;
}


void startConfigPortal() {
  // Set up Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
   WiFi.onEvent(WiFiEvent);
  portalToken = String(esp_random(), HEX);
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
  server.on("/reset", HTTP_POST, handleReset);   // POST only: a link or prefetch cannot erase settings
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
String htmlEscape(const String& value) {
  String out;
  out.reserve(value.length());
  for(unsigned i=0;i<value.length();++i) {
    char c=value[i];
    if(c=='&') out+="&amp;"; else if(c=='<') out+="&lt;"; else if(c=='>') out+="&gt;";
    else if(c=='"') out+="&quot;"; else if(c=='\'') out+="&#39;"; else out+=c;
  }
  return out;
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


// Factory reset: zero-fill every stored secret and identifier (SSID, password, device code, subscription,
// account, screen name), clear both flags and commit. The v2 layout marker stays so nothing is "migrated".
void wipeSettings() {
  writeSetting(SSID_ADDR,33,""); writeSetting(PASS_ADDR,65,""); writeSetting(DEV_ID,65,"");
  writeSetting(SUB_ID,65,""); writeSetting(USR_ID,65,""); writeSetting(SCR_NAME,65,"");
  EEPROM.write(GET_INFO_ADDR,0); EEPROM.write(CONFIG_FLAG_ADDR,0);
  EEPROM.write(500,0xE2); EEPROM.write(501,2); EEPROM.commit();
  storedSSID = ""; storedPassword = ""; storedDeviceId = "";
  storedSubId = ""; storedUserId = ""; storedScreenName = "";
  USE_SERIAL.println("All stored settings erased");
}

// The stored device code is the device's bearer credential: never send it back, only its last 4 characters.
String maskedDeviceId() {
  if(!validDeviceId(storedDeviceId)) return String();
  return "&bull;&bull;&bull;&bull;" + htmlEscape(storedDeviceId.substring(storedDeviceId.length()-4));
}

void handleRoot() {
  String html;
  char buffer[2048];
  
  // Build HTML header with style
  sprintf(buffer, HTML_HEADER, HTML_STYLE);
  html = String(buffer);
  
  html += "<h1>WiFi Setup</h1>";
  if (portalReason == PORTAL_REASON_AUTH) {
    html += "<div class=\"status error\">The server did not accept the stored device code. Enter the device code shown for this device in the epaperpix.app portal.</div>";
  } else if (portalReason == PORTAL_REASON_WIFI) {
    html += "<div class=\"status error\">The stored WiFi network could not be reached on the last few attempts.</div>";
  }
  
  bool configured = storedSSID.length() > 0 && EEPROM.read(CONFIG_FLAG_ADDR) == 1;
  if (configured) {
    html += "<div class=\"status success\">Currently configured WiFi: <strong>" + htmlEscape(storedSSID) + "</strong></div>";
  }
  String masked = maskedDeviceId();
  
  html += "<form action=\"/save\" method=\"POST\">";
  html +="<label for=\"wifi-ssid\">Wi-Fi Name (SSID)</label>";
  html +="<input type=\"text\" id=\"wifi-ssid\" name=\"username\" autocomplete=\"username\" maxlength=\"32\" placeholder=\"" + String(configured ? "Leave blank to keep the current network" : "MyHomeWiFi") + "\">";
  html +="<label for=\"wifi-password\">Wi-Fi Password</label>";
  html +="<input type=\"password\" id=\"wifi-password\" name=\"wifi-password\" autocomplete=\"current-password\" maxlength=\"64\" placeholder=\"" + String(configured ? "Leave blank to keep the saved password" : "*******") + "\">";
  html += "<label for=\"device-id\">Device Id:</label>";
  html += "<input type=\"text\" id=\"device-id\" name=\"device-id\" autocomplete=\"off\" maxlength=\"32\" placeholder=\"" + String(masked.length() > 0 ? "Leave blank to keep the saved code" : "32-character code from the portal") + "\">";
  if (masked.length() > 0) html += "<p><small>Saved device code ends in " + masked + ".</small></p>";
  html += "<div class=\"buttons\">";
  html += "<input type=\"submit\" value=\"Save & Connect\">";
  html += "</div></form>";
  html += "<p><small>Saving tests the connection first (up to 20 seconds); nothing is stored unless it connects. A blank password keeps the saved one only when the network name is unchanged.</small></p>";
  html += "<form action=\"/reset\" method=\"POST\" onsubmit=\"return confirm('Erase the WiFi settings and the device code?')\">";
  html += "<input type=\"hidden\" name=\"token\" value=\"" + portalToken + "\">";
  html += "<label><input type=\"checkbox\" name=\"confirm\" value=\"yes\" required> Erase all saved settings</label>";
  html += "<div class=\"buttons\"><button type=\"submit\" class=\"red\">Reset</button></div></form>";
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
    // Password whitespace is significant.
    deviceid.trim();
    // Blank fields keep what is stored, so a re-opened portal can fix one setting without re-typing (or
    // ever displaying) the others. A blank password is kept only for the same network name.
    if (ssid.length() == 0) ssid = storedSSID;
    if (password.length() == 0 && ssid == storedSSID) password = storedPassword;
    if (deviceid.length() == 0) deviceid = storedDeviceId;
    if(!validDeviceId(deviceid)) { server.send(400,"text/plain","Enter the 32-character device code from the portal"); return; }
    
    if (ssid.length() > 0 && ssid.length() <= EEPROM_STRING_SIZE - 1 && 
        password.length() <= EEPROM_PASS_SIZE - 1) {
      // Test before saving: stored details that cannot connect would skip the portal on every boot.
      // AP_STA keeps the soft AP (and this request) alive while the station tries the router.
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(ssid.c_str(), password.c_str());
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
        delay(500);
        USE_SERIAL.print(".");
      }
      USE_SERIAL.println();
      bool connected = WiFi.status() == WL_CONNECTED;

      String html;
      char buffer[2048];
      sprintf(buffer, HTML_HEADER, HTML_STYLE);
      html = String(buffer);
      html += "<h1>WiFi Setup</h1>";
      if (connected) {
        saveCredentials(ssid, password, deviceid);
        html += "<div class=\"status success\">Connected to <strong>" + htmlEscape(ssid) + "</strong>. Settings saved; the device is restarting.</div>";
      } else {
        html += "<div class=\"status error\">Could not connect to <strong>" + htmlEscape(ssid) + "</strong>. Nothing was saved.</div>";
        html += "<p>Check the WiFi name and password and try again.</p>";
        html += "<p><a href=\"/\">Back to setup</a></p>";
      }
      html += FPSTR(HTML_FOOTER);
      server.send(200, "text/html", html);

      if (connected) {
        USE_SERIAL.println("Connected to WiFi!");
        USE_SERIAL.print("IP address: ");
        USE_SERIAL.println(WiFi.localIP());
        configComplete = true;
        wifiFailCycles = 0;
        authFailCycles = 0;
        portalReason = 0;
        wifiConnectedThisWake = true;
        delay(1000); // let the response reach the phone
        USE_SERIAL.print("Reboot: ");
        GoToSleep(2);
      } else {
        USE_SERIAL.println("Failed to connect with new credentials");
        WiFi.disconnect();
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
  if (server.arg("confirm") != "yes" || portalToken.length() == 0 || server.arg("token") != portalToken) {
    server.send(400, "text/plain", "Tick the confirmation box on the setup page to erase the settings");
    return;
  }
  wipeSettings();
  
  String html;
  char buffer[2048];
  
  // Build HTML header with style
  sprintf(buffer, HTML_HEADER, HTML_STYLE);
  html = String(buffer);
  
  html += "<meta http-equiv=\"refresh\" content=\"3;url=/\">";
  html += "<h1>WiFi Setup</h1>";
  html += "<div class=\"status info\">All saved settings (WiFi and device code) have been erased.</div>";
  html += "<p>Redirecting back to setup page in 3 seconds...</p>";
  html += FPSTR(HTML_FOOTER);
  
  server.send(200, "text/html", html);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page Not Found");
}
