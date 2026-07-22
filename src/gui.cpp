#include "globals.h"

#include "gui.h"
#include "icons.h"
#include "time_operations.h"
#include "keyboard_bridge.h"

extern M5Canvas canvas;
extern Unit_RTC RTC;
extern rtc_time_type rtc_time;

extern bool network_available;
extern bool rtc_available;
extern bool totp_available;
extern char totp_buffer[7];
extern int8_t last_battery_percentage;

extern int device_mode;
extern int mode2_page;
extern int mode3_page;
extern int mode4_page;
extern int mode5_page;

extern int mode0_selection;

extern String mode1_passwordinput;

extern String mode3_tempssid;
extern String mode3_tempwpwd;
extern String mode3_tempaddr;
extern String mode3_tempport;
extern String mode3_tempdpwd;

extern bool mode7_show_results;
extern String mode7_query;
extern int mode7_matches;
extern int mode7_matchindex[100];
extern int mode7_index;

extern pvault::vault entries;
extern pvault::device_settings configuration;

extern const pwder_style::color_scheme pwcolors[1];
const int current_scheme = 0; // make it changeable later

void push_icon(const pwder_style::icon32 &icon, int xoffset, int yoffset, int scale) {
  canvas.fillRect(xoffset, yoffset, 32 * scale, 32 * scale, pwcolors[current_scheme].background_color);

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
      int index = ((y * 32) + x) * 3;

      uint8_t r = icon.bitmap[index];
      uint8_t g = icon.bitmap[index + 1];
      uint8_t b = icon.bitmap[index + 2];

      // xs - X scaled, ys - Y scaled
      for (int xs = 1; xs <= scale; xs++) {
        for (int ys = 1; ys <= scale; ys++) {
          if(!icon.mask[index / 3]) {
            canvas.drawPixel((x * scale) + xoffset + xs, (y * scale) + yoffset + ys, lgfx::v1::color565(r, g, b));
          }
        }
      }
    }
  }
}

void no_sdcard_crash_screen() {
  canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), pwcolors[current_scheme].background_color);
  canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, pwcolors[current_scheme].border_color);
  canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, pwcolors[current_scheme].foreground_color);

  // icon
  push_icon(error, 4, 4, 1);

  // caption
  canvas.setTextDatum(middle_left);
  canvas.drawString(SDCARD_NOT_FOUND_TITLE, 40, 20);

  // content
  canvas.setTextColor(pwcolors[current_scheme].background_text_color);
  canvas.setTextDatum(top_center);
  canvas.drawString(SDCARD_NOT_FOUND_DESCRIPTION, M5Cardputer.Display.width() / 2, 50);

  canvas.pushSprite(0, 0);
}

void password_import_screen() {
  canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), pwcolors[current_scheme].background_color);
  canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, pwcolors[current_scheme].border_color);
  canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, pwcolors[current_scheme].foreground_color);

  // icon
  push_icon(options, 4, 4, 1);

  // caption
  canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
  canvas.setTextDatum(middle_left);
  canvas.drawString(PWIMPORT_TITLEBAR, 40, 20);

  // content
  canvas.setTextColor(pwcolors[current_scheme].background_text_color);
  canvas.setTextDatum(top_center);
  canvas.drawString(PWIMPORT_REPLACE_PASSWORDS, M5Cardputer.Display.width() / 2, 50);
  canvas.setTextDatum(bottom_center);
  canvas.drawString("[ Y / N ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);

  canvas.pushSprite(0, 0);
}

void draw_ui() {
  String mode1_asterisks = ""; // password mask, also used in mode 3 (options) to hide Wi-Fi and device password
  String enumerator = "";

  // background
  canvas.fillRect(0, 0, M5Cardputer.Display.width(), M5Cardputer.Display.height(), pwcolors[current_scheme].background_color);
  canvas.fillRect(0, 40, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 80, pwcolors[current_scheme].border_color);
  canvas.fillRect(0, 44, M5Cardputer.Display.width(), M5Cardputer.Display.height() - 88, pwcolors[current_scheme].foreground_color);

  switch (device_mode) {
  case 0:
    // icon
    push_icon(key, 4, 4, 1);

    // caption
    canvas.setTextDatum(middle_left);
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.drawString(PASSWORD_MANAGER_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    if (M5Cardputer.Keyboard.isKeyPressed('v')) {
      canvas.setTextDatum(top_center);
      if ((network_available || rtc_available) && totp_available) {
        generate_totp(String(entries.credentials[mode0_selection].totp_secret));
        canvas.setTextColor(BLUE);
        canvas.drawString(String(entries.credentials[mode0_selection].username) + " | " + String(totp_buffer), M5Cardputer.Display.width() / 2, 50);
        canvas.setTextColor(pwcolors[current_scheme].background_text_color);
        totp_buffer[6] = '\0';
      } else {
        canvas.drawString(String(entries.credentials[mode0_selection].username), M5Cardputer.Display.width() / 2, 50);
      }
      canvas.setTextDatum(bottom_center);
      canvas.drawString(String(entries.credentials[mode0_selection].password), M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
    } else {
      canvas.setTextDatum(top_center);
      if ((network_available || rtc_available) && totp_available) {
        canvas.setTextColor(BLUE);
      }
      canvas.drawString(String(entries.credentials[mode0_selection].title), M5Cardputer.Display.width() / 2, 50);
      canvas.setTextColor(pwcolors[current_scheme].background_text_color);
      canvas.setTextDatum(bottom_center);
      if (entries.credential_count == 1) {
        canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      } else {
        enumerator = "";
        if (mode0_selection == 1) {
          enumerator = "[ " + String(mode0_selection) + " / " + String(entries.credential_count) + " ] >";
        } else if (mode0_selection == entries.credential_count) {
          enumerator = "< [ " + String(mode0_selection) + " / " + String(entries.credential_count) + " ]";
        } else {
          enumerator = "< [ " + String(mode0_selection) + " / " + String(entries.credential_count) + " ] >";
        }
        canvas.drawString(enumerator, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      }
    }
    break;

  case 1:
    // icon
    push_icon(padlock, 4, 4, 1);

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    canvas.drawString(LOGIN_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    canvas.setTextDatum(middle_center);
    mode1_asterisks = "";
    for (int i = 0; i < mode1_passwordinput.length(); i++) {
      mode1_asterisks += "*";
    }
    canvas.drawString(mode1_asterisks + "_", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
    break;

  case 2:
    // icon
    push_icon(help, 4, 4, 1);

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    canvas.drawString(HANDBOOK_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    switch (mode2_page) {
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
    push_icon(options, 4, 4, 1);

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    canvas.drawString(OPTIONS_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    canvas.setTextDatum(top_center);
    switch (mode3_page) {
    case 0:
      switch (configuration.input_mode) {
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
      break;
    case 1:
      canvas.drawString(OPTIONS_SSID + mode3_tempssid + "_", M5Cardputer.Display.width() / 2, 50);
      break;
    case 2:
      mode1_asterisks = "";
      for (int i = 0; i < mode3_tempwpwd.length(); i++) {
        mode1_asterisks += "*";
      }
      canvas.drawString(OPTIONS_WIFI_PASSWORD + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
      break;
    case 3:
      canvas.drawString(OPTIONS_SYNC_HOST + mode3_tempaddr + "_", M5Cardputer.Display.width() / 2, 50);
      break;
    case 4:
      canvas.drawString(OPTIONS_SYNC_PORT + mode3_tempport + "_", M5Cardputer.Display.width() / 2, 50);
      break;
    case 5:
      mode1_asterisks = "";
      for (int i = 0; i < mode3_tempdpwd.length(); i++) {
        mode1_asterisks += "*";
      }
      canvas.drawString(OPTIONS_DEVICE_PASSWORD + mode1_asterisks + "_", M5Cardputer.Display.width() / 2, 50);
      break;
    case 6:
      if (configuration.wifi_timeout == 0) {
        canvas.drawString(OPTIONS_WIFI_OFF, M5Cardputer.Display.width() / 2, 50);
      } else {
        canvas.drawString(OPTIONS_WIFI_TIMEOUT + String(configuration.wifi_timeout) + OPTIONS_WIFI_TIMEOUT_SECONDS_SHORTCUT,
                          M5Cardputer.Display.width() / 2, 50);
      }
      break;
    case 7:
      if (rtc_available) {
        RTC.getTime(&rtc_time);
        String nice_hours = "";
        String nice_minutes = "";
        if (rtc_time.Hours < 10) {
          nice_hours = "0" + String(rtc_time.Hours);
        } else {
          nice_hours = String(rtc_time.Hours);
        }
        if (rtc_time.Minutes < 10) {
          nice_minutes = "0" + String(rtc_time.Minutes);
        } else {
          nice_minutes = String(rtc_time.Minutes);
        }
        canvas.drawString(OPTIONS_RTC_TIME + nice_hours + ":" + nice_minutes + " UTC" + OPTIONS_RTC_NTPSYNC,
                          M5Cardputer.Display.width() / 2, 50);
      } else {
        canvas.drawString(OPTIONS_RTC_NTPSYNC_RTC_UNAVAILABLE, M5Cardputer.Display.width() / 2, 50);
      }
      break;
    case 8:
      canvas.drawString(OPTIONS_EXPORT_VAULT, M5Cardputer.Display.width() / 2, 50);
      break;
    default:
      break;
    }
    enumerator = "";
    if (mode3_page != 0) {
      enumerator += "< ";
    }
    enumerator += "[ " + String(mode3_page + 1) + " / " + String(MODE3_PAGES_NUMBER) + " ]";
    if (mode3_page != MODE3_PAGES_NUMBER - 1) {
      enumerator += " >";
    }
    canvas.setTextDatum(bottom_center);
    canvas.drawString(enumerator, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
    break;

  case 4:
    // icon
    switch (mode4_page) {
    case 0:
    case 1:
    case 2:
      push_icon(drum, 4, 4, 1);
      break;
    case 3:
      push_icon(ok, 4, 4, 1);
      break;
    case 4:
    case 5:
    case 6:
      push_icon(error, 4, 4, 1);
      break;
    }

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    canvas.drawString(SYNC_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    canvas.setTextDatum(top_center);
    switch (mode4_page) {
    case 0:
      canvas.drawString(SYNC_WIFI_CONNECTING_PHASE, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString("[ 1 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    case 1:
      canvas.drawString(SYNC_SERVER_CONNECTING_PHASE, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString("[ 2 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    case 2:
      canvas.drawString(SYNC_DOWNLOAD_PHASE, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString("[ 3 / 3 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    case 3:
      canvas.drawString(SYNC_OK, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString(SYNC_RETURN, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    case 4:
      canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString(SYNC_ERR_DESCRIPTION_WIFI, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    case 5:
      canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString(SYNC_ERR_DESCRIPTION_SERVER, M5Cardputer.Display.width() / 2,
                        M5Cardputer.Display.height() - 50);
      break;
    case 6:
      canvas.drawString(SYNC_ERR, M5Cardputer.Display.width() / 2, 50);
      canvas.setTextDatum(bottom_center);
      canvas.drawString(SYNC_ERR_DESCRIPTION_FILE, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
      break;
    }
    break;

  case 5:
    // icon
    push_icon(me, 4, 4, 1);

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    canvas.drawString(CREDITS_TITLEBAR, 40, 20);

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    canvas.setTextDatum(top_center);
    switch (mode5_page) {
      case 0:
        canvas.drawString(CREDITS_PWDER, M5Cardputer.Display.width() / 2, 50);
        break;
      case 1:
        canvas.drawString(CREDITS_WEBSITE, M5Cardputer.Display.width() / 2, 50);
        break;
      default:
        break;
    }
    canvas.setTextDatum(bottom_center);
    canvas.setTextColor(PURPLE);
    switch (mode5_page) {
      case 0:
        canvas.drawString(CREDITS_PWDER_GITHUB, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        break;
      case 1:
        canvas.drawString(CREDITS_WEBSITE_LINK, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        break;
      default:
        break;
    }
    break;

  case 7:
    // icon
    push_icon(search, 4, 4, 1);

    // caption
    canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
    canvas.setTextDatum(middle_left);
    if (mode7_show_results) {
      canvas.drawString(mode7_query, 40, 20);
    } else {
      canvas.drawString(SEARCH_TITLEBAR, 40, 20);
    }

    // content
    canvas.setTextColor(pwcolors[current_scheme].background_text_color);
    if (mode7_show_results) {
      if (M5Cardputer.Keyboard.isKeyPressed('v') && mode7_matches > 0) {
        canvas.setTextDatum(top_center);
        if ((network_available || rtc_available) && totp_available) {
          generate_totp(String(entries.credentials[mode7_matchindex[mode7_index]].totp_secret));
          canvas.setTextColor(BLUE);
          canvas.drawString(String(entries.credentials[mode7_matchindex[mode7_index]].username) + " | " + String(totp_buffer),
                            M5Cardputer.Display.width() / 2, 50);
          canvas.setTextColor(pwcolors[current_scheme].background_text_color);
          totp_buffer[6] = '\0';
        } else {
          canvas.drawString(String(entries.credentials[mode7_matchindex[mode7_index]].username), M5Cardputer.Display.width() / 2, 50);
        }
        canvas.setTextDatum(bottom_center);
        canvas.drawString(String(entries.credentials[mode7_matchindex[mode7_index]].password), M5Cardputer.Display.width() / 2,
                          M5Cardputer.Display.height() - 50);
      } else {
        if (mode7_matches > 0) {
          canvas.setTextDatum(top_center);
          if ((network_available || rtc_available) && totp_available) {
            canvas.setTextColor(BLUE);
          }
          canvas.drawString(String(entries.credentials[mode7_matchindex[mode7_index]].title), M5Cardputer.Display.width() / 2, 50);
          canvas.setTextColor(pwcolors[current_scheme].background_text_color);
        }
        canvas.setTextDatum(bottom_center);
        if (mode7_matches == 0) {
          canvas.setTextDatum(middle_center);
          canvas.drawString(SEARCH_NO_RESULTS, M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() / 2);
        } else if (mode7_matches == 1) {
          canvas.drawString("[ 1 / 1 ]", M5Cardputer.Display.width() / 2, M5Cardputer.Display.height() - 50);
        } else {
          enumerator = "";
          if (mode7_index == 0) {
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
  if (configuration.speaker_on == 0 && device_mode != 1) {
    push_icon(loudspeaker, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36, 1);
  } else if (configuration.speaker_on == 1 && device_mode != 1) {
    push_icon(loudspeaker_unmuted, M5Cardputer.Display.width() - 36, M5Cardputer.Display.height() - 36, 1);
  }

  canvas.setTextColor(pwcolors[current_scheme].foreground_text_color);
  canvas.setTextDatum(middle_right);

  // icon placement
  int iconx[3] = { 204, 168, 132 };
  bool iconx_used[3] = { false, false, false };

  if(ble_keyboard_ready()) {
    for(int i = 0; i < 3; i++) {
      if(!iconx_used[i]) {
        iconx_used[i] = true;
        push_icon(bluetooth, iconx[i], 4, 1);
        break;
      }
    }
  }

  if(network_available) {
    for(int i = 0; i < 3; i++) {
      if(!iconx_used[i]) {
        iconx_used[i] = true;
        push_icon(network, iconx[i], 4, 1);
        break;
      }
    }
  }

  if(rtc_available) {
    for(int i = 0; i < 3; i++) {
      if(!iconx_used[i]) {
        iconx_used[i] = true;
        push_icon(clockicon, iconx[i], 4, 1);
        break;
      }
    }
  }

  if (device_mode != 1) {
    push_icon(help, M5Cardputer.Display.width() - 108, M5Cardputer.Display.height() - 36, 1);
    if (!(device_mode == 7 && mode7_show_results == false)) {
      canvas.drawString("M ", M5Cardputer.Display.width() - 40, M5Cardputer.Display.height() - 20);
      canvas.drawString("H ", M5Cardputer.Display.width() - 112, M5Cardputer.Display.height() - 20);
    }
  }

  canvas.setTextDatum(middle_left);
  canvas.drawString(String(last_battery_percentage) + "%", 40, M5Cardputer.Display.height() - 20);
  push_icon(battery, 4, M5Cardputer.Display.height() - 36, 1);

  // vault usage
  int vault_usage_statusbar_width = map(entries.credential_count, 0, pvault::max_entries - 1, 0, M5Cardputer.Display.width());
  canvas.fillRect(0, 40, vault_usage_statusbar_width, 4, pwcolors[current_scheme].statusbar_color);

  // current position or progress
  int statusbar_left = 0;
  int statusbar_width = 0;

  switch(device_mode) {
    case 0:
      if(entries.credential_count != 0) {
        statusbar_width = M5Cardputer.Display.width() / entries.credential_count;
        statusbar_left = map(mode0_selection, 1, entries.credential_count, 0, M5Cardputer.Display.width() - statusbar_width);
      }
      break;
    case 2:
      statusbar_width = M5Cardputer.Display.width() / 4;
      statusbar_left = map(mode2_page, 0, 3, 0, M5Cardputer.Display.width() - statusbar_width);
      break;
    case 3:
      statusbar_width = M5Cardputer.Display.width() / MODE3_PAGES_NUMBER;
      statusbar_left = map(mode3_page, 0, MODE3_PAGES_NUMBER - 1, 0, M5Cardputer.Display.width() - statusbar_width);
      break;
    // case 4:
    //   break;
    case 5:
      statusbar_width = M5Cardputer.Display.width() / 2;
      statusbar_left = map(mode5_page, 0, 1, 0, M5Cardputer.Display.width() - statusbar_width);
      break;
    case 7:
      if(mode7_matches != 0) {
        statusbar_width = M5Cardputer.Display.width() / mode7_matches;
        statusbar_left = map(mode7_index, 0, mode7_matches - 1, 0, M5Cardputer.Display.width() - statusbar_width);
      }
      break;
    default:
      break;
  }

  statusbar_width += 1; // bro
  canvas.fillRect(statusbar_left, M5Cardputer.Display.height() - 44, statusbar_width, 4, pwcolors[current_scheme].statusbar_color);

  canvas.pushSprite(0, 0);
}
