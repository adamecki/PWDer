#include "sfx.h"
extern pvault::device_settings configuration;

namespace sfx {
    void beep() {
        if(configuration.speaker_on_auto_lock & 1) {
            M5Cardputer.Speaker.tone(8000, 20);
        }
    }

    void melody_c_major_scale() {
        if(configuration.speaker_on_auto_lock & 1) {
            M5Cardputer.Speaker.tone(261.63, 100);
            delay(100);

            M5Cardputer.Speaker.tone(293.66, 100);
            delay(100);

            M5Cardputer.Speaker.tone(329.63, 100);
            delay(100);

            M5Cardputer.Speaker.tone(349.23, 100);
            delay(100);

            M5Cardputer.Speaker.tone(392, 100);
            delay(100);

            M5Cardputer.Speaker.tone(440, 100);
            delay(100);

            M5Cardputer.Speaker.tone(493.88, 100);
            delay(100);

            M5Cardputer.Speaker.tone(523.25, 100);
            delay(100);
        }
    }
};
