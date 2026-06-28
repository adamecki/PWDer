#include "globals.h"

#include "file_operations.h"
#include "gui.h"
#include "icons.h"
#include "network_operations.h"
#include "time_operations.h"

extern USBHIDKeyboard Keyboard;
extern M5Canvas canvas;
extern NTPClient timeClient;
extern Unit_RTC RTC;
extern Cipher* cipher;
extern rtc_time_type rtc_time;
extern rtc_date_type rtc_date;
extern int wifi_timeout_seconds;
extern int device_mode;
extern bool device_muted;
extern bool network_available;
extern bool rtc_available;
extern bool totp_available;
extern char totp_buffer[7];
extern String wifissid;
extern String wifipswd;
extern String hostname;
extern String httpport;

extern bool mode0_preview;
extern int mode0_inputtype;
extern int mode0_selection;
extern int mode0_max;
extern String title[100];
extern String username[100];
extern String password[100];
extern String totp_secret[100];

extern bool mode1_ispasswordbeingchanged;
extern bool mode1_ispasswordbeingexported;
extern String mode1_passwordinput;
extern String mode1_devicepassword;

extern int mode2_page;

extern int mode3_page;
extern String mode3_tempssid;
extern String mode3_tempwpwd;
extern String mode3_tempaddr;
extern String mode3_tempport;
extern String mode3_tempdpwd;

extern int mode5_page;
const String mode5_interactive_hyperlinks[5] PROGMEM = {"https://github.com/adamecki/PWDer", "https://floriano.uk",
                                                        "https://github.com/josephpal/esp32-Encrypt",
                                                        "https://github.com/lucadentella/TOTP-Arduino",
                                                        "https://github.com/dirkx/Arduino-Base32-Decode"};

extern String mode7_query;
extern bool mode7_show_results;
extern int mode7_index;
extern int mode7_matches;
extern int mode7_contains_searched_string[100];

void check_keyboard_events() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (mode0_preview && (device_mode == 0 || device_mode == 7)) {
      mode0_preview = false;
      draw_ui();
    }

    if (M5Cardputer.Keyboard.isPressed()) {
      if (device_muted == false && device_mode != 1) {
        M5Cardputer.Speaker.tone(8000, 20);
      }

      // for complex typing modes
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // mode-specific actions
      switch (device_mode) {
      case 0:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          device_muted = !device_muted;
          if (!device_muted) {
            save_spkstate("1");
          } else {
            save_spkstate("0");
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('q')) { // search (query)
          device_mode = 7;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('h')) { // help
          device_mode = 2;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('c')) { // credits
          device_mode = 5;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('s')) { // synchronize
          device_mode = 4;
          draw_ui();
          net_password_import();
        } else if (M5Cardputer.Keyboard.isKeyPressed('o')) { // options
          device_mode = 3;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('n')) {
          retry_connection();
        } else if (M5Cardputer.Keyboard.isKeyPressed('l')) { // lock
          device_mode = 1;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // enter data using default mode
          if (mode0_inputtype == 0 || mode0_inputtype == 2) {
            for (int i = 0; i < username[mode0_selection].length(); i++) {
              Keyboard.press(username[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          }

          if (mode0_inputtype == 2) {
            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }

          if (mode0_inputtype == 1 || mode0_inputtype == 2) {
            for (int i = 0; i < password[mode0_selection].length(); i++) {
              Keyboard.press(password[mode0_selection][i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          }

          if (mode0_inputtype == 2) {
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
          if ((network_available || rtc_available) && totp_available) {
            generate_totp(totp_secret[mode0_selection]);
            for (int i = 0; i < 6; i++) {
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
          if (totp_secret[mode0_selection] != "") {
            totp_available = true;
          } else {
            totp_available = false;
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode0_selection > 0) { // previous password
          mode0_selection--;
          if (totp_secret[mode0_selection] != "") {
            totp_available = true;
          } else {
            totp_available = false;
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('v')) {
          draw_ui();
          mode0_preview = true;
        }
        break;

      case 1:
        if ((mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported) &&
            M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
          device_mode = 3;
          mode1_ispasswordbeingchanged = false;
          mode1_ispasswordbeingexported = false;
          mode1_passwordinput = "";
          draw_ui();
          push_icon(error, 4, 4);
          canvas.pushSprite(0, 0);
        } else {
          for (auto i : status.word) {
            mode1_passwordinput += i;
            draw_ui();
          }

          if (status.del) {
            mode1_passwordinput.remove(mode1_passwordinput.length() - 1);
            draw_ui();
          }

          if (status.enter) {
            if (mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported) {
              if (mode1_passwordinput == mode1_devicepassword) {
                device_mode = 3;
                if (mode1_ispasswordbeingchanged) {
                  mode1_devicepassword = mode3_tempdpwd;
                  save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword,
                              String(wifi_timeout_seconds));

                  // Update vault with new password
                  File old_vault = SD.open(SECRET_FILE_PATH, FILE_READ);
                  String old_vault_str = cipher->decryptString(old_vault.readString());
                  old_vault.close();

                  if (SD.exists(SECRET_FILE_PATH)) {
                    SD.remove(SECRET_FILE_PATH);
                  }

                  File new_vault = SD.open(SECRET_FILE_PATH, FILE_WRITE);
                  String new_vault_str = mode1_devicepassword +
                                         old_vault_str.substring(old_vault_str.indexOf('\n', 0)); // replace first line
                  new_vault.print(cipher->encryptString(new_vault_str));
                  new_vault.close();
                } else if (mode1_ispasswordbeingexported) {
                  export_vault();
                }
                draw_ui();
                push_icon(ok, 4, 4);
              } else {
                device_mode = 3;
                draw_ui();
                push_icon(error, 4, 4);
              }

              canvas.pushSprite(0, 0);

              mode1_passwordinput = "";
              mode1_ispasswordbeingchanged = false;
              mode1_ispasswordbeingexported = false;
            } else {
              if (mode1_passwordinput == mode1_devicepassword) {
                // after unlocking
                // check if there are passwords to import
                if (SD.exists(IMPORT_FILE_PATH)) {
                  device_mode = 6;
                } else {
                  device_mode = 0;
                }
                mode1_passwordinput = "";
                draw_ui();
              } else {
                mode1_passwordinput = "";
                draw_ui();
              }
            }
          }
        }
        break;

      case 2:
        if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          device_mode = 0;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode2_page > 0) {
          mode2_page--;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode2_page < 4) {
          mode2_page++;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          device_muted = !device_muted;
          if (!device_muted) {
            save_spkstate("1");
          } else {
            save_spkstate("0");
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('l')) {
          device_mode = 1;
          draw_ui();
        }
        break;

      case 3:
        if (M5Cardputer.Keyboard.isKeyPressed('`') && (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) || mode3_page == 0)) {
          mode3_tempssid = wifissid;
          mode3_tempwpwd = wifipswd;
          mode3_tempaddr = hostname;
          mode3_tempport = httpport;
          mode3_tempdpwd = mode1_devicepassword;

          device_mode = 0;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed(',') &&
                   mode3_page > 0) {
          mode3_page--;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('/') &&
                   mode3_page < MODE3_PAGES_NUMBER - 1) {
          mode3_page++;
          draw_ui();
        } else {
          for (auto i : status.word) {
            switch (mode3_page) {
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
            draw_ui();
          }

          if (status.del) {
            switch (mode3_page) {
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
            draw_ui();
          }

          if (status.enter) {
            switch (mode3_page) {
            case 0:
              switch (mode0_inputtype) {
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
              draw_ui();
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
              draw_ui();
              break;
            case 6:
              if (wifi_timeout_seconds < 15) {
                wifi_timeout_seconds++;
              } else {
                wifi_timeout_seconds = 0;
              }
              break;
            case 7:
              if (rtc_available) {
                if (!network_available) {
                  retry_connection();
                }
                if (network_available) {
                  timeClient.update();
                  struct tm new_time = to_regular_utc_timestamp(timeClient.getEpochTime());

                  rtc_time.Hours = new_time.tm_hour;
                  rtc_time.Minutes = new_time.tm_min;
                  rtc_time.Seconds = new_time.tm_sec;
                  RTC.setTime(&rtc_time);

                  rtc_date.Year = new_time.tm_year + 1900;
                  rtc_date.Month = new_time.tm_mon + 1;
                  rtc_date.Date = new_time.tm_mday;
                  RTC.setDate(&rtc_date);
                }
                draw_ui();
              }
              break;
            case 8:
              mode1_ispasswordbeingexported = true;
              device_mode = 1;
              draw_ui();
              break;
            default:
              break;
            }
            if (!(mode1_ispasswordbeingchanged || mode1_ispasswordbeingexported)) {
              save_config(String(mode0_inputtype), wifissid, wifipswd, hostname, httpport, mode1_devicepassword,
                          String(wifi_timeout_seconds));
              draw_ui();
              push_icon(ok, 4, 4);
              canvas.pushSprite(0, 0);
            }
          }
        }
        break;

      case 5:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          device_muted = !device_muted;
          if (!device_muted) {
            save_spkstate("1");
          } else {
            save_spkstate("0");
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          device_mode = 0;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
          for (int i = 0; i < mode5_interactive_hyperlinks[mode5_page].length(); i++) {
            Keyboard.press(mode5_interactive_hyperlinks[mode5_page][i]);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode5_page < 4) { // next page
          mode5_page++;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode5_page > 0) { // previous page
          mode5_page--;
          draw_ui();
        }
        break;

      case 6:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          device_muted = !device_muted;
          if (!device_muted) {
            save_spkstate("1");
          } else {
            save_spkstate("0");
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('y')) {
          file_password_import();
          device_mode = 0;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('n')) {
          SD.remove(IMPORT_FILE_PATH);
          device_mode = 0;
          draw_ui();
        }
        break;

      case 7:
        if (mode7_show_results) {
          if (M5Cardputer.Keyboard.isKeyPressed('/') && mode7_index < mode7_matches - 1) { // next password
            mode7_index++;
            if (totp_secret[mode7_contains_searched_string[mode7_index]] != "") {
              totp_available = true;
            } else {
              totp_available = false;
            }
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode7_index > 0) { // previous password
            mode7_index--;
            if (totp_secret[mode7_contains_searched_string[mode7_index]] != "") {
              totp_available = true;
            } else {
              totp_available = false;
            }
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed('`')) {
            mode7_matches = 0;
            mode7_index = 0;
            mode7_show_results = false;
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed('l')) {
            device_mode = 1;
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed('m')) {
            device_muted = !device_muted;
            if (!device_muted) {
              save_spkstate("1");
            } else {
              save_spkstate("0");
            }
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed('t')) { // press TAB on a computer
            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('r')) { // press RETURN on a computer
            Keyboard.press(KEY_RETURN);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('v')) {
            draw_ui();
            mode0_preview = true;
          } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // enter data using default mode
            if (mode0_inputtype == 0 || mode0_inputtype == 2) {
              for (int i = 0; i < username[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(username[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if (mode0_inputtype == 2) {
              Keyboard.press(KEY_TAB);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            if (mode0_inputtype == 1 || mode0_inputtype == 2) {
              for (int i = 0; i < password[mode7_contains_searched_string[mode7_index]].length(); i++) {
                Keyboard.press(password[mode7_contains_searched_string[mode7_index]][i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if (mode0_inputtype == 2) {
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
            if ((network_available || rtc_available) && totp_available) {
              generate_totp(totp_secret[mode7_contains_searched_string[mode7_index]]);
              for (int i = 0; i < 6; i++) {
                Keyboard.press(totp_buffer[i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
              totp_buffer[6] = '\0';
            }
          }
        } else {
          if (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
            device_mode = 0;
            if (totp_secret[mode0_selection] != "") {
              totp_available = true;
            } else {
              totp_available = false;
            }
            draw_ui();
          } else {
            for (auto i : status.word) {
              mode7_query += i;
              draw_ui();
            }

            if (status.del) {
              mode7_query.remove(mode7_query.length() - 1);
              draw_ui();
            }

            if (status.enter) {
              if (mode7_query != "") {
                String lowercase_query = mode7_query;
                lowercase_query.toLowerCase();

                for (int i = 0; i < mode0_max; i++) {
                  String lowercase_title = title[i];
                  lowercase_title.toLowerCase();

                  if (lowercase_title.indexOf(lowercase_query) != -1) {
                    mode7_contains_searched_string[mode7_matches] = i;
                    mode7_matches++;
                  }
                }

                mode7_show_results = true;
                if (totp_secret[mode7_contains_searched_string[mode7_index]] != "") {
                  totp_available = true;
                } else {
                  totp_available = false;
                }
                draw_ui();
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
