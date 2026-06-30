#include "globals.h"
#include "gui.h"

extern NTPClient timeClient;
extern bool network_available;
extern int device_mode;
extern int mode4_page;

extern pvault::vault entries;
extern pvault::device_settings configuration;

void read_response(WiFiClient* client, String& lines) {
  unsigned long timeout = millis();
  while (client->available() == 0) {
    if (millis() - timeout > 5000) {
      client->stop();
      mode4_page = 5;
      draw_ui();
      delay(3000);
      mode4_page = 0;
      device_mode = 0;
      draw_ui();
      return;
    }
  }

  while (client->available()) {
    lines += client->readStringUntil('\r');
  }
}

void retry_connection(bool reconnect = true) {
  if (reconnect) {
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);
  }

  if (configuration.wifi_timeout != 0) {
    WiFi.begin(entries.credentials[0].title, entries.credentials[0].username);

    int timeout_500ms = 0;

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      timeout_500ms++;
      if (timeout_500ms >= (2 * configuration.wifi_timeout)) {
        break;
      }
    }

    network_available = false;

    if (timeout_500ms < (2 * configuration.wifi_timeout)) {
      network_available = true;
      timeClient.begin();
    }
  }
}

void net_password_import() {
  if (network_available) {
    const char* host = entries.credentials[0].password;
    const int port = (int)entries.credentials[0].totp_secret;

    String import_string = "";

    mode4_page = 1;
    draw_ui();

    WiFiClient client;
    String footer = String(" HTTP/1.1\r\n") + "Host: " + String(host) + "\r\n" + "Connection: close\r\n\r\n";
    String readRequest = "GET /" + footer;

    if (!client.connect(host, port)) {
      mode4_page = 5;
      draw_ui();
      delay(3000);
      mode4_page = 0;
      device_mode = 0;
      draw_ui();
      return;
    }

    mode4_page = 2;
    draw_ui();

    client.print(readRequest);
    read_response(&client, import_string);

    if (SD.exists(IMPORT_FILE_PATH)) {
      SD.remove(IMPORT_FILE_PATH);
    }

    File import_file = SD.open(IMPORT_FILE_PATH, FILE_WRITE);

    int lines_read = 0;
    int start = 0;
    while (true) {
      int end = import_string.indexOf('\n', start);
      if (end == -1) {
        String line = import_string.substring(start);
        import_file.print(line + String('\n'));
        break;
      }

      String line = import_string.substring(start, end);
      start = end + 1;
      if (lines_read > 4) {
        import_file.print(line + String('\n'));
      }

      lines_read++;
    }

    import_file.close();

    if (lines_read > 7) {
      mode4_page = 3;
      draw_ui();
      delay(3000);

      mode4_page = 0;
      device_mode = 6;
      draw_ui();
    } else {
      if (SD.exists(IMPORT_FILE_PATH)) {
        SD.remove(IMPORT_FILE_PATH);
      }

      mode4_page = 6;
      draw_ui();
      delay(3000);

      mode4_page = 0;
      device_mode = 0;
      draw_ui();
    }
  } else {
    mode4_page = 4;
    draw_ui();
    delay(3000);
    mode4_page = 0;
    device_mode = 0;
    draw_ui();
    return;
  }
}
