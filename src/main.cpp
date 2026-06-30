#include "globals.h"

#include "file_operations.h"
#include "gui.h"
#include "keyboard_handler.h"
#include "network_operations.h"
#include "time_operations.h"

M5Canvas canvas(&M5Cardputer.Display);
USBHIDKeyboard Keyboard;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Unit_RTC RTC;
SPIClass sdcardSPI;

pvault::vault entries{};
pvault::device_settings configuration{};
uint8_t aes_key[pvault::key_size];

bool network_available = false;
bool totp_available = false;
char totp_buffer[7];
bool rtc_available = false;
rtc_time_type rtc_time;
rtc_date_type rtc_date;

int8_t last_battery_percentage;
unsigned long last_ui_refresh;

int device_mode = 1;
// "Device mode" determines what does draw_ui() function draw and how does the device react to key presses in loop()
// 0 - main page
// 1 - device locked
// 2 - help page
// 3 - options page
// 4 - sync page
// 5 - credits page
// 6 - file import page
// 7 - search

int mode0_selection = 1;
bool mode0_preview = false;

String mode1_passwordinput = "";
bool mode1_ispasswordbeingchanged = false;

int mode2_page = 0;

int mode3_page = 0;
// 0 - default password input mode
// 1 - ssid
// 2 - wifi pwd
// 3 - ip
// 4 - port
// 5 - devpwd
// 6 - wifi timeout
// * - ntp server
// 7 - ntp time sync
// * - manual rtc clock configuration
// 8 - export vault
String mode3_tempssid = "";
String mode3_tempwpwd = "";
String mode3_tempaddr = "";
String mode3_tempport = "";
String mode3_tempdpwd = "";

int mode4_page = 0;

int mode5_page = 0;

String mode7_query = "";
int mode7_matchindex[100];
int mode7_index = 0;
int mode7_matches = 0;
bool mode7_show_results = false;

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5Cardputer.Display.setRotation(1);
  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setTextFont(&fonts::Font2);
  canvas.setTextColor(WHITE);
  canvas.setTextSize(1);

  // start sdcard
  sdcardSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdcardSPI)) {
    no_sdcard_crash_screen();

    // halt system if no sdcard found
    while (1);
  }

  // read config or write a sample
  if(!SD.exists(VAULT_PATH)) {
    init_new_vault();
  } else {
    pvault::read_config(VAULT_PATH, configuration);
  }

  if(SD.exists(EXPORT_FILE_PATH)) {
    SD.remove(EXPORT_FILE_PATH);
  }

  mode3_tempssid = entries.credentials[0].title;
  mode3_tempwpwd = entries.credentials[0].username;
  mode3_tempaddr = entries.credentials[0].password;
  mode3_tempport = entries.credentials[0].totp_secret;

  // start keyboard
  Keyboard.begin();
  USB.begin();

  // try getting time
  setenv("TZ", "UTCO", 1);
  tzset();

  rtc_available = start_rtc();
  if (!rtc_available) {
    // start Wi-Fi connection
    retry_connection(false);
  }

  last_battery_percentage = M5.Power.getBatteryLevel();
  last_ui_refresh = millis();
  draw_ui();
}

void loop() {
  M5Cardputer.update();

  int8_t current_battery_percentage = M5.Power.getBatteryLevel();
  if ((current_battery_percentage != last_battery_percentage) && millis() - last_ui_refresh > UI_UPDATE_MILISECONDS) {
    last_battery_percentage = current_battery_percentage;
    last_ui_refresh = millis();
    draw_ui();
  }

  if (WiFi.status() == WL_CONNECTED && network_available == false) {
    network_available = true;
  } else if (WiFi.status() != WL_CONNECTED && network_available == true) {
    network_available = false;
  }

  check_keyboard_events();
}
