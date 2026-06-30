#pragma once

#include <Arduino.h>

// PWDer vault file format 1.0
// and functions for its operation
// let's see

namespace pvault {
    constexpr uint8_t   magic_size      = 4;
    constexpr uint8_t   file_version    = 1;

    constexpr uint32_t  iterations      = 10000;
    constexpr uint8_t   salt_size       = 16;
    constexpr uint8_t   nonce_size      = 12;

    constexpr uint8_t   key_size        = 16;
    constexpr uint8_t   tag_size        = 16;

    constexpr uint16_t  max_entries     = 101; // first is used for configuration
    constexpr uint16_t  max_length      = 128;

    #pragma pack(push, 1)
    // encrypted data
    struct credential {
        char title[max_length];
        char username[max_length];
        char password[max_length];
        char totp_secret[max_length];
    };

    struct vault {
        uint16_t credential_count;
        credential credentials[max_entries];
    };

    // unencrypted settings in header
    struct device_settings {
        uint8_t speaker_on;
        uint8_t input_mode;
        uint8_t wifi_timeout;
        uint8_t color_scheme;
    };

    // file header
    struct header {
        char magic[magic_size];
        uint8_t version;

        uint32_t iterations;
        uint8_t master_salt[salt_size];
        uint8_t nonce[nonce_size];

        device_settings settings;

        uint32_t ciphertext_length;
    };
    #pragma pack(pop)

    // surface-level functions
    bool get_key( // returns false if password is incorrect
        const char* path,
        const String& password,
        uint8_t* key
    );

    bool init_vault( // create new vault using provided password
        const char* path,
        const String& password,
        const device_settings& settings,
        const vault& data
    );
    bool update_vault( // update vault data with new information
        const char* path,
        const uint8_t* key,
        const device_settings& settings,
        const vault& data
    );
    bool load_vault( // load existing vault to memory
        const char* path,
        const uint8_t* key,
        device_settings& settings,
        vault& data
    );

    bool update_config( // change unencrypted config without touching the vault data
        const char* path,
        const device_settings& settings
    );
    bool read_config( // read config without need to decrypt the entire file
        const char* path,
        device_settings& settings
    );
}
