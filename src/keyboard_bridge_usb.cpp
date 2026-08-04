#include <Arduino.h>
#include "keyboard_bridge.h"
#include <USBHIDKeyboard.h>

USBHIDKeyboard Keyboard;

static uint8_t to_usb_code(special_key k) {
    switch(k) {
        case special_key::RETURN: return KEY_RETURN;
        case special_key::TAB:    return KEY_TAB;
        default:                  return KEY_TAB;
    }
}

void usb_keyboard_init() {
    Keyboard.begin();
}

void usb_send_key(special_key k) {
    Keyboard.press(to_usb_code(k));
    delay(25);
    Keyboard.releaseAll();
    delay(25);
}

void usb_send_key(char c) {
    Keyboard.press(c);
    delay(25);
    Keyboard.releaseAll();
    delay(25);
}
