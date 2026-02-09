#include "M5Cardputer.h"
#include "M5GFX.h"
#include <SD.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include "Cipher.h"
#include "enckey.h"
#include "icons.h"

M5Canvas canvas(&M5Cardputer.Display);
USBHIDKeyboard Keyboard;

#define SD_CS_PIN 12
#define SD_MOSI_PIN 14
#define SD_MISO_PIN 39
#define SD_CLK_PIN 40

#define PWDER_DIR_PATH "/pwder"
#define SPKSTATE_FILE_PATH "/pwder/spkstate"
#define CONFIG_FILE_PATH "/pwder/config"
#define SECRET_FILE_PATH "/pwder/secret"
#define IMPORT_FILE_PATH "/pwimport"

// variables with no value will be imported from files
String wifissid;
String wifipswd;
String hostname;
String httpport;

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
const String mode5_interactive_hyperlink = "https://github.com/adamecki/PWDer";
const String mode5_interactive_hyperlink2 = "https://floriano.uk";
const String mode5_interactive_hyperlink3 = "https://github.com/josephpal/esp32-Encrypt";

String mode7_query = "";
int mode7_contains_searched_string[100];
int mode7_index = 0;
int mode7_matches = 0;
bool mode7_show_results = false;

String title[100];
String username[100];
String password[100];

Cipher* cipher = new Cipher();

void pushIcon(const uint8_t bitmap[], int xoffset = 0, int yoffset = 0, int scale = 1) {
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

void save_config(String input_mode = "2", String ssid = "sample", String wpwd = "password", String ipaddr = "192.168.1.100", String port = "2137", String devpwd = "default") {
  if(SD.exists(CONFIG_FILE_PATH)) { SD.remove(CONFIG_FILE_PATH); }

  String long_config_string = input_mode +
                              String('\n') + ssid +
                              String('\n') + wpwd +
                              String('\n') + ipaddr +
                              String('\n') + port +
                              String('\n') + devpwd +
                              String('\n');
  
  File config_file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
  config_file.print(cipher->encryptString(long_config_string));
  config_file.close();
}

void importPasswordsFromFile() {
  File import_file = SD.open(IMPORT_FILE_PATH, FILE_READ);

  // reset
  mode0_max = 0;
  
  int swcase = 0;
  String import_string = "";
  while(import_file.available()) {
    if(mode0_max == 100) {
      break;
    } else {
      String import_line = import_file.readStringUntil('\n');
      // import_line.remove(import_line.length() - 1); // this is in case \r\n is used (but it looks like it isn't)
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

void readResponse(NetworkClient* client, String &lines) {
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

void importPasswordsFromNetwork() {
  const char* ssid = wifissid.c_str();
  const char* wpwd = wifipswd.c_str();
  const char* host = hostname.c_str();
  const int   port = httpport.toInt();

  String import_string = "";

  WiFi.begin(ssid, wpwd);
  int wifi_connection_timeout = 0; // max - 20 (10s timeout)
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    wifi_connection_timeout++;
    if(wifi_connection_timeout > 19) {
      mode4_page = 4;
      drawUI();
      delay(3000);
      mode4_page = 0;
      device_mode = 0;
      drawUI();
      return;
    }
  }

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
  readResponse(&client, import_string);

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
}

void drawUI() {
  String mode1_asterisks = ""; // password mask, also used in mode 3 (options) to hide Wi-Fi and device password
  
  // background
  canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), NAVY);
  canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, DARKGREY);
  canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, LIGHTGREY);

  switch(device_mode) {
    case 0:
      // icons
      pushIcon(key, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      pushIcon(help, M5Cardputer.Display.width() - 36, 4);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_right);
      canvas.drawString("H ->", M5Cardputer.Display.width() - 40, 20);
      canvas.drawString("M ->", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Passwords", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);

      // content
      canvas.setTextColor(BLACK);
      if(M5Cardputer.Keyboard.isKeyPressed('v')) {
        canvas.setTextDatum(top_center);
        canvas.drawString(username[mode0_selection], M5Cardputer.Display.width() / 2, 50);
        canvas.setTextDatum(bottom_center);
        canvas.drawString(password[mode0_selection], M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      } else {
        canvas.setTextDatum(top_center);
        canvas.drawString(title[mode0_selection], M5Cardputer.Display.width() / 2, 50);
        canvas.setTextDatum(bottom_center);
        if(mode0_max == 1) {
          canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        } else {
          if(mode0_selection == 0) {
            canvas.drawString("[ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          } else if (mode0_selection == mode0_max - 1) {
            canvas.drawString("< [ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          } else {
            canvas.drawString("< [ " + String(mode0_selection + 1) + " / " + String(mode0_max) + " ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          }
        }
      }
      
      // push
      canvas.pushSprite(0, 0);
      break;

    case 1:
      // icons
      pushIcon(padlock, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Enter password", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);
      
      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(middle_center);
      mode1_asterisks = "";
      for(int i = 0; i < mode1_passwordinput.length(); i++) {
        mode1_asterisks += "*";
      }
      canvas.drawString(mode1_asterisks + "_", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
      
      canvas.pushSprite(0, 0);
      break;

    case 2:
      // icons
      pushIcon(help, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_right);
      canvas.drawString("M ->", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Help", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);
      
      // content
      canvas.setTextColor(BLACK);
      switch(mode2_page) {
        case 0:
          canvas.setTextDatum(top_left);
          canvas.drawString("H - help", 8, 50);
          canvas.drawString("< (,) - back", (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString("Esc (') - exit", 8, M5Cardputer.Display.height() - 50);
          canvas.drawString("> (/) - forward", (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString("1 - username input", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("2 - password input", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          canvas.drawString("3 - full input", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("OK - default input", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_left);
          canvas.drawString("T - hit TAB", 8, 50);
          canvas.drawString("R - hit ENTER", (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString("V - preview", 8, M5Cardputer.Display.height() - 50);
          canvas.drawString("L - lock", (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_left);
          canvas.drawString("M - mute", 8, 50);
          canvas.drawString("O - options", (M5Cardputer.Display.width() / 2) + 8, 50);
          canvas.setTextDatum(bottom_left);
          canvas.drawString("S - sync", 8, M5Cardputer.Display.height() - 50);
          canvas.drawString("C - credits", (M5Cardputer.Display.width() / 2) + 8, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      
      canvas.pushSprite(0, 0);
      break;

    case 3:
      // icons
      pushIcon(options, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }
      
      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Options", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);
      
      // content
      canvas.setTextColor(BLACK);
      switch(mode3_page) {
        case 0:
          canvas.setTextDatum(top_center);
          switch(mode0_inputtype) {
            case 0:
              canvas.drawString("Default input mode: username", M5Cardputer.Display.width() / 2, 50);
              break;
            case 1:
              canvas.drawString("Default input mode: password", M5Cardputer.Display.width() / 2, 50);
              break;
            case 2:
              canvas.drawString("Default input mode: full", M5Cardputer.Display.width() / 2, 50);
              break;
            default:
              break;
          }
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 1 / 6 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString("WiFi SSID: " + mode3_tempssid + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 2 / 6 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          mode1_asterisks = "";
          for(int i = 0; i < mode3_tempwpwd.length(); i++) {
            mode1_asterisks += "*";
          }
          canvas.drawString("WiFi Key: " + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 3 / 6 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync host: " + mode3_tempaddr + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 4 / 6 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync port: " + mode3_tempport + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 5 / 6 ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 5:
          canvas.setTextDatum(top_center);
          mode1_asterisks = "";
          for(int i = 0; i < mode3_tempdpwd.length(); i++) {
            mode1_asterisks += "*";
          }
          canvas.drawString("Password: " + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("< [ 6 / 6 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      
      canvas.pushSprite(0, 0);
      break;

    case 4:
      // icons
      switch(mode4_page) {
        case 0:
        case 1:
        case 2:
          pushIcon(frame0, 4, 4);
          break;
        case 3:
          pushIcon(ok, 4, 4);
          break;
        case 4:
        case 5:
        case 6:
          pushIcon(error, 4, 4);
          break;
      }
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }
      
      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Synchronization", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);

      // content
      canvas.setTextColor(BLACK);
      switch(mode4_page) {
        case 0:
          canvas.setTextDatum(top_center);
          canvas.drawString("Connecting to Wi-Fi", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 1 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.setTextDatum(top_center);
          canvas.drawString("Connecting to server", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 2 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.setTextDatum(top_center);
          canvas.drawString("Downloading data", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("[ 3 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 3:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync OK, disconnecting", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("Returning in 3 seconds", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 4:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync error:", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("Wi-Fi error", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 5:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync error:", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("Server error", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 6:
          canvas.setTextDatum(top_center);
          canvas.drawString("Sync error:", M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString("File error", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
      }

      canvas.pushSprite(0, 0);
      break;

    case 5:
      // icons
      pushIcon(me, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_right);
      canvas.drawString("M ->", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
      canvas.setTextDatum(middle_left);
      canvas.drawString("Credits 8)", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);
  
      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(top_center);
      switch(mode5_page) {
        case 0:
          canvas.drawString("2026 PWDer by floriano (0.1) >", M5Cardputer.Display.width() / 2, 50);
          break;
        case 1:
          canvas.drawString("< Visit my website! >", M5Cardputer.Display.width() / 2, 50);
          break;
        case 2:
          canvas.drawString("< Thx for an AES128 library <3", M5Cardputer.Display.width() / 2, 50);
          break;
        default:
          break;
      }
      canvas.setTextDatum(bottom_center);
      canvas.setTextColor(PURPLE);
      switch(mode5_page) {
        case 0:
          canvas.drawString("adamecki/PWDer on GitHub", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 1:
          canvas.drawString("floriano.uk", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        case 2:
          canvas.drawString("josephpal/esp32-Encrypt on GitHub", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          break;
        default:
          break;
      }
      
      canvas.pushSprite(0, 0);
      break;
      
    case 6:
      // icons
      pushIcon(options, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_right);
      canvas.drawString("M ->", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
      canvas.setTextDatum(middle_left);
      canvas.drawString("New passwords found", 40, 20);
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);
      
      // content
      canvas.setTextColor(BLACK);
      canvas.setTextDatum(top_center);
      canvas.drawString("Replace current passwords?", M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString("[ Y / N ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      
      // push
      canvas.pushSprite(0, 0);
      break;

    case 7:
      // icons
      pushIcon(search, 4, 4);
      pushIcon(battery, 4, M5Cardputer.Display.height() - 36);
      if(device_muted) {
        pushIcon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      } else {
        pushIcon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36);
      }

      // captions
      canvas.setTextColor(WHITE);
      canvas.setTextDatum(middle_right);
      if(mode7_show_results) {
        canvas.drawString("M ->", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
        canvas.setTextDatum(middle_left);
        canvas.drawString(mode7_query, 40, 20);
      } else {
        canvas.setTextDatum(middle_left);
        canvas.drawString("Search", 40, 20);
      }
      canvas.drawString(String(M5Cardputer.Power.getBatteryLevel()) + "% | " + String(M5Cardputer.Power.getBatteryVoltage()) + "mV", 40, M5Cardputer.Display.height() - 20);

      // content
      canvas.setTextColor(BLACK);
      if(mode7_show_results) {
        if(M5Cardputer.Keyboard.isKeyPressed('v') && mode7_matches > 0) {
          canvas.setTextDatum(top_center);
          canvas.drawString(username[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, 50);
          canvas.setTextDatum(bottom_center);
          canvas.drawString(password[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        } else {
          if(mode7_matches > 0) {
            canvas.setTextDatum(top_center);
            canvas.drawString(title[mode7_contains_searched_string[mode7_index]], M5Cardputer.Display.width() / 2, 50);
          }
          canvas.setTextDatum(bottom_center);
          if(mode7_matches == 0) {
            canvas.setTextDatum(middle_center);
            canvas.drawString("No results", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
          } else if(mode7_matches == 1) {
            canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
          } else {
            if(mode7_index == 0) {
              canvas.drawString("[ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
            } else if (mode7_index == mode7_matches - 1) {
              canvas.drawString("< [ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
            } else {
              canvas.drawString("< [ " + String(mode7_index + 1) + " / " + String(mode7_matches) + " ] >", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
            }
          }
        }
      } else {
        canvas.setTextDatum(middle_center);
        canvas.drawString(mode7_query + "_", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
      }

      // push
      canvas.pushSprite(0, 0);
      break;
      
    default:
      break;
  }
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
    
    pushIcon(error, 4, 4);

    canvas.setTextDatum(middle_left);
    canvas.drawString("Error", 40, 20);
    
    canvas.setTextColor(BLACK);
    canvas.setTextDatum(top_center);
    canvas.drawString("SD card not found!", M5Cardputer.Display.width() / 2, 50);

    canvas.pushSprite(0, 0);

    // halt system if no sdcard found
    while(1);
  }

  // check if files exist on sdcard
  // if they don't exist - create them and add sample data
  // if they exist - import data

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
    String long_config_read = cipher->decryptString(config_file.readString());
    config_file.close();
    
    String config_read[6];
    
    int startPosition = 0;
    int newlinePosition = long_config_read.indexOf('\n', startPosition);
    for(int i = 0; i < 6; i++) {
      if(newlinePosition != -1) {
        config_read[i] = long_config_read.substring(startPosition, newlinePosition);
      } else { // we treat incomplete config file as an incorrect one
        if(SD.exists(SECRET_FILE_PATH)) {
          SD.remove(SECRET_FILE_PATH);
        }
        
        save_config();
    
        mode0_inputtype = 2;
        wifissid = "sample";
        wifipswd = "password";
        hostname = "192.168.1.100";
        httpport = "2137";
        mode1_devicepassword = "default";

        break;
      }

      startPosition = newlinePosition + 1;
      newlinePosition = long_config_read.indexOf('\n', startPosition);
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
  } else {
    if(SD.exists(SECRET_FILE_PATH)) {
      SD.remove(SECRET_FILE_PATH);
    }
    
    save_config();

    mode0_inputtype = 2;
    wifissid = "sample";
    wifipswd = "password";
    hostname = "192.168.1.100";
    httpport = "2137";
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
    String long_secret_file = cipher->decryptString(secret_file.readString());
    secret_file.close();

    int swcase = 0;

    int startPosition = 0;
    int newlinePosition = long_secret_file.indexOf('\n', startPosition);
    
    while(newlinePosition != -1) {
      if(mode0_max > 99) {
        break;
      } else {
        switch(swcase) {
          case 0:
            title[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(title[mode0_max][title[mode0_max].length() - 1] == '\r') {
              title[mode0_max].remove(title[mode0_max].length() - 1);
            }
            swcase = 1;
            break;
          case 1:
            username[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(username[mode0_max][username[mode0_max].length() - 1] == '\r') {
              username[mode0_max].remove(username[mode0_max].length() - 1);
            }
            swcase = 2;
            break;
          case 2:
            password[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(password[mode0_max][password[mode0_max].length() - 1] == '\r') {
              password[mode0_max].remove(password[mode0_max].length() - 1);
            }
            mode0_max++;
            swcase = 0;
            break;
          default:
            break;
        }
      }
      startPosition = newlinePosition + 1;
      newlinePosition = long_secret_file.indexOf('\n', startPosition);
    }

    if (startPosition < long_secret_file.length()) {
      if(mode0_max < 100) {
        switch(swcase) {
          case 0:
            title[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(title[mode0_max][title[mode0_max].length() - 1] == '\r') {
              title[mode0_max].remove(title[mode0_max].length() - 1);
            }
            swcase = 1;
            break;
          case 1:
            username[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(username[mode0_max][username[mode0_max].length() - 1] == '\r') {
              username[mode0_max].remove(username[mode0_max].length() - 1);
            }
            swcase = 2;
            break;
          case 2:
            password[mode0_max] = long_secret_file.substring(startPosition, newlinePosition);
            if(password[mode0_max][password[mode0_max].length() - 1] == '\r') {
              password[mode0_max].remove(password[mode0_max].length() - 1);
            }
            mode0_max++;
            swcase = 0;
            break;
          default:
            break;
        }
      }
    }

    if(mode0_max < 1) {
      String default_secret_file = "Nothing" + String('\n') + "absolute" + String('\n') + "emptiness";
      File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
      secret_file.print(cipher->encryptString(default_secret_file));
      secret_file.close();
  
      title[0] = "Nothing";
      username[0] = "absolute";
      password[0] = "emptiness";
  
      mode0_max = 1;
    }
  } else {
    String default_secret_file = "Nothing" + String('\n') + "absolute" + String('\n') + "emptiness";
    File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
    secret_file.print(cipher->encryptString(default_secret_file));
    secret_file.close();

    title[0] = "Nothing";
    username[0] = "absolute";
    password[0] = "emptiness";

    mode0_max = 1;
  }

  // Start keyboard
  Keyboard.begin();
  USB.begin();

  drawUI();
}

void loop() {
  // mostly keyboard input
  M5Cardputer.update();

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
            importPasswordsFromNetwork();
          } else if (M5Cardputer.Keyboard.isKeyPressed('o')) { // options
            device_mode = 3;
            drawUI();
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
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode0_selection > 0) { // previous password
            mode0_selection--;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed('v')) {
            drawUI();
            mode0_wasVPressed = true;
          }
          break;
          
        case 1:
          if (mode1_ispasswordbeingchanged && M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
            device_mode = 3;
            mode1_ispasswordbeingchanged = false;
            mode1_passwordinput = "";
            drawUI();
            pushIcon(error, 4, 4);
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
              if(mode1_ispasswordbeingchanged) {
                if(mode1_passwordinput == mode1_devicepassword) {
                  device_mode = 3;
                  mode1_ispasswordbeingchanged = false;
                  mode1_devicepassword = mode3_tempdpwd;
                  save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword);
                  mode1_passwordinput = "";
                  drawUI();
                  pushIcon(ok, 4, 4);
                  canvas.pushSprite(0, 0);
                } else {
                  device_mode = 3;
                  mode1_ispasswordbeingchanged = false;
                  mode1_passwordinput = "";
                  drawUI();
                  pushIcon(error, 4, 4);
                  canvas.pushSprite(0, 0);
                }
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
          } else if((M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('/') && mode3_page < 5) || (M5Cardputer.Keyboard.isKeyPressed('/') && mode3_page == 0)) {
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
                default:
                  break;
              }
              if(!mode1_ispasswordbeingchanged) {
                save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword);
                pushIcon(ok, 4, 4);
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
            switch(mode5_page) {
              case 0:
                for (int i = 0; i < mode5_interactive_hyperlink.length(); i++) {
                  Keyboard.press(mode5_interactive_hyperlink[i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);
                }
                break;
              case 1:
                for (int i = 0; i < mode5_interactive_hyperlink2.length(); i++) {
                  Keyboard.press(mode5_interactive_hyperlink2[i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);
                }
                break;
              case 2:
                for (int i = 0; i < mode5_interactive_hyperlink3.length(); i++) {
                  Keyboard.press(mode5_interactive_hyperlink3[i]);
                  delay(25);
                  Keyboard.releaseAll();
                  delay(25);
                }
                break;
              default:
                break;
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode5_page < 2) { // next password
            mode5_page++;
            drawUI();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode5_page > 0) { // previous password
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
            importPasswordsFromFile();
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
              drawUI();
            } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode7_index > 0) { // previous password
              mode7_index--;
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
            }
          } else {
            if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
              device_mode = 0;
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
