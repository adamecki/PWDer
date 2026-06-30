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
#include <NTPClient.h>
#include <Unit_RTC.h>
#include <pvault.h>
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
#define VAULT_PATH "/pwder.pvault"
#define IMPORT_FILE_PATH "/pwimport"
#define EXPORT_FILE_PATH "/pwexport"

// default configuration values
#define DEFAULT_PASSWORD "default"
#define DEFAULT_SSID "sample"
#define DEFAULT_WIFI_PASSWORD "password"
#define DEFAULT_SYNCHOST "192.168.1.100"
#define DEFAULT_SYNCPORT "7305"

// UI definitions
#define UI_UPDATE_MILISECONDS 3000
#define MODE3_PAGES_NUMBER 9
