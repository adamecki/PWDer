unsigned long to_unix_utc_timestamp(int year, int month, int day, int hour, int minute, int second);
struct tm to_regular_utc_timestamp(unsigned long epoch);
void generate_totp(String secret);
bool start_rtc();