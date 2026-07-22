#include "globals.h"
#include "pwrotocol.h"
#include "gui.h"

extern pvault::vault entries;
extern pvault::device_settings configuration;
extern uint8_t aes_key[pvault::key_size];
extern USBCDC USBSerialDevice;
extern int device_mode;
extern void draw_ui();

void pwrotocol_listen_and_respond() {
  if(USBSerialDevice.available()) {
    uint8_t cmd = USBSerialDevice.read();
    uint8_t salt[pvault::salt_size];

    switch(cmd) {
        case PWROTOCOL_PING:
            USBSerialDevice.write("OK", 2);
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
            if(device_mode != 0) { break; }

            if(SD.exists(IMPORT_FILE_PATH)) { SD.remove(IMPORT_FILE_PATH); }
            File import_file = SD.open(IMPORT_FILE_PATH, FILE_WRITE);

            USBSerialDevice.write("OK", 2);
            USBSerialDevice.flush();

            // === MAGIC ===
            // wait for magic
            unsigned long wait = millis();
            while(USBSerialDevice.available() < 8) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    import_file.close();
                    return;
                }
            }

            // receive magic
            char magic[8];
            if(USBSerialDevice.readBytes((char*)magic, 8) != 8) {
                import_file.close();
                return;
            }
            if(memcmp(magic, "PWIMPORT", 8) != 0) { return; }

            import_file.write(reinterpret_cast<const uint8_t*>(magic), 8);
            USBSerialDevice.write("OK", 2);
            USBSerialDevice.flush();

            // === IV ===
            // wait for nonce
            wait = millis();
            while(USBSerialDevice.available() < pvault::nonce_size) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    import_file.close();
                    return;
                }
            }

            // receive nonce
            uint8_t nonce[pvault::nonce_size];
            if(USBSerialDevice.readBytes((char*)nonce, pvault::nonce_size) != pvault::nonce_size) { return; }
            
            import_file.write(nonce, pvault::nonce_size);
            USBSerialDevice.write("OK", 2);
            USBSerialDevice.flush();

            // === LEN ===
            // wait for ciphertext length
            wait = millis();
            while(USBSerialDevice.available() < sizeof(uint32_t)) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    import_file.close();
                    return;
                }
            }

            // receive ciphertext length
            uint32_t len;
            if(USBSerialDevice.readBytes(reinterpret_cast<char*>(&len), sizeof(len)) != sizeof(len)) { return; }
            if(len != sizeof(pvault::vault)) { return; }

            import_file.write(reinterpret_cast<const uint8_t*>(&len), sizeof(uint32_t));
            USBSerialDevice.write("OK", 2);
            USBSerialDevice.flush();

            // === CIPHERTEXT ===
            // receive chunks of ciphertext

            uint8_t chunk_bytes[PWROTOCOL_CHUNK_SIZE];
            uint32_t bytes_received = 0;
            wait = millis();

            uint32_t chunk_target;
            uint32_t chunk_received;

            while(bytes_received < len) {
                if(len - bytes_received > PWROTOCOL_CHUNK_SIZE) {
                    chunk_target = PWROTOCOL_CHUNK_SIZE;
                } else {
                    chunk_target = len - bytes_received;
                }
                
                chunk_received = 0;

                while(chunk_received < chunk_target) {
                    int avail = USBSerialDevice.available();

                    if(avail > 0) {
                        uint32_t to_read = chunk_target - chunk_received;
                        if((uint32_t)avail < to_read) { to_read = avail; }

                        int r = USBSerialDevice.readBytes(reinterpret_cast<char*>(chunk_bytes + chunk_received), to_read);
                    
                        if(r <= 0) { 
                            import_file.close();
                            return;
                        }

                        chunk_received += r;
                        wait = millis();
                    } else {
                        delay(1);
                        if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                            return;
                            import_file.close();
                        }
                    }
                }

                uint32_t written = import_file.write(chunk_bytes, chunk_target);
                if(written != chunk_target) {    
                    import_file.close();
                    return;
                }

                USBSerialDevice.write("OK", 2);
                USBSerialDevice.flush();

                bytes_received += chunk_target;
            }
            
            // === TAG ===
            // wait for tag
            wait = millis();
            while(USBSerialDevice.available() < pvault::tag_size) {
                delay(1);
                if(millis() - wait > PWROTOCOL_TIMEOUT_MS) {
                    // too long
                    import_file.close();
                    return;
                }
            }

            // receive tag
            uint8_t tag[pvault::tag_size];
            if(USBSerialDevice.readBytes((char*)tag, pvault::tag_size) != pvault::tag_size) {
                import_file.close();
                return;
            }

            import_file.write(tag, pvault::tag_size);
            USBSerialDevice.write("OK", 2);
            USBSerialDevice.flush();

            import_file.close();

            // === IMPORT ===
            // ask
            password_import_screen();
            while(1) {
                M5Cardputer.update();
                if(M5Cardputer.Keyboard.isKeyPressed('n')) {
                    draw_ui();
                    return;
                } else if (M5Cardputer.Keyboard.isKeyPressed('y')) {
                    break;
                }
            }
            
            // get salt from vault file
            uint8_t salt[pvault::salt_size];
            if(!pvault::get_salt(VAULT_PATH, salt)) {
                draw_ui();
                return;
            }

            // import provided data
            pvault::replace_vault(VAULT_PATH, configuration, aes_key, salt, IMPORT_FILE_PATH);
            pvault::load_vault(VAULT_PATH, aes_key, configuration, entries);
            draw_ui();
            break;
        }
    }
}
