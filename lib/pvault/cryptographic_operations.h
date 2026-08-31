#pragma once

#include <Arduino.h>
#include <SD.h>

#include "pvault.h"

#define CHUNK_SIZE 1024 // bytes

namespace pvault_cryptography {
    // generate values
    bool generate_salt(uint8_t* salt);
    bool generate_nonce(uint8_t* nonce);

    // cryptography
    bool derive_key(const String& password, const uint8_t* salt, uint8_t* key); // Derive key from password

    bool decrypt_verify( // lightweight function to check if used key works with the ciphertext
        const uint8_t* key,
        const uint8_t* nonce,
        File& file,
        const uint32_t ciphertext_length,
        const uint8_t* tag,
        uint32_t header_size
    );

    bool encrypt_chunked( // writes ciphertext + tag to provided file
        const uint8_t* key,
        const uint8_t* nonce,
        const uint8_t* plaintext,
        uint32_t plaintext_length,
        File& file
    );

    bool decrypt_chunked( // reads tag itself + plaintext to provided pointer
        const uint8_t* key,
        const uint8_t* nonce,
        File& file,
        uint32_t ciphertext_length,
        uint8_t* plaintext
    );

    // vault file structure
    bool write_header(File& file, const pvault::header& hdr);
    bool read_header(File& file, pvault::header& hdr); // assumes that file pointer is set at the beginning of the file
    
    bool write_tag(File& file, const uint8_t* tag);
    bool read_tag(File& file, uint8_t* tag); // assumes that file pointer is set on tag's location

    // memory security
    void secure_zero(void* ptr, uint32_t size);
}
