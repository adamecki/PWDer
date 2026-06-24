#include "M5Cardputer.h"
#include "M5GFX.h"
#include <SD.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include "TOTP.h"
#include "Base32-Decode.h"
#include <NTPClient.h>
#include <Unit_RTC.h>
#include <time.h>
#include <cstdlib>
#include "Cipher.h"
#include "enckey.h"
#include "icons.h"

// change to compile with preferred language
#define lang_en
// #define lang_pl
#include "ui_strings.h"

M5Canvas canvas(&M5Cardputer.Display);
USBHIDKeyboard Keyboard;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP); // change to modifiable ntp server later
Unit_RTC RTC;

#define SD_CS_PIN 12
#define SD_MOSI_PIN 14
#define SD_MISO_PIN 39
#define SD_CLK_PIN 40

#define PWDER_DIR_PATH "/pwder"
#define SPKSTATE_FILE_PATH "/pwder/spkstate"
#define CONFIG_FILE_PATH "/pwder/config"
#define SECRET_FILE_PATH "/pwder/secret"
#define IMPORT_FILE_PATH "/pwimport"
#define EXPORT_FILE_PATH "/pwexport"

int wifi_timeout_seconds = 5; // Default value if config file is not readable

// variables with no value will be imported from files
String wifissid;
String wifipswd;
String hostname;
String httpport;

// totp state variables
bool rtc_available = false;
rtc_time_type rtc_time;
rtc_date_type rtc_date;

bool network_available = false;
bool totp_available = false;
char totp_buffer[7];

int device_mode = 1;
// "Device mode" determines what does drawUI() function draw and how does the device react to key presses in loop()
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
bool mode0_wasVPressed = false;

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
String mode3_tempssid = "";
String mode3_tempwpwd = "";
String mode3_tempaddr = "";
String mode3_tempport = "";
String mode3_tempdpwd = "";

int mode4_page = 0;
// 0 - connecting to wifi
// 1 - connecting to server
// 2 - downloading
// 3 - ok
// 4 - wifi error
// 5 - server error
// 6 - file error

int mode5_page = 0;
const String mode5_interactive_hyperlinks[5] = {
  "https://github.com/adamecki/PWDer",
  "https://floriano.uk",
  "https://github.com/josephpal/esp32-Encrypt",
  "https://github.com/lucadentella/TOTP-Arduino",
  "https://github.com/dirkx/Arduino-Base32-Decode"
};

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
  if (result == -1) { return -1; }
  return (long)result;
}

void generate_totp(String secret) {
  if(totp_available && (network_available || rtc_available)) {
    if(rtc_available) { // Use RTC as a primary time provider
      RTC.getDate(&rtc_date);
      RTC.getTime(&rtc_time);
    } else {
      timeClient.update();
    }
    const int decoded_len = secret.length();
    uint8_t decoded[decoded_len];
    int keyLen = base32decode(secret.c_str(), decoded, sizeof(decoded));
    TOTP totp = TOTP(decoded, keyLen);
    if(rtc_available) {
      strncpy(totp_buffer, totp.getCode(to_unix_utc_timestamp(rtc_date.Year, rtc_date.Month, rtc_date.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds)), sizeof(totp_buffer));
    } else {
      strncpy(totp_buffer, totp.getCode(timeClient.getEpochTime()), sizeof(totp_buffer));
    }
  }
}

void push_icon(const uint8_t bitmap[], int xoffset = 0, int yoffset = 0, int scale = 1) {
  // There is a scale feature I implemented, however it's not used at all.
  // Maybe somewhere in the future it might be useful somewhere, so I'm leaving it here.
  
  // canvas.fillRect(xoffset, yoffset, 32 * scale, 32 * scale, NAVY); // Uncomment if icons glitch

  for(int y = 0; y < 32; y++) {
    for(int x = 0; x < 32; x++) {
      int index = ((y * 32) + x) * 3;

      uint8_t r = bitmap[index];
      uint8_t g = bitmap[index + 1];
      uint8_t b = bitmap[index + 2];

      // xs - X scaled, ys - Y scaled
      for(int xs = 1; xs <= scale; xs++) {
        for(int ys = 1; ys <= scale; ys++) {
          canvas.drawPixel((x * scale) + xoffset + xs, (y * scale) + yoffset + ys, lgfx::v1::color565(r, g, b));
        }
      }
    }
  }
}

void save_spkstate(String state = "0") {
  // The speaker state is in another file due to frequent speaker state changes and overwriting a big file every time would be inefficient
  // it is also not encrypted
  if(SD.exists(SPKSTATE_FILE_PATH)) { SD.remove(SPKSTATE_FILE_PATH); }

  File spkstate_file = SD.open(SPKSTATE_FILE_PATH, FILE_WRITE);
  spkstate_file.println(state);
  spkstate_file.close();
}

void save_config(String input_mode = "2", String ssid = "sample", String wpwd = "password", String ipaddr = "192.168.1.100", String port = "7305", String devpwd = "default", String timeout = "5") {
  if(SD.exists(CONFIG_FILE_PATH)) { SD.remove(CONFIG_FILE_PATH); }

  String config_save_string = input_mode +
                              String('\n') + ssid +
                              String('\n') + wpwd +
                              String('\n') + ipaddr +
                              String('\n') + port +
                              String('\n') + devpwd +
                              String('\n') + timeout +
                              String('\n');
  
  File config_file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
  config_file.print(cipher->encryptString(config_save_string));
  config_file.close();
}

void export_passwords() {
  if(SD.exists(EXPORT_FILE_PATH)) { SD.remove(EXPORT_FILE_PATH); }

  String export_save_string = "";

  for(int i = 0; i < mode0_max; i++) {
    export_save_string += title[i] + String('\n');
    export_save_string += username[i] + String('\n');
    export_save_string += password[i] + String('\n');
    export_save_string += totp_secret[i];
    if(i < (mode0_max - 1)) {
      export_save_string += String('\n');
    }
  }

  File export_file = SD.open(EXPORT_FILE_PATH, FILE_WRITE);
  export_file.print(export_save_string);
  export_file.close();
}

void file_password_import() {
  File import_file = SD.open(IMPORT_FILE_PATH, FILE_READ);

  // reset
  mode0_max = 0;
  
  int swcase = 0;
  String import_string = mode1_devicepassword + String('\n');
  while(import_file.available()) {
    if(mode0_max == 100) {
      break;
    } else {
      String import_line = import_file.readStringUntil('\n');
      import_string += import_line + "\n";
      switch(swcase) {
        case 0:
          title[mode0_max] = import_line;
          swcase = 1;
          break;
        case 1:
          username[mode0_max] = import_line;
          swcase = 2;
          break;
        case 2:
          password[mode0_max] = import_line;
          swcase = 3;
          break;
        case 3:
          totp_secret[mode0_max] = import_line;
          swcase = 0;
          mode0_max++;
          break;
        default:
          break;
      }
    }
  }

  import_file.close();

  SD.remove(IMPORT_FILE_PATH);

  if(SD.exists(SECRET_FILE_PATH)) {
    SD.remove(SECRET_FILE_PATH);
  }

  File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
  secret_file.print(cipher->encryptString(import_string));
  secret_file.close();
}

void read_response(NetworkClient* client, String &lines) {
  unsigned long timeout = millis();
  while(client->available() == 0) {
    if(millis() - timeout > 5000) {
      client->stop();
      mode4_page = 5;
      drawUI();
      delay(3000);
      mode4_page = 0;
      device_mode = 0;
      drawUI();
      return;
    }
  }

  while(client->available()) {
    lines += client->readStringUntil('\r');
  }
}

bool start_rtc() {
  RTC.begin();
  delay(10);
  RTC.getDate(&rtc_date);
  if(rtc_date.Year == 2000) { // the used RTC library puts 2000 as a default year value when receiving no signal from RTC
    return false;
  } else {
    return true;
  }
}

void retry_connection() {
  if(wifi_timeout_seconds != 0) {
    WiFi.begin(wifissid, wifipswd);

    int timeout_500ms = 0;
    
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      timeout_500ms++;
      if (timeout_500ms >= (2 * wifi_timeout_seconds)) {
        break;
      }
    }

    network_available = false;

    if(timeout_500ms < (2 * wifi_timeout_seconds)) {
      network_available = true;
      timeClient.begin();  
    }
  }
}

void net_password_import() {
  if(network_available) {
    const char* host = hostname.c_str();
    const int   port = httpport.toInt();
  
    String import_string = "";
  
    mode4_page = 1;
    drawUI();
  
    NetworkClient client;
    String footer = String(" HTTP/1.1\r\n") + "Host: " + String(host) + "\r\n" + "Connection: close\r\n\r\n";
    String readRequest = "GET /" + footer;
  
    if(!client.connect(host, port)) {
      mode4_page = 5;
      drawUI();
      delay(3000);
      mode4_page = 0;
      device_mode = 0;
      drawUI();
      return;
    }
  
    mode4_page = 2;
    drawUI();
    
    client.print(readRequest);
    read_response(&client, import_string);
  
    if(SD.exists(IMPORT_FILE_PATH)) { SD.remove(IMPORT_FILE_PATH); }
  
    File import_file = SD.open(IMPORT_FILE_PATH, FILE_WRITE);
  
    int lines_read = 0;
    int start = 0;
    while(true) {
      int end = import_string.indexOf('\n', start);
      if (end == -1) {
        String line = import_string.substring(start);
        import_file.print(line + String('\n'));
        break;
      }
  
      String line = import_string.substring(start, end);
      start = end + 1;
      if(lines_read > 4) {
        import_file.print(line + String('\n'));
      }
  
      lines_read++;
    }
  
    import_file.close();
    
    if(lines_read > 7) {
      mode4_page = 3;
      drawUI();
      delay(3000);
      
      mode4_page = 0;
      device_mode = 6;
      drawUI();
    } else {
      if(SD.exists(IMPORT_FILE_PATH)) { SD.remove(IMPORT_FILE_PATH); }
      
      mode4_page = 6;
      drawUI();
      delay(3000);
  
      mode4_page = 0;
      device_mode = 0;
      drawUI();
    }
  } else {
    mode4_page = 4;
    drawUI();
    delay(3000);
    mode4_page = 0;
    device_mode = 0;
    drawUI();
    return;
  }
}

void drawUI() {
  String mode1_asterisks = ""; // password mask, also used in mode 3 (options) to hide Wi-Fi and device password
  
  // background
  canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), NAVY);
  canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, DARKGREY);
  canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, LIGHTGREY);

  switch(device_mode) {
    case 0:
      // icon
      push_icon(key, 4, 4);

      // caption
      canvas.setTextDatum(middle_left);
      canvas.setTextColor(WHITE);
      canvas.drawString(PASSWORD_MANAGER_TITLEBAR, 40, 20);

      // content
      canvas.setTextColor(BLACK);
      if(M5Cardputer.Keyboard.isKeyPressed('v')) {
        canvas.setTextDatum(top_center);
        if ((network_available || rtc_available) && totp_available) {
          generate_totp(totp_secret[mode0_selection]);
          canvas.setTextColor(BLUE);
          canvas.drawString(username[mode0_selection] + " | " + String(totp_buffer), M5Cardputer.Display.width() / 2, 50);
          canvas.setTextColor(BLACK);
          totp_buffer[6] = '\0';
        } else {
          canvas.drawString(username[mode0_selection], M5Cardputer.Display.width() / 2, 50);
        }
        canvas.setTextDatum(bottom_center);
        canvas.drawString(password[mode0_selection], M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      } else {
        canvas.setTextDatum(top_center);
        if((network_available || rtc_available) && totp_available) {
          canvas.setTextColor(BLUE);
        }
        canvas.drawString(title[mode0_selection], M5Cardputer.Display.width() / 2, 50);
        canvas.setTextColor(BLACK);
        canvas.setTextDatum(bottom_center);
        if(mode0_max == 1) {
          canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        } else {
          String enumerator;
          if(mode0_selection == 0) {
            enumerator = "[ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ] >";
          } else if (mode0_selection == mode0_max - 1) {
            enumerator = "< [ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ]";
          } else {
            enumerator = "< [ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ] >";
          }
          canvas.drawString(enumerator, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        }
      }
      break;

    case 1:
      // icon
      push_icon(padlock, 4, 4);

      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(LOGIN_TITLEBAR, 40, 20);

      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(middle_center);
      mode1_asterisks = "";
      for(int i = 0; i < mode1_passwordinput.length(); i++) {
        mode1_asterisks += "*";
      }
      canvas.drawString(mode1_asterisks + "_", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
      break;

    case 2:
      // icon
      push_icon(help, 4, 4);

      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(HANDBOOK_TITLEBAR, 40, 20);

      // content
      canvas.setTextColor(BLACK);
      switch(mode2_page) {
        case 0:
          canvas.setTextDatum(top_left);
          canvas.drawString(HANDBOOK_KEY_HELP, 8, 50);
          canvas.drawString(HANDBOOK_KEY_BACK, (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString(HANDBOOK_KEY_EXIT, 8, M5Cardputer.Display.height() - 50);
          canvas.drawString(HANDBOOK_KEY_FORWARD, (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString(HANDBOOK_KEY_USERNAME, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(HANDBOOK_KEY_PASSWORD, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          canvas.drawString(HANDBOOK_KEY_FULLINPUT, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(HANDBOOK_KEY_DEFAULTINPUT, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_left);
          canvas.drawString(HANDBOOK_KEY_TAB, 8, 50);
          canvas.drawString(HANDBOOK_KEY_ENTER, (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString(HANDBOOK_KEY_PREVIEW, 8, M5Cardputer.Display.height() - 50);
          canvas.drawString(HANDBOOK_KEY_LOCK, (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_left);
          canvas.drawString(HANDBOOK_KEY_MUTE, 8, 50);
          canvas.drawString(HANDBOOK_KEY_OPTIONS, (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString(HANDBOOK_KEY_SYNC, 8, M5Cardputer.Display.height() - 50);
          canvas.drawString(HANDBOOK_KEY_CREDITS, (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      break;

    case 3:
      // icon
      push_icon(options, 4, 4);
      
      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(OPTIONS_TITLEBAR, 40, 20);

      // content
      canvas.setTextColor(BLACK);
      switch(mode3_page) {
        case 0:
          canvas.setTextDatum(top_center);
          switch(mode0_inputtype) {
            case 0:
              canvas.drawString(OPTIONS_DEFAULT_USERNAME, M5Cardputer.Display.width() / 2, 50);
              break;
            case 1:
              canvas.drawString(OPTIONS_DEFAULT_PASSWORD, M5Cardputer.Display.width() / 2, 50);
              break;
            case 2:
              canvas.drawString(OPTIONS_DEFAULT_FULL, M5Cardputer.Display.width() / 2, 50);
              break;
            default:
              break;
          }
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 1 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString(OPTIONS_SSID + mode3_tempssid + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 2 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          mode1_asterisks = "";
          for(int i = 0; i < mode3_tempwpwd.length(); i++) {
            mode1_asterisks += "*";
          }
          canvas.drawString(OPTIONS_WIFI_PASSWORD + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 3 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_center);
          canvas.drawString(OPTIONS_SYNC_HOST + mode3_tempaddr + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 4 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_center);
          canvas.drawString(OPTIONS_SYNC_PORT + mode3_tempport + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 5 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 5:
          canvas.setTextDatum(top_center);
          mode1_asterisks = "";
          for(int i = 0; i < mode3_tempdpwd.length(); i++) {
            mode1_asterisks += "*";
          }
          canvas.drawString(OPTIONS_DEVICE_PASSWORD + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 6 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 6:
          canvas.setTextDatum(top_center);
          if(wifi_timeout_seconds == 0) {
            canvas.drawString(OPTIONS_WIFI_OFF, M5Cardputer.Display.width() / 2, 50);
          } else {
            canvas.drawString(OPTIONS_WIFI_TIMEOUT + String(wifi_timeout_seconds) + OPTIONS_WIFI_TIMEOUT_SECONDS_SHORTCUT, M5Cardputer.Display.width() / 2, 50);
          }
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 7 / 8 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 7:
          canvas.setTextDatum(top_center);
          canvas.drawString(OPTIONS_EXPORT_VAULT, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 8 / 8 ] ", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      break;

    case 4:
      // icon
      switch(mode4_page) {
        case 0:
        case 1:
        case 2:
          push_icon(frame0, 4, 4);
          break;
        case 3:
          push_icon(ok, 4, 4);
          break;
        case 4:
        case 5:
        case 6:
          push_icon(error, 4, 4);
          break;
      }
      
      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(SYNC_TITLEBAR, 40, 20);

      // content
      canvas.setTextColor(BLACK);
      switch(mode4_page) {
        case 0:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_WIFI_CONNECTING_PHASE, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 1 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_SERVER_CONNECTING_PHASE, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 2 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_DOWNLOAD_PHASE, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 3 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_OK, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(SYNC_RETURN, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(SYNC_ERR_DESCRIPTION_WIFI, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 5:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(SYNC_ERR_DESCRIPTION_SERVER, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 6:
          canvas.setTextDatum(top_center);
          canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(SYNC_ERR_DESCRIPTION_FILE, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
      }
      break;

    case 5:
      // icon
      push_icon(me, 4, 4);

      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(CREDITS_TITLEBAR, 40, 20);
  
      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(top_center);
      switch(mode5_page) {
        case 0:
          canvas.drawString(CREDITS_PWDER, M5Cardputer.Display.width() / 2, 50);
          break;
        case 1:
          canvas.drawString(CREDITS_WEBSITE, M5Cardputer.Display.width() / 2, 50);
          break;
        case 2:
          canvas.drawString(CREDITS_AES128, M5Cardputer.Display.width() / 2, 50);
          break;
        case 3:
          canvas.drawString(CREDITS_TOTP, M5Cardputer.Display.width() / 2, 50);
          break;
        case 4:
          canvas.drawString(CREDITS_BASE32, M5Cardputer.Display.width() / 2, 50);
          break;
        default:
          break;
      }
      canvas.setTextDatum(bottom_center);
      canvas.setTextColor(PURPLE);
      switch(mode5_page) {
        case 0:
          canvas.drawString(CREDITS_PWDER_GITHUB, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.drawString(CREDITS_WEBSITE_LINK, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.drawString(CREDITS_AES128_GITHUB, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.drawString(CREDITS_TOTP_GITHUB, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.drawString(CREDITS_BASE32_GITHUB, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      break;
      
    case 6:
      // icon
      push_icon(options, 4, 4);

      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString(PWIMPORT_TITLEBAR, 40, 20);
      
      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(top_center);
      canvas.drawString(PWIMPORT_REPLACE_PASSWORDS, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString("[ Y / N ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;

    case 7:
      // icon
      push_icon(search, 4, 4);

      // caption
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      if(mode7_show_results) {
        canvas.drawString(mode7_query, 40, 20);
      } else {
        canvas.drawString(SEARCH_TITLEBAR, 40, 20);
      }

      // content
      canvas.setTextColor(BLACK);
      if(mode7_show_results) {
        if(M5Cardputer.Keyboard.isKeyPressed('v') && mode7_matches > 0) {
          canvas.setTextDatum(top_center);
          if ((network_available || rtc_available) && totp_available) {
            generate_totp(totp_secret[mode7_contains_searched_string[mode7_index]]);
            canvas.setTextColor(BLUE);
            canvas.drawString(username[mode7_contains_searched_string[mode7_index]] + " | " + String(totp_buffer), M5Cardputer.Display.width() / 2, 50);
            canvas.setTextColor(BLACK);
            totp_buffer[6] = '\0';
          } else {
            canvas.drawString(username[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, 50);
          }
          canvas.setTextDatum(bottom_center);
          canvas.drawString(password[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        } else {
          if(mode7_matches > 0) {
            canvas.setTextDatum(top_center);
            if((network_available || rtc_available) && totp_available) {
              canvas.setTextColor(BLUE);
            }
            canvas.drawString(title[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, 50);
            canvas.setTextColor(BLACK);
          }
          canvas.setTextDatum(bottom_center);
          if(mode7_matches == 0) {
            canvas.setTextDatum(middle_center);
            canvas.drawString(SEARCH_NO_RESULTS, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
          } else if(mode7_matches == 1) {
            canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          } else {
            String enumerator;
            if(mode7_index == 0) {
              enumerator = "[ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ] >";
            } else if (mode7_index == mode7_matches - 1) {
              enumerator = "< [ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ]";
            } else {
              enumerator = "< [ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ] >";
            }
            canvas.drawString(enumerator, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          }
        }
      } else {
        canvas.setTextDatum(middle_center);
        canvas.drawString(mode7_query + "_", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
      }
      break;
      
    default:
      break;
  }

  // the mostly unchanging rest of the UI
  push_icon(battery, 4, M5Cardputer.Display.height() - 36);
  if(device_muted && device_mode != 1) {
    push_icon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
  } else if (device_muted == false && device_mode != 1) {
    push_icon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
  }
  if(network_available && rtc_available) {
    push_icon(clockicon, M5Cardputer.Display.width() - 72, 4);
    push_icon(network, M5Cardputer.Display.width() - 36, 4);
  } else if (network_available) {
    push_icon(network, M5Cardputer.Display.width() - 36, 4);
  } else if (rtc_available) {
    push_icon(clockicon, M5Cardputer.Display.width() - 36, 4);
  }

  canvas.setTextColor(WHITE);
  canvas.setTextDatum(middle_right);
  if(device_mode != 1) {
    if(!(device_mode == 7 && mode7_show_results == false)) {
      canvas.drawString("M ", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
    }
  }

  canvas.setTextDatum(middle_left);
  canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "%", 40, M5Cardputer.Display.height() - 20);

  canvas.pushSprite(0, 0);
}

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
  if(!SD.begin(SD_CS_PIN)) {
    canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), NAVY);
    canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, DARKGREY);
    canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, LIGHTGREY);
    
    push_icon(error, 4, 4);

    canvas.setTextDatum(middle_left);
    canvas.drawString(SDCARD_NOT_FOUND_TITLE, 40, 20);
    
    canvas.setTextColor(BLACK);
    canvas.setTextDatum(top_center);
    canvas.drawString(SDCARD_NOT_FOUND_DESCRIPTION, M5Cardputer.Display.width() / 2, 50);

    canvas.pushSprite(0, 0);

    // halt system if no sdcard found
    while(1);
  }

  // check if files exist on sdcard
  // if they don't exist - create them and add sample data
  // if they exist - import data

  // remove export file every run for safety
  if(SD.exists(EXPORT_FILE_PATH)) {
    SD.remove(EXPORT_FILE_PATH);
  }

  if(!SD.exists(PWDER_DIR_PATH)) {
    SD.mkdir(PWDER_DIR_PATH);
  }
  
  // spkstate
  if(SD.exists(SPKSTATE_FILE_PATH)) {
    String spkstate_read;
    File spkstate = SD.open(SPKSTATE_FILE_PATH, FILE_READ);
    spkstate_read = spkstate.readStringUntil('\r');
    if(spkstate_read == "1") {
      device_muted = false;
    } else {
      device_muted = true;
    }
    spkstate.close();
  } else {
    save_spkstate();
    device_muted = true;
  }

  // config
  if(SD.exists(CONFIG_FILE_PATH)) {
    File config_file = SD.open(CONFIG_FILE_PATH, FILE_READ);
    String config_read_string = cipher->decryptString(config_file.readString());
    config_file.close();
    
    String config_read[7];
    
    int start_position = 0;
    int newline_position = config_read_string.indexOf('\n', start_position);
    for(int i = 0; i < 7; i++) {
      if(newline_position != -1) {
        config_read[i] = config_read_string.substring(start_position, newline_position);
      } else { // we treat incomplete config file as an incorrect one
        if(SD.exists(SECRET_FILE_PATH)) {
          SD.remove(SECRET_FILE_PATH);
        }
        
        save_config();
    
        mode0_inputtype = 2;
        wifissid = "sample";
        wifipswd = "password";
        hostname = "192.168.1.100";
        httpport = "7305";
        mode1_devicepassword = "default";

        break;
      }

      start_position = newline_position + 1;
      newline_position = config_read_string.indexOf('\n', start_position);
    }

    // input type selection
    if(config_read[0] == "2") {
      mode0_inputtype = 2;
    } else if(config_read[1] == "1") {
      mode0_inputtype = 1;
    } else {
      mode0_inputtype = 0;
    }

    // others
    wifissid = config_read[1];
    wifipswd = config_read[2];
    hostname = config_read[3];
    httpport = config_read[4];
    mode1_devicepassword = config_read[5];

    wifi_timeout_seconds = config_read[6].toInt();
  } else {
    if(SD.exists(SECRET_FILE_PATH)) {
      SD.remove(SECRET_FILE_PATH);
    }
    
    save_config();

    mode0_inputtype = 2;
    wifissid = "sample";
    wifipswd = "password";
    hostname = "192.168.1.100";
    httpport = "7305";
    mode1_devicepassword = "default";
  }

  mode3_tempssid = wifissid;
  mode3_tempwpwd = wifipswd;
  mode3_tempaddr = hostname;
  mode3_tempport = httpport;
  mode3_tempdpwd = mode1_devicepassword;
  
  // secret
  if (SD.exists(SECRET_FILE_PATH)) {
    File secret_file = SD.open(SECRET_FILE_PATH, FILE_READ);
    String secret_read_string = cipher->decryptString(secret_file.readString());
    secret_file.close();

    int swcase = 0;

    int start_position = 0;
    int newline_position = secret_read_string.indexOf('\n', start_position);

    if (newline_position != -1) {
      String passcheck = secret_read_string.substring(start_position, newline_position);
      if (passcheck[passcheck.length() - 1] == '\r') {
        passcheck.remove(passcheck.length() - 1);
      }
      if (passcheck != mode1_devicepassword) {
        if(SD.exists(SECRET_FILE_PATH)) { SD.remove(SECRET_FILE_PATH); }
        if(SD.exists(CONFIG_FILE_PATH)) { SD.remove(CONFIG_FILE_PATH); }
        
        canvas.setTextColor(RED);
        canvas.setTextDatum(top_center);
        canvas.drawString(INTEGRITY_CHECK_FAILED_LINE1, M5Cardputer.Display.width() / 2, 9);
        canvas.drawString(INTEGRITY_CHECK_FAILED_LINE2, M5Cardputer.Display.width() / 2, 43);
        canvas.drawString(INTEGRITY_CHECK_FAILED_LINE3, M5Cardputer.Display.width() / 2, 77);
        canvas.drawString(INTEGRITY_CHECK_FAILED_LINE4, M5Cardputer.Display.width() / 2, 111);
        canvas.pushSprite(0, 0);

        while(1);
      }

      start_position = newline_position + 1;
      newline_position = secret_read_string.indexOf('\n', start_position);

      while(newline_position != -1) {
        if(mode0_max > 99) {
          break;
        } else {
          switch(swcase) {
            case 0:
              title[mode0_max] = secret_read_string.substring(start_position, newline_position);
              if(title[mode0_max][title[mode0_max].length() - 1] == '\r') {
                title[mode0_max].remove(title[mode0_max].length() - 1);
              }
              swcase = 1;
              break;
            case 1:
              username[mode0_max] = secret_read_string.substring(start_position, newline_position);
              if(username[mode0_max][username[mode0_max].length() - 1] == '\r') {
                username[mode0_max].remove(username[mode0_max].length() - 1);
              }
              swcase = 2;
              break;
            case 2:
              password[mode0_max] = secret_read_string.substring(start_position, newline_position);
              if(password[mode0_max][password[mode0_max].length() - 1] == '\r') {
                password[mode0_max].remove(password[mode0_max].length() - 1);
              }
              swcase = 3;
              break;
            case 3:
              totp_secret[mode0_max] = secret_read_string.substring(start_position, newline_position);
              if(totp_secret[mode0_max][totp_secret[mode0_max].length() - 1] == '\r') {
                totp_secret[mode0_max].remove(totp_secret[mode0_max].length() - 1);
              }
              mode0_max++;
              swcase = 0;
              break;
            default:
              break;
          }
        }
        start_position = newline_position + 1;
        newline_position = secret_read_string.indexOf('\n', start_position);
      }
    }

    if (start_position < secret_read_string.length()) {
      if(mode0_max < 100) {
        switch(swcase) {
          case 0:
            title[mode0_max] = secret_read_string.substring(start_position, newline_position);
            if(title[mode0_max][title[mode0_max].length() - 1] == '\r') {
              title[mode0_max].remove(title[mode0_max].length() - 1);
            }
            swcase = 1;
            break;
          case 1:
            username[mode0_max] = secret_read_string.substring(start_position, newline_position);
            if(username[mode0_max][username[mode0_max].length() - 1] == '\r') {
              username[mode0_max].remove(username[mode0_max].length() - 1);
            }
            swcase = 2;
            break;
          case 2:
            password[mode0_max] = secret_read_string.substring(start_position, newline_position);
            if(password[mode0_max][password[mode0_max].length() - 1] == '\r') {
              password[mode0_max].remove(password[mode0_max].length() - 1);
            }
            swcase = 3;
            break;
          case 3:
            totp_secret[mode0_max] = secret_read_string.substring(start_position, newline_position);
            if(totp_secret[mode0_max][totp_secret[mode0_max].length() - 1] == '\r') {
              totp_secret[mode0_max].remove(totp_secret[mode0_max].length() - 1);
            }
            mode0_max++;
            swcase = 0;
            break;
          default:
            break;
        }
      }
    }

    // If no passwords are present
    if(mode0_max < 1) {
      String default_secret_file = mode1_devicepassword + String('\n') + SAMPLE_ENTRY + String('\n') + SAMPLE_USERNAME + String('\n') + SAMPLE_PASSWORD + String('\n');
      File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
      secret_file.print(cipher->encryptString(default_secret_file));
      secret_file.close();
  
      title[0] = SAMPLE_ENTRY;
      username[0] = SAMPLE_USERNAME;
      password[0] = SAMPLE_PASSWORD;
  
      mode0_max = 1;
    }
  } else {
    // If a secret file does not exist
    String default_secret_file = mode1_devicepassword + String('\n') + SAMPLE_ENTRY + String('\n') + SAMPLE_USERNAME + String('\n') + SAMPLE_PASSWORD + String('\n');
    File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
    secret_file.print(cipher->encryptString(default_secret_file));
    secret_file.close();

    title[0] = SAMPLE_ENTRY;
    username[0] = SAMPLE_USERNAME;
    password[0] = SAMPLE_PASSWORD;

    mode0_max = 1;
  }

  // Start keyboard
  Keyboard.begin();
  USB.begin();

  // try getting time
  setenv("TZ", "UTCO", 1);
  tzset();

  rtc_available = start_rtc();
  if(!rtc_available) {
    // start Wi-Fi connection
    retry_connection();
  }
  
  // check for first TOTP
  if(totp_secret[mode0_selection] != "") {totp_available = true;}

  drawUI();
}

void loop() {
  // mostly keyboard input
  M5Cardputer.update();

  if(WiFi.status() == WL_CONNECTED && network_available == false) {
    network_available = true;
  } else if (WiFi.status() != WL_CONNECTED && network_available == true) {
    network_available = false;
  }

  if(M5Cardputer.Keyboard.isChange()) {
    if(mode0_wasVPressed && (device_mode == 0 || device_mode == 7)) {
      mode0_wasVPressed = false;
      drawUI();
    }
    
    if(M5Cardputer.Keyboard.isPressed()) {
      if(device_muted == false && device_mode != 1) {
        M5Cardputer.Speaker.tone(8000, 20);
      }

      // for complex typing modes
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // mode-specific actions
      switch(device_mode) {
        case 0:
          if        (M5Cardputer.Keyboard.isKeyPressed('m')) {
            device_muted = !device_muted;
            if(!device_muted){
              save_spkstate("1");
            } else {
              save_spkstate("0");
            }
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('q')) { // search (query)
            device_mode = 7;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('h')) { // help
            device_mode = 2;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('c')) { // credits
            device_mode = 5;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('s')) { // synchronize
            device_mode = 4;
            drawUI();
            net_password_import();
          } else if (M5Cardputer.Keyboard.isKeyPressed('o')) { // options
            device_mode = 3;
            drawUI();
  //      } else if (M5Cardputer.Keyboard.isKeyPressed('n')) {/ // retry connecting to Wi-Fi (future, now restart needed)
  //        retry_connection();/
          } else if (M5Cardputer.Keyboard.isKeyPressed('l')) { // lock
            device_mode = 1;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // enter data using default mode
            if(mode0_inputtype == 0 || mode0_inputtype == 2) {
              for (int i = 0; i < username[mode0_selection].length(); i++) {
                Keyboard.press(username[mode0_selection][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if(mode0_inputtype == 2) {
              Keyboard.press(KEY_TAB);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            if(mode0_inputtype == 1 || mode0_inputtype == 2) {
              for (int i = 0; i < password[mode0_selection].length(); i++) {
                Keyboard.press(password[mode0_selection][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if(mode0_inputtype == 2) {
              Keyboard.press(KEY_RETURN);
              delay(25);
              Keyboard.releaseAll();
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
            for (int i = 0; i < username[mode0_selection].length(); i++) {
              Keyboard.press(username[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
            for (int i = 0; i < password[mode0_selection].length(); i++) {
              Keyboard.press(password[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
            for (int i = 0; i < username[mode0_selection].length(); i++) {
              Keyboard.press(username[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
            delay(25);

            for (int i = 0; i < password[mode0_selection].length(); i++) {
              Keyboard.press(password[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            Keyboard.press(KEY_RETURN);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
            if((network_available || rtc_available) && totp_available) {
              generate_totp(totp_secret[mode0_selection]);
              for(int i = 0; i < 6; i++) {
                Keyboard.press(totp_buffer[i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);  
              }
              totp_buffer[6] = '\0';
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('t')) { // press TAB on a computer
            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('r')) { // press RETURN on a computer
            Keyboard.press(KEY_RETURN);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode0_selection < mode0_max - 1) { // next password
            mode0_selection++;
            if(totp_secret[mode0_selection] != "") {totp_available = true;} else {totp_available = false;}
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode0_selection > 0) { // previous password
            mode0_selection--;
            if(totp_secret[mode0_selection] != "") {totp_available = true;} else {totp_available = false;}
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('v')) {
            drawUI();
            mode0_wasVPressed = true;
          }
          break;
          
        case 1:
          if ((mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported) && M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
            device_mode = 3;
            mode1_ispasswordbeingchanged = false;
            mode1_ispasswordbeingexported = false;
            mode1_passwordinput = "";
            drawUI();
            push_icon(error, 4, 4);
            canvas.pushSprite(0, 0);
          } else {
            for(auto i : status.word) {
              mode1_passwordinput += i;
              drawUI();
            }
  
            if(status.del) {
              mode1_passwordinput.remove(mode1_passwordinput.length() - 1);
              drawUI();
            }
  
            if(status.enter) {
              if(mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported) {
                if(mode1_passwordinput == mode1_devicepassword) {
                  device_mode = 3;
                  if(mode1_ispasswordbeingchanged) {
                    mode1_devicepassword = mode3_tempdpwd;
                    save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword, String(wifi_timeout_seconds));

                    // Update vault with new password
                    File old_vault = SD.open(SECRET_FILE_PATH, FILE_READ);
                    String old_vault_str = cipher->decryptString(old_vault.readString());
                    old_vault.close();

                    if(SD.exists(SECRET_FILE_PATH)) { SD.remove(SECRET_FILE_PATH); }

                    File new_vault = SD.open(SECRET_FILE_PATH, FILE_WRITE);
                    String new_vault_str = mode1_devicepassword + old_vault_str.substring(old_vault_str.indexOf('\n', 0)); // replace first line
                    new_vault.print(cipher->encryptString(new_vault_str));
                    new_vault.close();
                  } else if(mode1_ispasswordbeingexported) {
                    export_passwords();
                  }
                  drawUI();
                  push_icon(ok, 4, 4);
                } else {
                  device_mode = 3;
                  drawUI();
                  push_icon(error, 4, 4);
                }

                canvas.pushSprite(0, 0);

                mode1_passwordinput = "";
                mode1_ispasswordbeingchanged = false;
                mode1_ispasswordbeingexported = false;
              } else {
                if(mode1_passwordinput == mode1_devicepassword) {
                    // after unlocking
                    // check if there are passwords to import
                    if(SD.exists(IMPORT_FILE_PATH)) {
                      device_mode = 6;
                    } else {
                      device_mode = 0;
                    }
                  mode1_passwordinput = "";
                  drawUI();
                } else {
                  mode1_passwordinput = "";
                  drawUI();
                }
              }
            }
          }
          break;
          
        case 2:
          if(M5Cardputer.Keyboard.isKeyPressed('`')) {
            device_mode = 0;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode2_page > 0) {
            mode2_page--;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode2_page < 4) {
            mode2_page++;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('m')) {
            device_muted = !device_muted;
            if(!device_muted){
              save_spkstate("1");
            } else {
              save_spkstate("0");
            }
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('l')) {
            device_mode = 1;
            drawUI();
          }
          break;
          
        case 3:
          if(M5Cardputer.Keyboard.isKeyPressed('`') && (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) || mode3_page == 0)) {
            mode3_tempssid = wifissid;
            mode3_tempwpwd = wifipswd;
            mode3_tempaddr = hostname;
            mode3_tempport = httpport;
            mode3_tempdpwd = mode1_devicepassword;
            
            device_mode = 0;
            drawUI();
          } else if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed(',') && mode3_page > 0) {
            mode3_page--;
            drawUI();  
          } else if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('/') && mode3_page < 7) {
            mode3_page++;
            drawUI();
          } else {
            for(auto i : status.word) {
              switch(mode3_page) {
                case 1:
                  mode3_tempssid += i;
                  break;
                case 2:
                  mode3_tempwpwd += i;
                  break;
                case 3:
                  mode3_tempaddr += i;
                  break;
                case 4:
                  mode3_tempport += i;
                  break;
                case 5:
                  mode3_tempdpwd += i;
                  break;
                default:
                  break;
              }
              drawUI();
            }
  
            if(status.del) {
              switch(mode3_page) {
                case 1:
                  mode3_tempssid.remove(mode3_tempssid.length() - 1);
                  break;
                case 2:
                  mode3_tempwpwd.remove(mode3_tempwpwd.length() - 1);
                  break;
                case 3:
                  mode3_tempaddr.remove(mode3_tempaddr.length() - 1);
                  break;
                case 4:
                  mode3_tempport.remove(mode3_tempport.length() - 1);
                  break;
                case 5:
                  mode3_tempdpwd.remove(mode3_tempdpwd.length() - 1);
                  break;
                default:
                  break;
              }
              drawUI();
            }
  
            if(status.enter) {
              switch(mode3_page) {
                case 0:
                  switch(mode0_inputtype) {
                    case 0:
                      mode0_inputtype = 1;
                      break;
                    case 1:
                      mode0_inputtype = 2;
                      break;
                    case 2:
                      mode0_inputtype = 0;
                      break;
                  }
                  drawUI();
                  break;
                case 1:
                  wifissid = mode3_tempssid;
                  break;
                case 2:
                  wifipswd = mode3_tempwpwd;
                  break;
                case 3:
                  hostname = mode3_tempaddr;
                  break;
                case 4:
                  httpport = mode3_tempport;
                  break;
                case 5:
                  mode1_ispasswordbeingchanged = true;
                  device_mode = 1;
                  drawUI();
                  break;
                case 6:
                  if(wifi_timeout_seconds < 15) {
                    wifi_timeout_seconds++;
                  } else {
                    wifi_timeout_seconds = 0;
                  }
                  break;
                case 7:
                  mode1_ispasswordbeingexported = true;
                  device_mode = 1;
                  drawUI();
                  break;
                default:
                  break;
              }
              if(!(mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported)) {
                save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword, String(wifi_timeout_seconds));
                drawUI();
                push_icon(ok, 4, 4);
                canvas.pushSprite(0, 0);
              }
            }
          }
          break;
          
        case 5:
          if(M5Cardputer.Keyboard.isKeyPressed('m')) {
            device_muted = !device_muted;
            if(!device_muted){
              save_spkstate("1");
            } else {
              save_spkstate("0");
            }
            drawUI();
          } else if(M5Cardputer.Keyboard.isKeyPressed('`')) {
            device_mode = 0;
            drawUI(); 
          } else if(M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            for (int i = 0; i < mode5_interactive_hyperlinks[mode5_page].length(); i++) {
              Keyboard.press(mode5_interactive_hyperlinks[mode5_page][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);  
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode5_page < 4) { // next page
            mode5_page++;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode5_page > 0) { // previous page
            mode5_page--;
            drawUI();
          }
          break;
          
        case 6:
          if(M5Cardputer.Keyboard.isKeyPressed('m')) {
            device_muted = !device_muted;
            if(!device_muted){
              save_spkstate("1");
            } else {
              save_spkstate("0");
            }
            drawUI();
          } else if(M5Cardputer.Keyboard.isKeyPressed('y')) {
            file_password_import();
            device_mode = 0;
            drawUI();
          } else if(M5Cardputer.Keyboard.isKeyPressed('n')) {
            SD.remove(IMPORT_FILE_PATH);
            device_mode = 0;
            drawUI();
          }
          break;

        case 7:
          if(mode7_show_results) {
            if (M5Cardputer.Keyboard.isKeyPressed('/') && mode7_index < mode7_matches - 1) { // next password
              mode7_index++;
              if(totp_secret[mode7_contains_searched_string[mode7_index]] != "") {totp_available = true;} else {totp_available = false;}
              drawUI();
            } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode7_index > 0) { // previous password
              mode7_index--;
              if(totp_secret[mode7_contains_searched_string[mode7_index]] != "") {totp_available = true;} else {totp_available = false;}
              drawUI();
            } else if(M5Cardputer.Keyboard.isKeyPressed('`')) {
              mode7_matches = 0;
              mode7_index = 0;
              mode7_show_results = false;
              drawUI();
            } else if(M5Cardputer.Keyboard.isKeyPressed('l')) {
              device_mode = 1;
              drawUI();
            } else if(M5Cardputer.Keyboard.isKeyPressed('m')) {
              device_muted = !device_muted;
              if(!device_muted){
                save_spkstate("1");
              } else {
                save_spkstate("0");
              }
              drawUI();
            } else if(M5Cardputer.Keyboard.isKeyPressed('t')) { // press TAB on a computer
              Keyboard.press(KEY_TAB);
              delay(25);
              Keyboard.releaseAll();
            } else if(M5Cardputer.Keyboard.isKeyPressed('r')) { // press RETURN on a computer
              Keyboard.press(KEY_RETURN);
              delay(25);
              Keyboard.releaseAll();
            } else if(M5Cardputer.Keyboard.isKeyPressed('v')) {
              drawUI();
              mode0_wasVPressed = true;
            } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // enter data using default mode
              if(mode0_inputtype == 0 || mode0_inputtype == 2) {
                for (int i = 0; i < username[mode7_contains_searched_string[mode7_index]].length(); i++) {
                  Keyboard.press(username[mode7_contains_searched_string[mode7_index]][i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);
                }
              }
  
              if(mode0_inputtype == 2) {
                Keyboard.press(KEY_TAB);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
  
              if(mode0_inputtype == 1 || mode0_inputtype == 2) {
                for (int i = 0; i < password[mode7_contains_searched_string[mode7_index]].length(); i++) {
                  Keyboard.press(password[mode7_contains_searched_string[mode7_index]][i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);
                }
              }
  
              if(mode0_inputtype == 2) {
                Keyboard.press(KEY_RETURN);
                delay(25);
                Keyboard.releaseAll();
              }
            } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
              for (int i = 0; i < username[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(username[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
              for (int i = 0; i < password[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(password[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
              for (int i = 0; i < username[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(username[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
  
              Keyboard.press(KEY_TAB);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
  
              for (int i = 0; i < password[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(password[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
  
              Keyboard.press(KEY_RETURN);
              delay(25);
              Keyboard.releaseAll();
            } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
              if((network_available || rtc_available) && totp_available) {
                generate_totp(totp_secret[mode7_contains_searched_string[mode7_index]]);
                for(int i = 0; i < 6; i++) {
                  Keyboard.press(totp_buffer[i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);  
                }
                totp_buffer[6] = '\0';
              }
            }
          } else {
            if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
              device_mode = 0;
              if(totp_secret[mode0_selection] != "") {totp_available = true;} else {totp_available = false;}
              drawUI();
            } else {
              for(auto i : status.word) {
                mode7_query += i;
                drawUI();
              }
    
              if(status.del) {
                mode7_query.remove(mode7_query.length() - 1);
                drawUI();
              }
    
              if(status.enter) {
                if(mode7_query != "") {
                  String lowercase_query = mode7_query;
                  lowercase_query.toLowerCase();
                    
                  for(int i = 0; i < mode0_max; i++) {
                    String lowercase_title = title[i];
                    lowercase_title.toLowerCase();

                    if(lowercase_title.indexOf(lowercase_query) != -1) {
                      mode7_contains_searched_string[mode7_matches] = i;
                      mode7_matches++;
                    }
                  }
                  
                  mode7_show_results = true;
                  if(totp_secret[mode7_contains_searched_string[mode7_index]] != "") {totp_available = true;} else {totp_available = false;}
                  drawUI();
                }
              }
            }
          }
          break;
          
        default:
          break;
      }
    }
  }
}
