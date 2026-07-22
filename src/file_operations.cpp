#include "globals.h"
#include "gui.h"

extern pvault::vault entries;
extern pvault::device_settings configuration;
extern uint8_t aes_key[pvault::key_size];

extern USBCDC USBSerialDevice;

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
