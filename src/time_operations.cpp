#include "globals.h"

extern bool totp_available;
extern bool network_available;
extern bool rtc_available;
extern Unit_RTC RTC;
extern rtc_time_type rtc_time;
extern rtc_date_type rtc_date;
extern NTPClient timeClient;
extern char totp_buffer[7];

unsigned long to_unix_utc_timestamp(int year, int month, int day, int hour, int minute, int second) {
  struct tm t;

  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = 0;

  time_t result = mktime(&t);
  if (result == -1) {
    return -1;
  }
  return (long)result;
}

struct tm to_regular_utc_timestamp(unsigned long epoch) {
  time_t timestamp = static_cast<time_t>(epoch);
  struct tm timeinfo;
  gmtime_r(&timestamp, &timeinfo);
  return timeinfo;
}

void generate_totp(String secret) {
  if (totp_available && (network_available || rtc_available)) {
    if (rtc_available) { // Use RTC as a primary time provider
      RTC.getDate(&rtc_date);
      RTC.getTime(&rtc_time);
    } else {
      timeClient.update();
    }
    const int decoded_len = secret.length();
    uint8_t decoded[decoded_len];
    int keyLen = base32decode(secret.c_str(), decoded, sizeof(decoded));
    TOTP totp = TOTP(decoded, keyLen);
    if (rtc_available) {
      strncpy(totp_buffer,
              totp.getCode(to_unix_utc_timestamp(rtc_date.Year, rtc_date.Month, rtc_date.Date, rtc_time.Hours,
                                                 rtc_time.Minutes, rtc_time.Seconds)),
              sizeof(totp_buffer));
    } else {
      strncpy(totp_buffer, totp.getCode(timeClient.getEpochTime()), sizeof(totp_buffer));
    }
  }
}

bool start_rtc() {
  RTC.begin();
  delay(10);
  RTC.getDate(&rtc_date);
  if (rtc_date.Year == 2000) {
    return false;
  } else {
    return true;
  }
}
