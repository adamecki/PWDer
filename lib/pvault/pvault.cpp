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

        uint32_t len = hdr.ciphertext_length;

        // get tag
        uint8_t tag[pvault::tag_size];
        uint32_t file_size = file.size();
        file.seek(file_size - pvault::tag_size);
        file.read(tag, pvault::tag_size);

        bool decryption = pvault_cryptography::decrypt_verify(key, hdr.nonce, file, len, tag, sizeof(pvault::header));
        
        file.close();

        return decryption;
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
        uint32_t len = sizeof(data);

        // generate parameters
        pvault_cryptography::generate_salt(salt);
        pvault_cryptography::generate_nonce(nonce);
        pvault_cryptography::derive_key(password, salt, key);

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

        pvault_cryptography::write_header(file, hdr);

        // encrypt
        bool encryption = pvault_cryptography::encrypt_chunked(
            key,
            nonce,
            reinterpret_cast<const uint8_t*>(&data),
            len,
            file
        );

        // cleanup
        pvault_cryptography::secure_zero(salt, salt_size);
        pvault_cryptography::secure_zero(nonce, nonce_size);
        pvault_cryptography::secure_zero(key, key_size);
        file.close();

        return encryption;
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

        pvault_cryptography::write_header(write, hdr);

        // encrypt
        uint32_t len = sizeof(data);

        bool encryption = pvault_cryptography::encrypt_chunked(
            key,
            hdr.nonce,
            reinterpret_cast<const uint8_t*>(&data),
            len,
            write
        );

        // cleanup
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
        uint8_t nonce[nonce_size];

        // parameters
        memcpy(nonce, hdr.nonce, nonce_size);
        
        bool decryption = pvault_cryptography::decrypt_chunked(
            key,
            nonce,
            file,
            len,
            reinterpret_cast<uint8_t*>(&data)
        );

        // cleanup
        file.close();
        return decryption;
    }

    bool update_config(
        const char* path,
        const device_settings& settings
    ) {
        // open the old file
        File old_file = SD.open(path, FILE_READ);
        if(!old_file) { return false; }

        // read its header
        header hdr{};
        if(!pvault_cryptography::read_header(old_file, hdr)) {
            old_file.close();
            return false;
        }

        // update settings in header
        hdr.settings = settings;

        // obtain length and add tag size to it
        uint32_t len = hdr.ciphertext_length;
        len += pvault::tag_size;

        // open temp file
        String temp_path = String(path);
        temp_path += ".temp";

        if(SD.exists(temp_path)) { SD.remove(temp_path); }
        File new_file = SD.open(temp_path, FILE_WRITE);
        if(!new_file) {
            old_file.close();
            return false;
        }

        // rewrite header and remaining bytes
        if(!pvault_cryptography::write_header(new_file, hdr)) {
            new_file.close();
            old_file.close();
            return false;
        }

        uint32_t to_write = len;
        uint32_t write_size;
        
        uint8_t buffer[CHUNK_SIZE];

        while(to_write > 0) {
            if(to_write > CHUNK_SIZE) {
                write_size = CHUNK_SIZE;
            } else {
                write_size = to_write;
            }
        
            old_file.read(buffer, write_size);
            new_file.write(buffer, write_size);

            to_write -= write_size;
        }

        old_file.close();
        new_file.close();

        // delete old file and replace with new
        SD.remove(path);
        if(SD.exists(temp_path)) { SD.rename(temp_path, path); }

        // exit
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

    bool get_salt(
        const char* path,
        uint8_t* salt
    ) {
        if (!path) { return false; }

        File file = SD.open(path, FILE_READ);
        if(!file) { return false; }
        header hdr{};
        if(!pvault_cryptography::read_header(file, hdr)) {
            file.close();
            return false;
        }

        memcpy(salt, hdr.master_salt, pvault::salt_size);

        file.close();
        return true;
    }

    bool replace_vault(
        const char* path,
        const device_settings& settings,
        
        const uint8_t* key,
        const uint8_t* salt,
        
        const char* import_path
    ) {
        File import_file = SD.open(import_path, FILE_READ);
        // read file header
        // magic 8 bytes
        uint8_t magic[8];
        import_file.read(magic, 8);
    
        if(memcmp(magic, "PWIMPORT", 8) != 0) {
            return false;
        }

        // nonce 12 bytes
        uint8_t nonce[pvault::nonce_size];
        import_file.read(nonce, pvault::nonce_size);

        // length 4 bytes
        uint32_t len;
        import_file.read(reinterpret_cast<uint8_t*>(&len), sizeof(uint32_t));

        // read tag
        uint32_t file_size = import_file.size();
        uint8_t tag[pvault::tag_size];
        import_file.seek(file_size - pvault::tag_size);
        import_file.read(tag, pvault::tag_size);

        // check if key that we have can decrypt the imported vault. Otherwise there's no point doing it.
        bool decryption = pvault_cryptography::decrypt_verify(key, nonce, import_file, len, tag, 24);
        if(!decryption) { return false; }
        
        // overwrite vault with provided settings 
        header hdr{};

        hdr.magic[0] = 'C';
        hdr.magic[1] = 'R';
        hdr.magic[2] = 'Y';
        hdr.magic[3] = 'P';

        hdr.iterations = pvault::iterations;
        hdr.settings = settings;

        hdr.ciphertext_length = len;
        hdr.version = pvault::file_version;
        memcpy(hdr.master_salt, salt, pvault::salt_size);
        memcpy(hdr.nonce, nonce, pvault::nonce_size);

        if(SD.exists(path)) { SD.remove(path); }
        File new_file = SD.open(path, FILE_WRITE);
        
        pvault_cryptography::write_header(new_file, hdr);

        // copy ciphertext from old file to new
        uint32_t to_copy = len;    
        uint32_t copy_size;
        uint8_t copy_buffer[CHUNK_SIZE];

        import_file.seek(24);

        while(to_copy > 0) {
            if(to_copy > CHUNK_SIZE) {
                copy_size = CHUNK_SIZE;
            } else {
                copy_size = to_copy;
            }
        
            import_file.read(copy_buffer, copy_size);
            new_file.write(copy_buffer, copy_size);

            to_copy -= copy_size;
        }

        pvault_cryptography::write_tag(new_file, tag);

        import_file.close();
        new_file.close();

        // remove import file
        SD.remove(import_path);

        return true;
    }
}
