#include "globals.h"
#include "gui.h"

extern pvault::vault entries;
extern pvault::device_settings configuration;
extern uint8_t aes_key[pvault::key_size];

void init_new_vault() {
  pvault::credential init_cred{};

  // first credental is used for storing encrypted configuration
  strncpy(init_cred.title, DEFAULT_SSID, sizeof(init_cred.title));
  strncpy(init_cred.username, DEFAULT_WIFI_PASSWORD, sizeof(init_cred.username));
  strncpy(init_cred.password, DEFAULT_SYNCHOST, sizeof(init_cred.password));
  strncpy(init_cred.totp_secret, DEFAULT_SYNCPORT, sizeof(init_cred.totp_secret));

  entries.credentials[0] = init_cred;

  // second and next are used for storing actual user data
  strncpy(init_cred.title, SAMPLE_ENTRY, sizeof(init_cred.title));
  strncpy(init_cred.username, SAMPLE_USERNAME, sizeof(init_cred.username));
  strncpy(init_cred.password, SAMPLE_PASSWORD, sizeof(init_cred.password));
  strncpy(init_cred.totp_secret, "", sizeof(init_cred.totp_secret));

  entries.credentials[1] = init_cred;
  entries.credential_count = 1;

  configuration.color_scheme = 0;
  configuration.input_mode = 2;
  configuration.speaker_on = 0;
  configuration.wifi_timeout = 5;

  pvault::init_vault(VAULT_PATH, DEFAULT_PASSWORD, configuration, entries);
}

void file_password_import() {
  File import_file = SD.open(IMPORT_FILE_PATH, FILE_READ);
  pvault::credential init_cred{};

  int entries_loaded = 0;
  int linetype = 0;

  while(import_file.available()) {
    if(entries_loaded == 100) {
      break;
    } else {
      String line = import_file.readStringUntil('\n');
      switch(linetype) {
        case 0:
          strncpy(init_cred.title, line.c_str(), sizeof(init_cred.title));
          break;
        case 1:
          strncpy(init_cred.username, line.c_str(), sizeof(init_cred.username));
          break;
        case 2:
          strncpy(init_cred.password, line.c_str(), sizeof(init_cred.password));
          break;
        case 3:
          strncpy(init_cred.totp_secret, line.c_str(), sizeof(init_cred.totp_secret));
          break;
      }
      if(linetype == 3) {
        linetype = 0;
        entries_loaded++;
        entries.credentials[entries_loaded] = init_cred;
      } else {
        linetype++;
      }
    }
  }

  import_file.close();
  if(entries_loaded != 0) {
    pvault::credential empty_cred{};
    // remove entries above loaded one
    for(int i = entries_loaded + 1; i < pvault::max_entries; i++) {
      entries.credentials[i] = empty_cred;
    }
    entries.credential_count = entries_loaded;

    // update vault and load it
    pvault::update_vault(VAULT_PATH, aes_key, configuration, entries);
  }

  SD.remove(IMPORT_FILE_PATH);
}

void export_vault() {
  String export_save_string = "";
  for(int i = 1; i <= entries.credential_count; i++) {
    export_save_string += String(entries.credentials[i].title) + String('\n');
    export_save_string += String(entries.credentials[i].username) + String('\n');
    export_save_string += String(entries.credentials[i].password) + String('\n');
    export_save_string += String(entries.credentials[i].totp_secret) + String('\n');
    if(i < entries.credential_count) {
      export_save_string += String('\n');
    }
  }

  if(SD.exists(EXPORT_FILE_PATH)) { SD.remove(EXPORT_FILE_PATH); }

  File export_file = SD.open(EXPORT_FILE_PATH, FILE_WRITE);
  export_file.print(export_save_string);
  export_file.close();
}
