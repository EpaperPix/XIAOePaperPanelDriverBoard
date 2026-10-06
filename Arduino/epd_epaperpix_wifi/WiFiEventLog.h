#pragma once
// One readable serial line per WiFi event. The core numbers its events (100 WIFI_OFF, 101 WIFI_READY,
// 102 SCAN_DONE, 110 STA_START, 111 STA_STOP, 112 STA_CONNECTED, 113 STA_DISCONNECTED, 115 STA_GOT_IP, ...);
// printing the bare number told nobody anything. Shared by every epaperpix sketch: keep the copies identical.
#include <WiFi.h>

inline void logWiFiEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi: radio ready");
      break;
    case ARDUINO_EVENT_WIFI_OFF:
      Serial.println("WiFi: radio off");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      Serial.printf("WiFi: scan finished, %u networks found\n", (unsigned)info.wifi_scan_done.number);
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi: client mode started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("WiFi: client mode stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.printf("WiFi: joined \"%.*s\" on channel %u\n", (int)info.wifi_sta_connected.ssid_len,
                    (const char*)info.wifi_sta_connected.ssid, (unsigned)info.wifi_sta_connected.channel);
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("WiFi: got address %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WiFi: lost its address");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      // The reason tells a deliberate disconnect (8 ASSOC_LEAVE, before deep sleep) from a wrong password
      // (15 4WAY_HANDSHAKE_TIMEOUT), a missing router (201 NO_AP_FOUND) and so on.
      uint8_t reason = info.wifi_sta_disconnected.reason;
      Serial.printf("WiFi: disconnected from the router, reason %u %s%s\n", (unsigned)reason,
                    WiFi.disconnectReasonName((wifi_err_reason_t)reason),
                    reason == WIFI_REASON_ASSOC_LEAVE ? " (we left on purpose)" : "");
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      Serial.println("WiFi: the router changed its security mode");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("WiFi: setup network started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("WiFi: setup network stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("WiFi: a phone or laptop joined the setup network");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("WiFi: a phone or laptop left the setup network");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.printf("WiFi: the setup network gave %s to a phone or laptop\n",
                    IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      break;   // nearby devices probing for networks: far too frequent to log
    default:
      Serial.printf("WiFi: %s (event %d)\n", WiFi.eventName(event), (int)event);
      break;
  }
}
