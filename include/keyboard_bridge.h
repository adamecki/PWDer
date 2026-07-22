#pragma once
enum class special_key {
    RETURN, TAB
};

void usb_keyboard_init();
void usb_send_key(special_key k);
void usb_send_key(char c);

void ble_keyboard_init();
bool ble_keyboard_ready();
void ble_send_key(special_key k);
void ble_send_key(char c);
