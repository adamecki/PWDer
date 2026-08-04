#include "globals.h"

#include "file_operations.h"
#include "gui.h"
#include "sfx.h"
#include "icons.h"
#include "network_operations.h"
#include "keyboard_bridge.h"
#include "time_operations.h"

#define BASIC_BRIGHTNESS 0xFF
extern const int color_schemes_number;

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
extern int mode3_tempbrightness;


extern int mode5_page;
const String mode5_interactive_hyperlinks[2] PROGMEM = {"https://github.com/adamecki/PWDer", "https://floriano.uk"};

extern String mode7_query;
extern bool mode7_show_results;
extern int mode7_index;
extern int mode7_matches;
extern int mode7_matchindex[100];

extern int auto_lock;
extern unsigned long last_action;

void press_with_preferred_keyboard(special_key k) {
  if(ble_keyboard_ready()) {
    ble_send_key(k);
  } else {
    usb_send_key(k);
  }
}

void press_with_preferred_keyboard(char c) {
  if(ble_keyboard_ready()) {
    ble_send_key(c);
  } else {
    usb_send_key(c);
  }
}

void check_keyboard_events() {
  if(M5Cardputer.BtnA.wasPressed()) {
    if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN)) {
      save_screenshot_bmp();
      sfx::melody_c_major_scale();
    } else {
      // keyboard mode to be introduced
    }
  }

  if (M5Cardputer.Keyboard.isChange()) {
    last_action = millis();

    if(auto_lock != 0) {
      auto_lock = 0;
      M5Cardputer.Display.setBrightness(BASIC_BRIGHTNESS);
      return;
    }

    if (mode0_preview && (device_mode == 0 || device_mode == 7)) {
      mode0_preview = false;
      draw_ui();
    }

    if (M5Cardputer.Keyboard.isPressed()) {
      if (device_mode != 1) { sfx::beep(); }

      // for complex typing modes
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      // mode-specific actions
      switch (device_mode) {
      case 0:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on_auto_lock & 1) {
            configuration.speaker_on_auto_lock -= 1;
          } else {
            configuration.speaker_on_auto_lock += 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('q')) { // search (query)
          device_mode = 7;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('h')) { // help
          device_mode = 2;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('c')) { // about
          device_mode = 5;
          draw_ui();
        } else if(M5Cardputer.Keyboard.isKeyPressed('b')) { // enable bluetooth keyboard
          ble_keyboard_init();
        // } else if (M5Cardputer.Keyboard.isKeyPressed('s')) { // synchronize (deprecated)
        //   device_mode = 4;
        //   draw_ui();
        //   net_password_import();
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
              press_with_preferred_keyboard(String(entries.credentials[mode0_selection].username)[i]);
            }
          }

          if (configuration.input_mode == 2) {
            press_with_preferred_keyboard(special_key::TAB);
          }

          if (configuration.input_mode == 1 || configuration.input_mode == 2) {
            for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
              press_with_preferred_keyboard(String(entries.credentials[mode0_selection].password)[i]);
            }
          }

          if (configuration.input_mode == 2) {
            press_with_preferred_keyboard(special_key::RETURN);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
          for (int i = 0; i < String(entries.credentials[mode0_selection].username).length(); i++) {
            press_with_preferred_keyboard(String(entries.credentials[mode0_selection].username)[i]);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
          for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
            press_with_preferred_keyboard(String(entries.credentials[mode0_selection].password)[i]);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
          for (int i = 0; i < String(entries.credentials[mode0_selection].username).length(); i++) {
            press_with_preferred_keyboard(String(entries.credentials[mode0_selection].username)[i]);
          }

          press_with_preferred_keyboard(special_key::TAB);

          for (int i = 0; i < String(entries.credentials[mode0_selection].password).length(); i++) {
            press_with_preferred_keyboard(String(entries.credentials[mode0_selection].password)[i]);
          }

          press_with_preferred_keyboard(special_key::RETURN);
        } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
          if ((network_available || rtc_available) && totp_available) {
            generate_totp(String(entries.credentials[mode0_selection].totp_secret));
            for (int i = 0; i < 6; i++) {
              press_with_preferred_keyboard(totp_buffer[i]);
            }
            totp_buffer[6] = '\0';
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('t')) { // press TAB on a computer
          press_with_preferred_keyboard(special_key::TAB);
        } else if (M5Cardputer.Keyboard.isKeyPressed('r')) { // press RETURN on a computer
          press_with_preferred_keyboard(special_key::RETURN);
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
          push_icon(error, 4, 4, 1);
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
            splash_screen();
            splash_screen_create_progressbar();
            delay(250);
            splash_screen_update_progressbar_percentage(10);

            // verify password
            if(pvault::get_key(VAULT_PATH, mode1_passwordinput, mode1_newpassword_tempkey)) {
              splash_screen_update_progressbar_percentage(33);
              delay(250);

              // password ok
              if(mode1_ispasswordbeingchanged) {
                // change password
                // rewrite vault
                pvault::init_vault(VAULT_PATH, mode3_tempdpwd, configuration, entries);
                // obtain new key
                memcpy(aes_key, mode1_newpassword_tempkey, pvault::key_size);

                splash_screen_update_progressbar_percentage(100);
                delay(250);

                mode3_tempdpwd = "";
                device_mode = 0;
                draw_ui();
                push_icon(ok, 4, 4, 1);
                canvas.pushSprite(0, 0);
              } else {
                // unlock device
                memcpy(aes_key, mode1_newpassword_tempkey, pvault::key_size);
                pvault::load_vault(VAULT_PATH, aes_key, configuration, entries);

                mode3_tempssid = String(entries.credentials[0].title);
                mode3_tempwpwd = String(entries.credentials[0].username);
                mode3_tempaddr = String(entries.credentials[0].password);
                mode3_tempport = String(entries.credentials[0].totp_secret);

                // if(SD.exists(IMPORT_FILE_PATH)) {
                //   device_mode = 6;
                // } else {
                device_mode = 0;
                if(String(entries.credentials[mode0_selection].totp_secret) != "") { totp_available = true; }
                // }

                splash_screen_update_progressbar_percentage(100);
                delay(250);

                draw_ui();
              }
            } else {
              splash_screen_update_progressbar_percentage(33);
              delay(250);

              // wrong password
              if(mode1_ispasswordbeingchanged) {
                mode3_tempdpwd = "";
                device_mode = 3;
                draw_ui();
              } else {
                mode1_passwordinput = "";
                draw_ui();
              }
              push_icon(error, 4, 4, 1);
              canvas.pushSprite(0, 0);
              delay(250);
            }

            mode1_passwordinput = "";
            mode1_ispasswordbeingchanged = false;
          }
        }
        break;

      case 2:
        if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          if((mode2_page != 1) || (mode2_page == 1 && M5Cardputer.Keyboard.isKeyPressed(KEY_FN))) {
            // challenge user to learn FN + ESC/arrows combinations
            device_mode = 0;
            draw_ui();
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode2_page > 0) {
          if((mode2_page != 1) || (mode2_page == 1 && M5Cardputer.Keyboard.isKeyPressed(KEY_FN))) {
            mode2_page--;
            draw_ui();
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode2_page < 10) {
          if((mode2_page != 1) || (mode2_page == 1 && M5Cardputer.Keyboard.isKeyPressed(KEY_FN))) {
            mode2_page++;
            draw_ui();
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on_auto_lock & 1) {
            configuration.speaker_on_auto_lock -= 1;
          } else {
            configuration.speaker_on_auto_lock += 1;
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
          mode3_tempdpwd = "";
          mode3_tempbrightness = configuration.brightness;
          M5Cardputer.Display.setBrightness(configuration.brightness);
          // mode3_tempaddr = String(entries.credentials[0].password);
          // mode3_tempport = String(entries.credentials[0].totp_secret);

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
        } else if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed(';') && mode3_page == 0) {
          if(mode3_tempbrightness < 0xFF) {
            mode3_tempbrightness += 0x10;

            if(mode3_tempbrightness & 0xF) {
              mode3_tempbrightness = (mode3_tempbrightness & ~0xF) | (0xF);
            }

            M5Cardputer.Display.setBrightness(mode3_tempbrightness);
            draw_ui();
          }
        } else if(M5Cardputer.Keyboard.isKeyPressed(KEY_FN) && M5Cardputer.Keyboard.isKeyPressed('.') && mode3_page == 0) {
          if(mode3_tempbrightness > 0x0F) {
            mode3_tempbrightness -= 0x10;

            if(mode3_tempbrightness & 0xF) {
              mode3_tempbrightness = (mode3_tempbrightness & ~0xF) | (0xF);
            }

            M5Cardputer.Display.setBrightness(mode3_tempbrightness);
            draw_ui();
          }
        } else {
          for (auto i : status.word) {
            switch (mode3_page) {
            case 4:
              mode3_tempdpwd += i;
              break;
            case 6:
              mode3_tempssid += i;
              break;
            case 7:
              mode3_tempwpwd += i;
              break;
            default:
              break;
            }
            draw_ui();
          }

          if (status.del) {
            switch (mode3_page) {
            case 4:
              mode3_tempdpwd.remove(mode3_tempdpwd.length() - 1);
              break;
            case 6:
              mode3_tempssid.remove(mode3_tempssid.length() - 1);
              break;
            case 7:
              mode3_tempwpwd.remove(mode3_tempwpwd.length() - 1);
              break;
            default:
              break;
            }
            draw_ui();
          }

          if (status.enter) {
            switch (mode3_page) {
            case 1:
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
            case 2:
              switch(configuration.speaker_on_auto_lock) {
                case 0:
                case 1:
                  configuration.speaker_on_auto_lock += 2;
                  break;
                case 2:
                case 3:
                  configuration.speaker_on_auto_lock -= 2;
                  break;
                default:
                  break;
              }
              break;
            case 3:
              if(configuration.color_scheme >= color_schemes_number - 1) {
                configuration.color_scheme = 0;
              } else {
                configuration.color_scheme += 1;
              }
              break;
            case 4:
              mode1_ispasswordbeingchanged = true;
              device_mode = 1;
              draw_ui();
              break;
            case 5:
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
            case 6:
            case 7:
              strncpy(init_cred.title, (char*)mode3_tempssid.c_str(), sizeof(init_cred.title));
              strncpy(init_cred.username, (char*)mode3_tempwpwd.c_str(), sizeof(init_cred.username));
              break;
            default:
              break;
            }
            // things that need rewriting encrypted data
            if(mode3_page == 6 || mode3_page == 7) {
              entries.credentials[0] = init_cred;
              pvault::update_vault(VAULT_PATH, aes_key, configuration, entries);
              draw_ui();
              push_icon(ok, 4, 4, 1);
              canvas.pushSprite(0, 0);
            }
            // things that need rewriting header only
            if(mode3_page == 0 || mode3_page == 1 || mode3_page == 2 || mode3_page == 3) {
              configuration.brightness = mode3_tempbrightness;
              pvault::update_config(VAULT_PATH, configuration);
              draw_ui();
              push_icon(ok, 4, 4, 1);
              canvas.pushSprite(0, 0);
            }
            // things that just need graphical confirmation
            if(mode3_page == 5) {
              draw_ui();
              push_icon(ok, 4, 4, 1);
              canvas.pushSprite(0, 0);
            }
          }
        }
        break;

      case 5:
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
          if(configuration.speaker_on_auto_lock & 1) {
            configuration.speaker_on_auto_lock -= 1;
          } else {
            configuration.speaker_on_auto_lock += 1;
          }

          pvault::update_config(VAULT_PATH, configuration);
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          device_mode = 0;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
          for (int i = 0; i < mode5_interactive_hyperlinks[mode5_page].length(); i++) {
            press_with_preferred_keyboard(mode5_interactive_hyperlinks[mode5_page][i]);
          }
        } else if (M5Cardputer.Keyboard.isKeyPressed('/') && mode5_page < 1) { // next page
          mode5_page++;
          draw_ui();
        } else if (M5Cardputer.Keyboard.isKeyPressed(',') && mode5_page > 0) { // previous page
          mode5_page--;
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
            if(configuration.speaker_on_auto_lock & 1) {
              configuration.speaker_on_auto_lock -= 1;
            } else {
              configuration.speaker_on_auto_lock += 1;
            }

            pvault::update_config(VAULT_PATH, configuration);
            draw_ui();
          } else if(M5Cardputer.Keyboard.isKeyPressed('b')) { // enable bluetooth keyboard
            ble_keyboard_init();
          } else if(M5Cardputer.Keyboard.isKeyPressed('n')) { // retry network connection
            retry_connection();
          } else if (M5Cardputer.Keyboard.isKeyPressed('h')) { // help
            device_mode = 2;
            draw_ui();
          } else if (M5Cardputer.Keyboard.isKeyPressed('t')) { // press TAB on a computer
            press_with_preferred_keyboard(special_key::TAB);
          } else if (M5Cardputer.Keyboard.isKeyPressed('r')) { // press RETURN on a computer
            press_with_preferred_keyboard(special_key::RETURN);
          } else if (M5Cardputer.Keyboard.isKeyPressed('v')) {
            draw_ui();
            mode0_preview = true;
          } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) { // enter data using default mode
            if (configuration.input_mode == 0 || configuration.input_mode == 2) {
              for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
                press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
              }
            }

            if (configuration.input_mode == 2) {
              press_with_preferred_keyboard(special_key::TAB);
            }

            if (configuration.input_mode == 1 || configuration.input_mode == 2) {
              for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
                press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
              }
            }

            if (configuration.input_mode == 2) {
              press_with_preferred_keyboard(special_key::RETURN);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('1')) { // enter username
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
              press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('2')) { // enter password
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
              press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
            }
          } else if (M5Cardputer.Keyboard.isKeyPressed('3')) { // enter all
            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].username).length(); i++) {
              press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].username)[i]);
            }

            press_with_preferred_keyboard(special_key::TAB);

            for (int i = 0; i < String(entries.credentials[mode7_matchindex[mode7_index]].password).length(); i++) {
              press_with_preferred_keyboard(String(entries.credentials[mode7_matchindex[mode7_index]].password)[i]);
            }

            press_with_preferred_keyboard(special_key::RETURN);
          } else if (M5Cardputer.Keyboard.isKeyPressed('4')) { // Enter TOTP if available
            if ((network_available || rtc_available) && totp_available) {
              generate_totp(String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret));
              for (int i = 0; i < 6; i++) {
                press_with_preferred_keyboard(totp_buffer[i]);
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
