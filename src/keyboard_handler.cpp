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
extern rtc_time_type rtc_time;
extern rtc_date_type rtc_date;

extern pvault::vault entries;
extern pvault::device_settings configuration;
extern uint8_t aes_key[pvault::key_size];
pvault::credential init_cred{};

extern int device_mode;
extern bool network_available;
extern bool rtc_available;
extern bool totp_available;
extern char totp_buffer[7];

extern bool mode0_preview;
extern int mode0_selection;

extern bool mode1_ispasswordbeingchanged;
uint8_t mode1_newpassword_tempkey[pvault::key_size];
extern String mode1_passwordinput;

extern int mode2_page;

extern int mode3_page;
extern String mode3_tempssid;
extern String mode3_tempwpwd;
extern String mode3_tempaddr;
extern String mode3_tempport;
extern String mode3_tempdpwd;

extern int mode5_page;
const String mode5_interactive_hyperlinks[2] PROGMEM = {"https://github.com/adamecki/PWDer", "https://floriano.uk"};

extern String mode7_query;
extern bool mode7_show_results;
extern int mode7_index;
extern int mode7_matches;
extern int mode7_matchindex[100];

void check_keyboard_events() {
  if (M5Cardputer.Keyboard.isChange()) {
    if (mode0_preview && (device_mode == 0 || device_mode == 7)) {
      mode0_preview = false;
      draw_ui();
    }

    if (M5Cardputer.Keyboard.isPressed()) {
      if (configuration.speaker_on == 1 && device_mode != 1) {
        M5Cardputer.Speaker.tone(8000, 20);
      }

      // for complex typing modes
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // mode-specific actions
      switch (device_mode) {
      case 0:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on == 1) {
            configuration.speaker_on = 0;
          } else {
            configuration.speaker_on = 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
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
          if (configuration.input_mode == 0 || configuration.input_mode == 2) {
            for (int i = 0; i < String(entries.credentials[mode0_selection].username).length(); i++) {
              Keyboard.press(String(entries.credentials[mode0_selection].username)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          }

          if (configuration.input_mode == 2) {
            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }

          if (configuration.input_mode == 1 || configuration.input_mode == 2) {
            for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
              Keyboard.press(String(entries.credentials[mode0_selection].password)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          }

          if (configuration.input_mode == 2) {
            Keyboard.press(KEY_RETURN);
            delay(25);
            Keyboard.releaseAll();
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
          for (int i = 0; i < String(entries.credentials[mode0_selection].username).length(); i++) {
            Keyboard.press(String(entries.credentials[mode0_selection].username)[i]);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
          for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
            Keyboard.press(String(entries.credentials[mode0_selection].password)[i]);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
          for (int i = 0; i < String(entries.credentials[mode0_selection].username).length(); i++) {
            Keyboard.press(String(entries.credentials[mode0_selection].username)[i]);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }

          Keyboard.press(KEY_TAB);
          delay(25);
          Keyboard.releaseAll();
          delay(25);

          for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
            Keyboard.press(String(entries.credentials[mode0_selection].password)[i]);
            delay(25);
            Keyboard.releaseAll();
            delay(25);
          }

          Keyboard.press(KEY_RETURN);
          delay(25);
          Keyboard.releaseAll();
        } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
          if ((network_available || rtc_available) && totp_available) {
            generate_totp(String(entries.credentials[mode0_selection].totp_secret));
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
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode0_selection < entries.credential_count) { // next password
          mode0_selection++;
          if (String(entries.credentials[mode0_selection].totp_secret) != "") {
            totp_available = true;
          } else {
            totp_available = false;
          }
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode0_selection > 1) { // previous password
          mode0_selection--;
          if (String(entries.credentials[mode0_selection].totp_secret) != "") {
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
        if (mode1_ispasswordbeingchanged &&
            M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('`')) {
          device_mode = 3;
          mode1_ispasswordbeingchanged = false;
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
            // verify password
            if(pvault::get_key(VAULT_PATH, mode1_passwordinput, mode1_newpassword_tempkey)) {
              // password ok
              if(mode1_ispasswordbeingchanged) {
                // change password
                // rewrite vault
                pvault::init_vault(VAULT_PATH, mode3_tempdpwd, configuration, entries);
                // obtain new key
                memcpy(aes_key, mode1_newpassword_tempkey, pvault::key_size);

                device_mode = 0;
                draw_ui();
                push_icon(ok, 4, 4);
                canvas.pushSprite(0, 0);
              } else {
                // unlock device
                memcpy(aes_key, mode1_newpassword_tempkey, pvault::key_size);
                pvault::load_vault(VAULT_PATH, aes_key, configuration, entries);

                mode3_tempssid = String(entries.credentials[0].title);
                mode3_tempwpwd = String(entries.credentials[0].username);
                mode3_tempaddr = String(entries.credentials[0].password);
                mode3_tempport = String(entries.credentials[0].totp_secret);

                if(SD.exists(IMPORT_FILE_PATH)) {
                  device_mode = 6;
                } else {
                  device_mode = 0;
                  if(String(entries.credentials[mode0_selection].totp_secret) != "") { totp_available = true; }
                }

                draw_ui();
              }
            } else {
              // wrong password
              if(mode1_ispasswordbeingchanged) {
                device_mode = 3;
                draw_ui();
              } else {
                mode1_passwordinput = "";
                draw_ui();
              }
              push_icon(error, 4, 4);
              canvas.pushSprite(0, 0);
            }

            mode1_passwordinput = "";
            mode1_ispasswordbeingchanged = false;
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
          if(configuration.speaker_on == 1) {
            configuration.speaker_on = 0;
          } else {
            configuration.speaker_on = 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('l')) {
          device_mode = 1;
          draw_ui();
        }
        break;

      case 3:
        if (M5Cardputer.Keyboard.isKeyPressed('`') && (M5Cardputer.Keyboard.isKeyPressed(KEY_FN) || mode3_page == 0)) {
          mode3_tempssid = String(entries.credentials[0].title);
          mode3_tempwpwd = String(entries.credentials[0].username);
          mode3_tempaddr = String(entries.credentials[0].password);
          mode3_tempport = String(entries.credentials[0].totp_secret);
          mode3_tempdpwd = "";

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
              switch (configuration.input_mode) {
              case 0:
                configuration.input_mode = 1;
                break;
              case 1:
                configuration.input_mode = 2;
                break;
              case 2:
                configuration.input_mode = 0;
                break;
              }
              draw_ui();
              break;
            case 1:
            case 2:
            case 3:
            case 4:
              strncpy(init_cred.title, (char*)mode3_tempssid.c_str(), sizeof(init_cred.title));
              strncpy(init_cred.username, (char*)mode3_tempwpwd.c_str(), sizeof(init_cred.username));
              strncpy(init_cred.password, (char*)mode3_tempaddr.c_str(), sizeof(init_cred.password));
              strncpy(init_cred.totp_secret, (char*)mode3_tempport.c_str(), sizeof(init_cred.totp_secret));
              break;
            case 5:
              mode1_ispasswordbeingchanged = true;
              device_mode = 1;
              draw_ui();
              break;
            case 6:
              if (configuration.wifi_timeout < 15) {
                configuration.wifi_timeout++;
              } else {
                configuration.wifi_timeout = 0;
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
              export_vault();
              push_icon(ok, 4, 4);
              canvas.pushSprite(0, 0);
              break;
            default:
              break;
            }
            // things that need rewriting encrypted data
            if(mode3_page == 1 || mode3_page == 2 || mode3_page == 3 || mode3_page == 4) {
              entries.credentials[0] = init_cred;
              pvault::update_vault(VAULT_PATH, aes_key, configuration, entries);
              draw_ui();
              push_icon(ok, 4, 4);
              canvas.pushSprite(0, 0);
            }
            // things that need rewriting header only
            if(mode3_page == 0 || mode3_page == 6) {
              pvault::update_config(VAULT_PATH, configuration);
              draw_ui();
              push_icon(ok, 4, 4);
              canvas.pushSprite(0, 0);
            }
            if(mode3_page == 7) {
              draw_ui();
              push_icon(ok, 4, 4);
              canvas.pushSprite(0, 0);
            }
          }
        }
        break;

      case 5:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on == 1) {
            configuration.speaker_on = 0;
          } else {
            configuration.speaker_on = 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
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
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode5_page < 1) { // next page
          mode5_page++;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode5_page > 0) { // previous page
          mode5_page--;
          draw_ui();
        }
        break;

      case 6:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on == 1) {
            configuration.speaker_on = 0;
          } else {
            configuration.speaker_on = 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
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
            if (String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret) != "") {
              totp_available = true;
            } else {
              totp_available = false;
            }
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode7_index > 0) { // previous password
            mode7_index--;
            if (String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret) != "") {
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
            if(configuration.speaker_on == 1) {
              configuration.speaker_on = 0;
            } else {
              configuration.speaker_on = 1;
            }

            pvault::update_config(VAULT_PATH, configuration);
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
            if (configuration.input_mode == 0 || configuration.input_mode == 2) {
              for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
                Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if (configuration.input_mode == 2) {
              Keyboard.press(KEY_TAB);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            if (configuration.input_mode == 1 || configuration.input_mode == 2) {
              for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
                Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
                delay(25);
                Keyboard.releaseAll();
                delay(25);
              }
            }

            if (configuration.input_mode == 2) {
              Keyboard.press(KEY_RETURN);
              delay(25);
              Keyboard.releaseAll();
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
              Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
              Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
              Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            Keyboard.press(KEY_TAB);
            delay(25);
            Keyboard.releaseAll();
            delay(25);

            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
              Keyboard.press(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
              delay(25);
              Keyboard.releaseAll();
              delay(25);
            }

            Keyboard.press(KEY_RETURN);
            delay(25);
            Keyboard.releaseAll();
          } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
            if ((network_available || rtc_available) && totp_available) {
              generate_totp(String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret));
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
            if (String(entries.credentials[mode0_selection].totp_secret) != "") {
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

                for (int i = 1; i < entries.credential_count + 1; i++) {
                  String lowercase_title = String(entries.credentials[i].title);
                  lowercase_title.toLowerCase();

                  if (lowercase_title.indexOf(lowercase_query) != -1) {
                    mode7_matchindex[mode7_matches] = i;
                    mode7_matches++;
                  }
                }

                mode7_show_results = true;
                if (String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret) != "") {
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
