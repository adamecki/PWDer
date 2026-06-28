#include "globals.h"
#include "gui.h"

extern Cipher* cipher;
extern int mode0_inputtype;
extern String wifissid;
extern String wifipswd;
extern String hostname;
extern String httpport;
extern String mode1_devicepassword;
extern int wifi_timeout_seconds;
extern bool device_muted;
extern int mode0_max;
extern String title[100];
extern String username[100];
extern String password[100];
extern String totp_secret[100];

bool config_read_correct = true;

void read_spkstate() {
  String spkstate_read;
  File spkstate = SD.open(SPKSTATE_FILE_PATH, FILE_READ);
  spkstate_read = spkstate.readStringUntil('\r');
  device_muted = !spkstate_read.toInt();
  spkstate.close();
}

void save_spkstate(String state = "0") {
  // The speaker state is in another file due to frequent speaker state changes and overwriting a big file every time
  // would be inefficient it is also not encrypted
  if (SD.exists(SPKSTATE_FILE_PATH)) {
    SD.remove(SPKSTATE_FILE_PATH);
  }

  File spkstate_file = SD.open(SPKSTATE_FILE_PATH, FILE_WRITE);
  spkstate_file.println(state);
  spkstate_file.close();
}

void save_config(String input_mode = "2", String ssid = "sample", String wpwd = "password",
                 String ipaddr = "192.168.1.100", String port = "7305", String devpwd = "default",
                 String timeout = "5") {
  if (SD.exists(CONFIG_FILE_PATH)) {
    SD.remove(CONFIG_FILE_PATH);
  }

  String config_save_string = input_mode + String('\n') + ssid + String('\n') + wpwd + String('\n') + ipaddr +
                              String('\n') + port + String('\n') + devpwd + String('\n') + timeout + String('\n');

  File config_file = SD.open(CONFIG_FILE_PATH, FILE_WRITE);
  config_file.print(cipher->encryptString(config_save_string));
  config_file.close();
}

void init_sample_config() {
  save_config();

  mode0_inputtype = 2;
  wifissid = "sample";
  wifipswd = "password";
  hostname = "192.168.1.100";
  httpport = "7305";
  mode1_devicepassword = "default";
  wifi_timeout_seconds = 5;
}

void read_and_verify_config() {
  File config_file = SD.open(CONFIG_FILE_PATH, FILE_READ);
  String config_read_string = cipher->decryptString(config_file.readString());
  config_file.close();

  String config_read[7];
  int start_position = 0;
  int newline_position = config_read_string.indexOf('\n', start_position);

  for (int i = 0; i < 7; i++) {
    if (newline_position != -1) {
      config_read[i] = config_read_string.substring(start_position, newline_position);
    } else {
      if (SD.exists(SECRET_FILE_PATH)) {
        SD.remove(SECRET_FILE_PATH);
      }
      config_read_correct = false;
      init_sample_config();
      break;
    }

    start_position = newline_position + 1;
    newline_position = config_read_string.indexOf('\n', start_position);
  }

  if (config_read_correct) {
    mode0_inputtype = config_read[0].toInt();
    wifissid = config_read[1];
    wifipswd = config_read[2];
    hostname = config_read[3];
    httpport = config_read[4];
    mode1_devicepassword = config_read[5];
    wifi_timeout_seconds = config_read[6].toInt();
  }
}

void init_sample_secret() {
  String sample_secret_file = mode1_devicepassword + String('\n') + SAMPLE_ENTRY + String('\n') + SAMPLE_USERNAME +
                              String('\n') + SAMPLE_PASSWORD + String('\n');
  File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
  secret_file.print(cipher->encryptString(sample_secret_file));
  secret_file.close();

  title[0] = SAMPLE_ENTRY;
  username[0] = SAMPLE_USERNAME;
  password[0] = SAMPLE_PASSWORD;
  totp_secret[0] = "";

  mode0_max = 1;
}

void read_and_verify_secret() {
  File secret_file = SD.open(SECRET_FILE_PATH, FILE_READ);
  String secret_read_string = cipher->decryptString(secret_file.readString());
  secret_file.close();

  int linetype = 0;
  int start_position = 0;
  int newline_position = secret_read_string.indexOf('\n', start_position);

  if (newline_position != -1) {
    String passcheck = secret_read_string.substring(start_position, newline_position);

    if (passcheck[passcheck.length() - 1] == '\r') {
      passcheck.remove(passcheck.length() - 1);
    }

    if (passcheck != mode1_devicepassword) {
      if (SD.exists(SECRET_FILE_PATH)) {
        SD.remove(SECRET_FILE_PATH);
      }
      if (SD.exists(CONFIG_FILE_PATH)) {
        SD.remove(CONFIG_FILE_PATH);
      }

      integrity_check_failed_crash_screen();

      while (1);
    }

    start_position = newline_position + 1;
    newline_position = secret_read_string.indexOf('\n', start_position);

    while (newline_position != -1) {
      if (mode0_max > 99) {
        break;
      } else {
        switch (linetype) {
        case 0:
          title[mode0_max] = secret_read_string.substring(start_position, newline_position);
          if (title[mode0_max][title[mode0_max].length() - 1] == '\r') {
            title[mode0_max].remove(title[mode0_max].length() - 1);
          }
          linetype = 1;
          break;
        case 1:
          username[mode0_max] = secret_read_string.substring(start_position, newline_position);
          if (username[mode0_max][username[mode0_max].length() - 1] == '\r') {
            username[mode0_max].remove(username[mode0_max].length() - 1);
          }
          linetype = 2;
          break;
        case 2:
          password[mode0_max] = secret_read_string.substring(start_position, newline_position);
          if (password[mode0_max][password[mode0_max].length() - 1] == '\r') {
            password[mode0_max].remove(password[mode0_max].length() - 1);
          }
          linetype = 3;
          break;
        case 3:
          totp_secret[mode0_max] = secret_read_string.substring(start_position, newline_position);
          if (totp_secret[mode0_max][totp_secret[mode0_max].length() - 1] == '\r') {
            totp_secret[mode0_max].remove(totp_secret[mode0_max].length() - 1);
          }
          mode0_max++;
          linetype = 0;
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
    if (mode0_max < 100) {
      switch (linetype) {
      case 0:
        title[mode0_max] = secret_read_string.substring(start_position, newline_position);
        if (title[mode0_max][title[mode0_max].length() - 1] == '\r') {
          title[mode0_max].remove(title[mode0_max].length() - 1);
        }
        linetype = 1;
        break;
      case 1:
        username[mode0_max] = secret_read_string.substring(start_position, newline_position);
        if (username[mode0_max][username[mode0_max].length() - 1] == '\r') {
          username[mode0_max].remove(username[mode0_max].length() - 1);
        }
        linetype = 2;
        break;
      case 2:
        password[mode0_max] = secret_read_string.substring(start_position, newline_position);
        if (password[mode0_max][password[mode0_max].length() - 1] == '\r') {
          password[mode0_max].remove(password[mode0_max].length() - 1);
        }
        linetype = 3;
        break;
      case 3:
        totp_secret[mode0_max] = secret_read_string.substring(start_position, newline_position);
        if (totp_secret[mode0_max][totp_secret[mode0_max].length() - 1] == '\r') {
          totp_secret[mode0_max].remove(totp_secret[mode0_max].length() - 1);
        }
        mode0_max++;
        linetype = 0;
        break;
      default:
        break;
      }
    }
  }

  // If no passwords are present
  if (mode0_max < 1) {
    init_sample_secret();
  }
}

void file_password_import() {
  File import_file = SD.open(IMPORT_FILE_PATH, FILE_READ);

  // reset
  mode0_max = 0;

  int swcase = 0;
  String import_string = mode1_devicepassword + String('\n');
  while (import_file.available()) {
    if (mode0_max == 100) {
      break;
    } else {
      String import_line = import_file.readStringUntil('\n');
      import_string += import_line + "\n";
      switch (swcase) {
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

  if (SD.exists(SECRET_FILE_PATH)) {
    SD.remove(SECRET_FILE_PATH);
  }

  File secret_file = SD.open(SECRET_FILE_PATH, FILE_WRITE);
  secret_file.print(cipher->encryptString(import_string));
  secret_file.close();
}

void export_vault() {
  if (SD.exists(EXPORT_FILE_PATH)) {
    SD.remove(EXPORT_FILE_PATH);
  }

  String export_save_string = "";

  for (int i = 0; i < mode0_max; i++) {
    export_save_string += title[i] + String('\n');
    export_save_string += username[i] + String('\n');
    export_save_string += password[i] + String('\n');
    export_save_string += totp_secret[i];
    if (i < (mode0_max - 1)) {
      export_save_string += String('\n');
    }
  }

  File export_file = SD.open(EXPORT_FILE_PATH, FILE_WRITE);
  export_file.print(export_save_string);
  export_file.close();
}
