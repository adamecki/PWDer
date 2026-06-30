#include "cryptographic_operations.h"

#include <cstring>
#include <mbedtls/md.h>
#include <mbedtls/gcm.h>
#include <mbedtls/pkcs5.h>
#include <esp_system.h>

namespace pvault_cryptography {
    bool generate_salt(uint8_t* salt) {
        if(salt == nullptr) { return false; }
        esp_fill_random(salt, pvault::salt_size);
        return true;
    }

    bool generate_nonce(uint8_t* nonce) {
        if(nonce == nullptr) { return false; }
        esp_fill_random(nonce, pvault::nonce_size);
        return true;
    }

    bool derive_key(const String& password, const uint8_t* salt, uint8_t* key) {
        // argument validation
        if(salt == nullptr || key == nullptr) { return false; }

        // init ESP32 cryptography
        mbedtls_md_context_t md_ctx;
        mbedtls_md_init(&md_ctx);

        // SHA256 algorithm descriptions
        const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if(md_info == nullptr) { return false; }

        // create HMAC context
        int result = mbedtls_md_setup(&md_ctx, md_info, 1);
        if(result != 0) {
            mbedtls_md_free(&md_ctx);
            return false;
        }

        // generate AES128 key from provided password
        result = mbedtls_pkcs5_pbkdf2_hmac(
            &md_ctx,
            reinterpret_cast<const unsigned char*>(password.c_str()),
            password.length(),
            salt,
            pvault::salt_size,
            pvault::iterations,
            pvault::key_size,
            key
        );

        // free HMAC context
        mbedtls_md_free(&md_ctx);

        if(result != 0) {
            secure_zero(key, pvault::key_size);
            return false;
        }

        return true;
    }

    bool encrypt_gcm(
        const uint8_t* key,
        const uint8_t* nonce,
        const uint8_t* plaintext,
        uint32_t plaintext_length,
        uint8_t* ciphertext,
        uint8_t* tag
    ) {
        // argument validation
        if(!key || !nonce || !plaintext || !ciphertext || !tag) { return false; }

        // GCM init
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);

        int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
        if(ret != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }

        // encryption
        ret = mbedtls_gcm_crypt_and_tag(
            &gcm,
            MBEDTLS_GCM_ENCRYPT,
            plaintext_length,
            nonce,
            pvault::nonce_size,
            nullptr,
            0,
            plaintext,
            ciphertext,
            pvault::tag_size,
            tag
        );

        mbedtls_gcm_free(&gcm);
        if(ret != 0) {
            secure_zero(ciphertext, plaintext_length);
            secure_zero(tag, pvault::tag_size);

            return false;
        }

        return true;
    }

    bool decrypt_gcm(
        const uint8_t* key,
        const uint8_t* nonce,
        const uint8_t* ciphertext,
        uint32_t ciphertext_length,
        uint8_t* plaintext,
        const uint8_t* tag
    ) {
        // argument validation
        if(!key || !nonce || !ciphertext || !tag || !plaintext) { return false; }

        // GCM init
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);

        int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
        if(ret != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }

        // verify integrity and decrypt
        ret = mbedtls_gcm_auth_decrypt(
            &gcm,
            ciphertext_length,
            nonce,
            pvault::nonce_size,
            nullptr,
            0,
            tag,
            pvault::tag_size,
            ciphertext,
            plaintext
        );

        mbedtls_gcm_free(&gcm);
        if(ret != 0) {
            // integrity verification failed
            secure_zero(plaintext, ciphertext_length);
            return false;
        }

        return true;
    }

    bool write_header(File& file, const pvault::header& hdr) {
        // file validation
        if(!file) { return false; }

        // write header
        uint32_t written = file.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(pvault::header));

        // validate written data
        if(written != sizeof(pvault::header)) { return false; }

        return true;
    }

    bool read_header(File& file, pvault::header& hdr) {
        // file validation
        if(!file) { return false; }

        // read binary header
        uint32_t read_bytes = file.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(pvault::header));

        // validate read data
        if(read_bytes != sizeof(pvault::header)) { return false; }
        
        return true;
    }

    bool write_tag(File& file, const uint8_t* tag) {
        // file and tag validation
        if(!file || !tag) { return false; }

        // write binary tag
        uint32_t written = file.write(tag, pvault::tag_size);

        // validate written data
        if(written != pvault::tag_size) { return false; }
        
        return true;
    }
    
    bool read_tag(File& file, uint8_t* tag) {
        // file and tag validation
        if(!file || !tag) { return false; }

        // read binary tag
        uint32_t read_bytes = file.read(tag, pvault::tag_size);

        // validate read data
        if(read_bytes != pvault::tag_size) { return false; }

        return true;
    }

    void secure_zero(void* ptr, uint32_t size) {
        volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(ptr);

        while(size--) {
            *p++ = 0;
        }
    }
}
