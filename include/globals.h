#pragma once

// Device libraries
#include <M5Cardputer.h>
#include <M5GFX.h>
#include <SD.h>
#include <SPI.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <WiFi.h>

// Software libraries
#include "TOTP.h"
#include <Arduino.h>
#include <Base32-Decode.h>
#include <Cipher.h>
#include <NTPClient.h>
#include <Unit_RTC.h>
#include <cstdlib>
#include <time.h>

// Language
#define lang_en
// #define lang_pl
#include "ui_strings.h"

// SD support definitions
#define SD_CS_PIN 12
#define SD_MOSI_PIN 14
#define SD_MISO_PIN 39
#define SD_CLK_PIN 40

// File paths
#define PWDER_DIR_PATH "/pwder"
#define SPKSTATE_FILE_PATH "/pwder/spkstate"
#define CONFIG_FILE_PATH "/pwder/config"
#define SECRET_FILE_PATH "/pwder/secret"
#define IMPORT_FILE_PATH "/pwimport"
#define EXPORT_FILE_PATH "/pwexport"

// UI definitions
#define UI_UPDATE_MILISECONDS 3000
#define MODE3_PAGES_NUMBER 9
