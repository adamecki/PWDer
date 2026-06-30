#include "pvault.h"
#include "cryptographic_operations.h"

namespace pvault {
    bool get_key(
        const char* path,
        const String& password,
        uint8_t* key
    ) {
        File file = SD.open(path, FILE_READ);
        if(!file) { return false; }
        
        // get master salt and nonce
        header hdr{};
        pvault_cryptography::read_header(file, hdr);

        // obtain key candidate
        pvault_cryptography::derive_key(password, hdr.master_salt, key);

        // dummy decryption
        vault* dummy = new vault;
        uint32_t len = hdr.ciphertext_length;
        uint8_t* ciphertext = new uint8_t[len];
        uint8_t tag[tag_size];

        if(file.read(ciphertext, len) != len) {
            pvault_cryptography::secure_zero(ciphertext, len);
            pvault_cryptography::secure_zero(tag, tag_size);
            delete dummy;
            delete[] ciphertext;
            file.close();
            return false;
        }
        if(file.read(tag, tag_size) != tag_size) {
            pvault_cryptography::secure_zero(ciphertext, len);
            pvault_cryptography::secure_zero(tag, tag_size);
            delete dummy;
            delete[] ciphertext;
            file.close();
            return false;
        }
        bool decryption = pvault_cryptography::decrypt_gcm(key, hdr.nonce, ciphertext, len, reinterpret_cast<uint8_t*>(dummy), tag);

        pvault_cryptography::secure_zero(ciphertext, len);
        pvault_cryptography::secure_zero(tag, tag_size);
        delete dummy;
        delete[] ciphertext;
        file.close();

        if(decryption) {
            return true;
        } else {
            return false;
        }
    }

    bool init_vault(
        const char* path,
        const String& password,
        const device_settings& settings,
        const vault& data
    ) {
        // input validation
        if(!path || password.length() == 0) { return false; }

        // open file
        if(SD.exists(path)) { SD.remove(path); }
        File file = SD.open(path, FILE_WRITE);
        if(!file) { return false; }

        // buffers
        header hdr{};
        uint8_t salt[salt_size];
        uint8_t nonce[nonce_size];
        uint8_t key[key_size];
        uint8_t tag[tag_size];
        uint32_t len = sizeof(data);

        uint8_t* ciphertext = new uint8_t[len];

        if(!ciphertext) {
            pvault_cryptography::secure_zero(salt, salt_size);
            pvault_cryptography::secure_zero(nonce, nonce_size);
            pvault_cryptography::secure_zero(tag, tag_size);
            pvault_cryptography::secure_zero(key, key_size);
            pvault_cryptography::secure_zero(ciphertext, len);
            delete[] ciphertext;
            file.close();
            return false;
        }

        // generate parameters
        pvault_cryptography::generate_salt(salt);
        pvault_cryptography::generate_nonce(nonce);
        pvault_cryptography::derive_key(password, salt, key);

        // encrypt
        bool encryption = pvault_cryptography::encrypt_gcm(
            key,
            nonce,
            reinterpret_cast<const uint8_t*>(&data),
            len,
            ciphertext,
            tag
        );

        if(!encryption) {
            pvault_cryptography::secure_zero(salt, salt_size);
            pvault_cryptography::secure_zero(nonce, nonce_size);
            pvault_cryptography::secure_zero(tag, tag_size);
            pvault_cryptography::secure_zero(key, key_size);
            pvault_cryptography::secure_zero(ciphertext, len);
            delete[] ciphertext;
            file.close();
            return false;
        }

        // build header
        hdr.magic[0] = 'C';
        hdr.magic[1] = 'R';
        hdr.magic[2] = 'Y';
        hdr.magic[3] = 'P';

        hdr.version = file_version;
        hdr.iterations = iterations;

        memcpy(hdr.master_salt, salt, salt_size);
        memcpy(hdr.nonce, nonce, nonce_size);

        hdr.settings = settings;

        hdr.ciphertext_length = len;

        // save file
        pvault_cryptography::write_header(file, hdr);
        file.write(ciphertext, len);
        pvault_cryptography::write_tag(file, tag);

        // cleanup
        pvault_cryptography::secure_zero(salt, salt_size);
        pvault_cryptography::secure_zero(nonce, nonce_size);
        pvault_cryptography::secure_zero(tag, tag_size);
        pvault_cryptography::secure_zero(key, key_size);
        pvault_cryptography::secure_zero(ciphertext, len);
        delete[] ciphertext;
        file.close();

        return true;
    }

    bool update_vault(
        const char* path,
        const uint8_t* key,
        const device_settings& settings,
        const vault& data
    ) {
        // input validation
        if(!path || !key) { return false; }

        // open file
        File read = SD.open(path, FILE_READ);
        if(!read) { return false; }

        // read header
        header hdr{};
        if(!pvault_cryptography::read_header(read, hdr)) {
            read.close();
            return false;
        }

        // close file (we have everything needed)
        read.close();

        // write header and encrypted data
        SD.remove(path);
        File write = SD.open(path, FILE_WRITE);
        if(!write) {
            return false;
        }

        // generate new nonce
        pvault_cryptography::generate_nonce(hdr.nonce);

        // modify header
        hdr.settings = settings;

        // encrypt
        uint8_t tag[tag_size];
        uint32_t len = sizeof(data);

        uint8_t* ciphertext = new uint8_t[len];

        if(!ciphertext) {
            pvault_cryptography::secure_zero(tag, tag_size);
            pvault_cryptography::secure_zero(ciphertext, len);
            delete[] ciphertext;
            write.close();
            return false;
        }

        bool encryption = pvault_cryptography::encrypt_gcm(
            key,
            hdr.nonce,
            reinterpret_cast<const uint8_t*>(&data),
            len,
            ciphertext,
            tag
        );

        if(!encryption) {
            pvault_cryptography::secure_zero(tag, tag_size);
            pvault_cryptography::secure_zero(ciphertext, len);
            delete[] ciphertext;
            write.close();
            return false;
        }
        
        // save file with modified header
        pvault_cryptography::write_header(write, hdr);
        write.write(ciphertext, len);
        pvault_cryptography::write_tag(write, tag);

        // cleanup
        pvault_cryptography::secure_zero(tag, tag_size);
        pvault_cryptography::secure_zero(ciphertext, len);
        delete[] ciphertext;
        write.close();

        return true;
    }

    bool load_vault(
        const char* path,
        const uint8_t* key,
        device_settings& settings,
        vault& data
    ) {
        // input validation
        if(!path || !key) { return false; }

        // open file
        File file = SD.open(path, FILE_READ);
        if(!file) { return false; }

        // read header
        header hdr{};
        if(!pvault_cryptography::read_header(file, hdr)) {
            file.close();
            return false;
        }

        // header validation
        if(hdr.magic[0] != 'C' || hdr.magic[1] != 'R' || hdr.magic[2] != 'Y' || hdr.magic[3] != 'P') {
            file.close();
            return false;
        }

        if(hdr.version != file_version) {
            file.close();
            return false;
        }

        if(hdr.ciphertext_length == 0) {
            file.close();
            return false;
        }

        settings = hdr.settings;
        
        // buffers
        uint32_t len = hdr.ciphertext_length;
        uint8_t* ciphertext = new uint8_t[len];
        uint8_t nonce[nonce_size];
        uint8_t tag[tag_size];

        if(!ciphertext) {
            delete[] ciphertext;
            file.close();
            return false;
        }

        // read ciphertext
        if(file.read(ciphertext, len) != len) {
            delete[] ciphertext;
            file.close();
            return false;
        }

        // read tag
        if(!pvault_cryptography::read_tag(file, tag)) {
            delete[] ciphertext;
            file.close();
            return false;
        }

        // parameters
        memcpy(nonce, hdr.nonce, nonce_size);
        vault* plainvault = new vault;
        
        bool decryption = pvault_cryptography::decrypt_gcm(
            key,
            nonce,
            ciphertext,
            len,
            reinterpret_cast<uint8_t*>(plainvault),
            tag
        );

        data = *plainvault;

        // cleanup
        delete plainvault;
        delete[] ciphertext;
        file.close();

        return decryption;
    }

    bool update_config(
        const char* path,
        const device_settings& settings
    ) {
        File read = SD.open(path, FILE_READ);
        if(!read) { return false; }
        
        header hdr{};
        pvault_cryptography::read_header(read, hdr);

        hdr.settings = settings;

        uint32_t len = hdr.ciphertext_length;
        uint8_t* ciphertext = new uint8_t[len];
        uint8_t tag[tag_size];

        read.read(ciphertext, len);
        read.read(tag, tag_size);

        read.close();

        SD.remove(path);
        File write = SD.open(path, FILE_WRITE);
        pvault_cryptography::write_header(write, hdr);
        write.write(ciphertext, len);
        pvault_cryptography::write_tag(write, tag);
        write.close();

        delete[] ciphertext;

        return true;
    }

    bool read_config(
        const char* path,
        device_settings& settings
    ) {
        File file = SD.open(path, FILE_READ);
        if(!file) { return false; }

        header hdr{};
        pvault_cryptography::read_header(file, hdr);

        file.close();

        settings = hdr.settings;
        
        return true;
    }
}
