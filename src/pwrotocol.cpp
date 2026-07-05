#include "globals.h"
#include "pwrotocol.h"

extern pvault::vault entries;
extern USBCDC USBSerialDevice;
extern int device_mode;
extern void draw_ui();

void pwrotocol_listen_and_respond() {
  if(USBSerialDevice.available()) {
    uint8_t cmd = USBSerialDevice.read();
    uint8_t salt[pvault::salt_size];

    switch(cmd) {
        case PWROTOCOL_PING:
            USBSerialDevice.write("OK", sizeof("OK"));
            break;
        case PWROTOCOL_GET_SALT:
            if(!pvault::get_salt(VAULT_PATH, salt)) {
                USBSerialDevice.write("FAIL", sizeof("FAIL"));
                break;
            }
            
            USBSerialDevice.write(salt, pvault::salt_size);
            break;
        case PWROTOCOL_GET_ITERATIONS:
            USBSerialDevice.write(reinterpret_cast<const uint8_t*>(&pvault::iterations), sizeof(pvault::iterations));
            break;
        case PWROTOCOL_REQUEST_CONF:
            USBSerialDevice.write(reinterpret_cast<const uint8_t*>(&entries.credentials[0]), sizeof(pvault::credential));
            break;
        case PWROTOCOL_FILE_IMPORT:
            // wait for magic
            unsigned long wait = millis();
            while(USBSerialDevice.available() < 8) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    return;
                }
            }

            // receive magic
            char magic[8];
            if(USBSerialDevice.readBytes((char*)magic, 8) != 8) {
                return;
            }
            if(memcmp(magic, "PWIMPORT", 8) != 0) { return; }

            // wait for nonce
            wait = millis();
            while(USBSerialDevice.available() < pvault::nonce_size) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    return;
                }
            }

            // receive nonce
            uint8_t nonce[pvault::nonce_size];
            if(USBSerialDevice.readBytes((char*)nonce, pvault::nonce_size) != pvault::nonce_size) { return; }

            // wait for ciphertext length
            wait = millis();
            while(USBSerialDevice.available() < sizeof(uint32_t)) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    return;
                }
            }

            // receive ciphertext length
            uint32_t len;
            if(USBSerialDevice.readBytes(reinterpret_cast<char*>(&len), sizeof(len)) != sizeof(len)) { return; }
            if(len != sizeof(pvault::vault)) { return; }

            // allocate buffer for ciphertext
            uint8_t* ciphertext = new uint8_t[len];
            if(!ciphertext) { return; }

            // receive ciphertext
            wait = millis();
            size_t bytes_received = 0;
            while(bytes_received < len) {
                if(USBSerialDevice.available()) {
                    bytes_received += USBSerialDevice.readBytes(reinterpret_cast<char*>(ciphertext + bytes_received), len - bytes_received);
                }
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    delete[] ciphertext;
                    return;
                }
            }
            
            // wait for tag
            wait = millis();
            while(USBSerialDevice.available() < pvault::tag_size) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    delete[] ciphertext;
                    return;
                }
            }

            // receive tag
            uint8_t tag[pvault::tag_size];
            if(USBSerialDevice.readBytes((char*)tag, pvault::tag_size) != pvault::tag_size) {
                delete[] ciphertext;
                return;
            }

            // write to file for further import
            if(SD.exists(IMPORT_FILE_PATH)) { SD.remove(IMPORT_FILE_PATH); }

            File file = SD.open(IMPORT_FILE_PATH, FILE_WRITE);
            if(!file) {
                delete[] ciphertext;
                return;
            }

            file.write(reinterpret_cast<const uint8_t*>(magic), 8);
            file.write(nonce, pvault::nonce_size);
            file.write(reinterpret_cast<const uint8_t*>(&len), sizeof(len));
            file.write(ciphertext, len);
            file.write(tag, pvault::tag_size);

            file.close();
            delete[] ciphertext;

            device_mode = 6;
            draw_ui();

            break;
        }
    }
}
