#include "globals.h"

#include "file_operations.h"
#include "gui.h"
#include "sfx.h"
#include "keyboard_events.h"
#include "network_operations.h"
#include "keyboard_bridge.h"
#include "time_operations.h"
#include "pwrotocol.h"

#define SCREEN_DIM_MS 15000
#define SCREEN_OFF_MS 20000
#define SCREEN_LCK_MS 30000
#define BASIC_BRIGHTNESS 0x80
#define DIMMED_BRIGHTNESS 0x40

M5Canvas canvas(&M5Cardputer.Display);
USBCDC USBSerialDevice;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
Unit_RTC RTC;
SPIClass sdcardSPI;

pvault::vault entries{};
pvault::device_settings configuration{};
uint8_t aes_key[pvault::key_size];

bool network_available = false;
bool network_initialized = false;
bool bluetooth_initialized = false;
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
// 4 - sync page (unused for now)
// 5 - credits page
// 6 - file import page (deprecated)
// 7 - search

unsigned long last_action = millis();
int auto_lock = 0;
// 0 - normal work
// 1 - screen dimmed
// 2 - screen off

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
uint8_t mode3_tempbrightness = BASIC_BRIGHTNESS;

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

  uint8_t brightness = 0x00;
  M5Cardputer.Display.setBrightness(brightness);

  M5Cardputer.Display.setRotation(1);

  canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
  canvas.setFont(&fonts::Font2);
  canvas.setTextColor(WHITE);
  canvas.setTextSize(1);

  splash_screen();

  while(brightness < BASIC_BRIGHTNESS) {
    brightness += 1;
    M5Cardputer.Display.setBrightness(brightness);
    delay(2);
  }
  
  splash_screen_create_progressbar();

  // start sdcard
  sdcardSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, sdcardSPI)) {
    no_sdcard_crash_screen();

    // halt system if no sdcard found
    while (1);
  }

  splash_screen_update_progressbar_percentage(25);

  // read config or write a sample
  if(!SD.exists(VAULT_PATH)) {
    init_new_vault();
  } else {
    pvault::read_config(VAULT_PATH, configuration);
  }
  M5Cardputer.Display.setBrightness(configuration.brightness);
  mode3_tempbrightness = configuration.brightness;

  if(SD.exists(EXPORT_FILE_PATH)) {
    SD.remove(EXPORT_FILE_PATH);
  }

  // update splash screen with new theme when it's loaded
  splash_screen();

  M5Cardputer.update();
  if(M5Cardputer.BtnA.isPressed() && M5Cardputer.Keyboard.isKeyPressed(KEY_FN)) {
    save_screenshot_bmp();
    sfx::melody_c_major_scale();
  }

  splash_screen_create_progressbar();
  splash_screen_update_progressbar_percentage(50);

  // mode3_tempssid = entries.credentials[0].title;
  // mode3_tempwpwd = entries.credentials[0].username;
  // mode3_tempaddr = entries.credentials[0].password;
  // mode3_tempport = entries.credentials[0].totp_secret;

  // start keyboard and serial port
  USB.begin();
  USBSerialDevice.begin(115200);
  usb_keyboard_init(); // USB by default, ble attempts to connect only when B is pressed

  delay(1000);

  splash_screen_update_progressbar_percentage(75);

  // try getting time
  setenv("TZ", "UTCO", 1);
  tzset();

  rtc_available = start_rtc();

  last_battery_percentage = M5.Power.getBatteryLevel();
  last_ui_refresh = millis();

  splash_screen_update_progressbar_percentage(100);
  delay(250);

  draw_ui();
}

void loop() {
  M5Cardputer.update();

  int8_t current_battery_percentage = M5.Power.getBatteryLevel();

  if ((current_battery_percentage != last_battery_percentage) && millis() - last_ui_refresh > UI_UPDATE_MILISECONDS && auto_lock < 2) {
    last_battery_percentage = current_battery_percentage;
    ble_keyboard_update_battery_level();
    last_ui_refresh = millis();
    draw_ui();
  }

  if (WiFi.status() == WL_CONNECTED && network_available == false) {
    network_available = true;
  } else if (WiFi.status() != WL_CONNECTED && network_available == true) {
    network_available = false;
  }

  // check for serial commands
  pwrotocol_listen_and_respond();

  check_keyboard_events();

  if(configuration.speaker_on_auto_lock == 2 || configuration.speaker_on_auto_lock == 3) {
    if((millis() - last_action >= SCREEN_DIM_MS) && auto_lock == 0) {
      auto_lock = 1;
      if(M5Cardputer.Display.getBrightness() > DIMMED_BRIGHTNESS) {
        M5Cardputer.Display.setBrightness(DIMMED_BRIGHTNESS);
      }
    } else if((millis() - last_action >= SCREEN_OFF_MS) && auto_lock == 1) {
      auto_lock = 2;
      M5Cardputer.Display.setBrightness(0x00);
    } else if((millis() - last_action >= SCREEN_LCK_MS) && auto_lock == 2) {
      auto_lock = 3;
      device_mode = 1;
      draw_ui();
    }
  }
}
