#include "globals.h"

#include "enckey.h"
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

int wifi_timeout_seconds;

String wifissid;
String wifipswd;
String hostname;
String httpport;

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
bool device_muted;

int mode0_inputtype;
int mode0_selection = 0;
int mode0_max = 0;
bool mode0_preview = false;

String mode1_devicepassword;
String mode1_passwordinput = "";
bool mode1_ispasswordbeingchanged = false;
bool mode1_ispasswordbeingexported = false;

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
int mode7_contains_searched_string[100];
int mode7_index = 0;
int mode7_matches = 0;
bool mode7_show_results = false;

String title[100];
String username[100];
String password[100];
String totp_secret[100];

Cipher* cipher = new Cipher();

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  M5Cardputer.Display.setRotation(1);
  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setTextFont(&fonts::Font2);
  canvas.setTextColor(WHITE);
  canvas.setTextSize(1);

  // set up encryption
  cipher->setKey(enckey);

  // start sdcard
  sdcardSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdcardSPI)) {
    no_sdcard_crash_screen();

    // halt system if no sdcard found
    while (1);
  }

  // check if files exist on sdcard
  // if they don't exist - create them and add sample data
  // if they exist - import data

  // remove export file every run for safety
  if (SD.exists(EXPORT_FILE_PATH)) {
    SD.remove(EXPORT_FILE_PATH);
  }

  if (!SD.exists(PWDER_DIR_PATH)) {
    SD.mkdir(PWDER_DIR_PATH);
  }

  // spkstate
  if (SD.exists(SPKSTATE_FILE_PATH)) {
    read_spkstate();
  } else {
    save_spkstate();
    device_muted = true;
  }

  // config
  if (SD.exists(CONFIG_FILE_PATH)) {
    read_and_verify_config();
  } else {
    if (SD.exists(SECRET_FILE_PATH)) {
      SD.remove(SECRET_FILE_PATH);
    }
    init_sample_config();
  }

  mode3_tempssid = wifissid;
  mode3_tempwpwd = wifipswd;
  mode3_tempaddr = hostname;
  mode3_tempport = httpport;
  mode3_tempdpwd = mode1_devicepassword;

  // secret
  if (SD.exists(SECRET_FILE_PATH)) {
    read_and_verify_secret();
  } else {
    init_sample_secret();
  }

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

  // check for first TOTP
  if (totp_secret[mode0_selection] != "") {
    totp_available = true;
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
