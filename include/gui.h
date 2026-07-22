#pragma once
#include <M5GFX.h>

namespace pwder_style {
    struct color_scheme {
        int background_color;
        int foreground_color;

        int border_color;
        int statusbar_color;

        int background_text_color;
        int foreground_text_color;
    };

    struct icon32 {
        const uint8_t bitmap[3072];
        const bool mask[1024];
    };
}

const pwder_style::color_scheme pwcolors[1] = {
    {
        .background_color = NAVY,
        .foreground_color = LIGHTGREY,

        .border_color = DARKGREY,
        .statusbar_color = PURPLE,

        .background_text_color = BLACK,
        .foreground_text_color = WHITE
    }
};

void push_icon(const pwder_style::icon32 &icon, int xoffset, int yoffset, int scale);
void no_sdcard_crash_screen();
void password_import_screen();
void draw_ui();
