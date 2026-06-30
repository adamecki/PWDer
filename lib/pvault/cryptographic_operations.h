#pragma once

#include <Arduino.h>
#include <SD.h>

#include "pvault.h"

namespace pvault_cryptography {
    // cryptographic operations
    bool generate_salt(uint8_t* salt);
    bool generate_nonce(uint8_t* nonce);
    bool derive_key(const String& password, const uint8_t* salt, uint8_t* key);
    
    bool encrypt_gcm(
        const uint8_t* key,
        const uint8_t* nonce,
        const uint8_t* plaintext,
        uint32_t plaintext_length,
        uint8_t* ciphertext,
        uint8_t* tag
    );
    bool decrypt_gcm(
        const uint8_t* key,
        const uint8_t* nonce,
        const uint8_t* ciphertext,
        uint32_t ciphertext_length,
        uint8_t* plaintext,
        const uint8_t* tag
    );

    // filesystem operations
    bool write_header(File& file, const pvault::header& hdr);
    bool read_header(File& file, pvault::header& hdr);
    bool write_tag(File& file, const uint8_t* tag);
    bool read_tag(File& file, uint8_t* tag);

    // memory security
    void secure_zero(void* ptr, uint32_t size);
}
