#pragma once
// The clock is only nice to have: HTTPS on this core does not check certificate dates (mbedTLS is built
// without MBEDTLS_HAVE_TIME_DATE), and nothing else reads it. The RTC keeps the time through deep sleep and
// every API reply's Date header refreshes it, so NTP is asked only when the clock was never set (after a
// flash or a power cycle). A network that blocks NTP no longer fails the wake: on 2026-10-06 a 4G router
// (HH72V1) answered no NTP request, and every wake backed off 60, 120, 240 s before reaching the API.
// Shared by every epaperpix sketch: keep the copies identical.
#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#define CLOCK_VALID_AFTER 1704067200L  // 2024-01-01 UTC: an earlier clock was never set
#define NTP_WAIT_MS 5000UL

inline bool clockIsSet() { return time(nullptr) > CLOCK_VALID_AFTER; }

inline void printClock() {
  time_t now = time(nullptr);
  struct tm utc;
  gmtime_r(&now, &utc);
  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d UTC\n", utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                utc.tm_hour, utc.tm_min, utc.tm_sec);
}

// Days since 1970-01-01 for a civil date (Howard Hinnant's days_from_civil).
inline int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

// Set the clock from an HTTP Date header ("Tue, 06 Oct 2026 06:44:16 GMT"). Returns false if it does not parse.
inline bool setClockFromHttpDate(const String& value) {
  int day, year, hh, mm, ss;
  char mon[4] = {0};
  if (sscanf(value.c_str(), "%*3s, %d %3s %d %d:%d:%d", &day, mon, &year, &hh, &mm, &ss) != 6) return false;
  static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char* at = strstr(months, mon);
  if (at == nullptr || (at - months) % 3 != 0) return false;
  int64_t secs = daysFromCivil(year, (unsigned)((at - months) / 3 + 1), (unsigned)day) * 86400 + hh * 3600 + mm * 60 + ss;
  if (secs <= CLOCK_VALID_AFTER) return false;
  struct timeval tv;
  tv.tv_sec = (time_t)secs;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  return true;
}

// Ask NTP only when the clock was never set, for at most NTP_WAIT_MS, and never fail the wake over it.
inline void syncClock() {
  Serial.print("Clock: ");
  if (clockIsSet()) {
    printClock();
    return;
  }
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  Serial.print("not set, asking NTP ");
  unsigned long started = millis();
  while (!clockIsSet() && millis() - started < NTP_WAIT_MS) {
    delay(250);
    Serial.print(".");
  }
  if (clockIsSet()) {
    Serial.print(" ");
    printClock();
  } else {
    Serial.println(" no answer (the network may block NTP); carrying on, the API reply will set it");
  }
}
