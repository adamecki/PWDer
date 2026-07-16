#include <Arduino.h>
#include "keyboard_bridge.h"
#include <BleKeyboard.h>

BleKeyboard bleKeyboard;

static uint8_t to_ble_code(special_key k) {
    switch(k) {
        case special_key::RETURN: return KEY_RETURN;
        case special_key::TAB:    return KEY_TAB;
    }
}

void ble_keyboard_init() {
    bleKeyboard.begin();
}

void ble_keyboard_end() {
    bleKeyboard.end();
}

bool ble_keyboard_ready() {
    return bleKeyboard.isConnected();
}

void ble_send_key(special_key k) {
    bleKeyboard.press(to_ble_code(k));
    delay(25);
    bleKeyboard.releaseAll();
    delay(25);
}

void ble_send_key(char c) {
    bleKeyboard.press(c);
    delay(25);
    bleKeyboard.releaseAll();
    delay(25);
}
