#include <Arduino.h>
#include <M5Cardputer.h>
#include "keyboard_bridge.h"
#include "gui.h"
#include "network_operations.h"
#include "asciimap.h"

#include <BleCompositeHID.h>
#include <KeyboardDevice.h>

BleCompositeHID compositeHID("Cardputer", "M5Stack", 100);
BLEHostConfiguration bleHostConfig;
KeyboardDevice* bleKeyboard;
extern bool network_initialized;
extern bool bluetooth_initialized;

static uint8_t to_ble_code(special_key k) {
    switch(k) {
        case special_key::RETURN: return KEY_ENTER;
        case special_key::TAB:    return KEY_TAB;
        default:                  return KEY_TAB;
    }
}

void ble_keyboard_init() {
    if(network_initialized) {
        connection_init_error();
        delay(3000);
        draw_ui();
        return;
    }

    bleHostConfig.setHidType(HID_KEYBOARD);

    bleKeyboard = new KeyboardDevice();
    compositeHID.addDevice(bleKeyboard);
    compositeHID.begin(bleHostConfig);

    bluetooth_initialized = true;
}

void ble_keyboard_update_battery_level() {
    compositeHID.setBatteryLevel(M5.Power.getBatteryLevel());
}

bool ble_keyboard_ready() {
    return compositeHID.isConnected();
}

void ble_send_key(special_key k) {
    bleKeyboard->keyPress(to_ble_code(k));
    delay(25);
    bleKeyboard->keyRelease(to_ble_code(k));
    delay(25);
}

void ble_send_key(char c) {
    uint8_t ascii = (uint8_t)c;
    if(ascii > 127) { return; }

    keymap map = ascii_to_hid[ascii];
    if(map.usage_id == 0) { return; }

    if (map.shift) {
        bleKeyboard->modifierKeyPress(KEY_MOD_LSHIFT);
    }
    
    bleKeyboard->keyPress(map.usage_id);
    delay(25);
    
    bleKeyboard->keyRelease(map.usage_id);
    delay(25);

    if (map.shift) {
        bleKeyboard->modifierKeyRelease(KEY_MOD_LSHIFT);
        delay(25);
    }
}
