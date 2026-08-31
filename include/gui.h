#pragma once
#include <M5GFX.h>

namespace pwder_style {
    struct color_scheme {
        String name;

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

// spoiler alert
const String motd[32] PROGMEM = {
    "Papa Whiskey Delta echo romeo",
    "Don't give 'em the SSH keys!",
    "username: admin, password: admin",
    "Can a Cardputer run Forza Horizon?",
    "Pow-There!",
    "2+2=5, trust me bro",
    "Corny sunglasses, not gonna lie",
    "100% NO AI",
    "What is the definition of insanity?",
    "Click here to add text",
    "Password must be 8.5 characters long",
    "I'm sorry if it crashed",
    "sudo touch grass",
    "Now works with smart fridges",
    "PWDer Holy-C port, anyone?",
    "Manager, yet all passwords the same",
    "At least one person uses it",
    "I dare you to beatbox right now",
    "You picked the wrong Cardputer, fool",
    "encrypted = encrypt(decrypted);",
    "Hello Reddit!",
    "This line of text is called MOTD",
    "Make sure nobody's looking!",
    "Linux users when viewing file: ^._.^",
    "sudo rm -rf /",
    "PWDer in Germany: Der PWDer",
    "Don't lose it!",
    "ssh $USER@localhost",
    "I got a card, I got a puter...",
    "Also try geo-tp's Password Manager",
    "This is not Windows 95",
    "Probably doesn't beat Touch ID",
};

const int color_schemes_number = 7;
const pwder_style::color_scheme pwcolors[7] PROGMEM = {
    {
        .name = "Classic",

        .background_color = NAVY,
        .foreground_color = LIGHTGREY,

        .border_color = DARKGREY,
        .statusbar_color = PURPLE,

        .background_text_color = WHITE,
        .foreground_text_color = BLACK
    },
    {
        .name = "Classic Dark",

        .background_color = NAVY,
        .foreground_color = DARKGREY,

        .border_color = LIGHTGREY,
        .statusbar_color = PURPLE,

        .background_text_color = WHITE,
        .foreground_text_color = WHITE
    },
    {
        .name = "Industrial",

        .background_color = LIGHTGREY,
        .foreground_color = WHITE,
        
        .border_color = DARKGREY,
        .statusbar_color = BLACK,

        .background_text_color = BLACK,
        .foreground_text_color = BLACK
    },
    {
        .name = "Sky",

        .background_color = SKYBLUE,
        .foreground_color = WHITE,
        
        .border_color = LIGHTGREY,
        .statusbar_color = YELLOW,

        .background_text_color = BLACK,
        .foreground_text_color = BLACK
    },
    {
        .name = "Garden",

        .background_color = GREEN,
        .foreground_color = WHITE,
        
        .border_color = LIGHTGREY,
        .statusbar_color = SKYBLUE,

        .background_text_color = BLACK,
        .foreground_text_color = BLACK
    },
    {
        .name = "Elegance",

        .background_color = PURPLE,
        .foreground_color = LIGHTGREY,
        
        .border_color = WHITE,
        .statusbar_color = DARKGREY,

        .background_text_color = WHITE,
        .foreground_text_color = BLACK
    },
    {
        .name = "L33t H4xx0r",

        .background_color = BLACK,
        .foreground_color = BLACK,
        
        .border_color = DARKGREEN,
        .statusbar_color = GREEN,

        .background_text_color = GREEN,
        .foreground_text_color = GREEN
    }
};

#pragma pack(push, 1)
struct bmp_header {
    // bmp file
    uint16_t bf_type = 0x4D42;
    uint32_t bf_size;
    
    uint16_t bf_reserved_1 = 0;
    uint16_t bf_reserved_2 = 0;
    
    uint32_t bf_off_bits = 54;
    
    // bmp info
    uint32_t bi_size = 40;
    
    int32_t bi_width;
    int32_t bi_height;
    
    uint16_t bi_planes = 1;
    uint16_t bi_bit_count = 24;
    uint32_t bi_compression = 0;
    uint32_t bi_size_image = 0;
    
    int32_t bi_x_pels_per_meter = 0;
    int32_t bi_y_pels_per_meter = 0;

    uint32_t bi_clr_used = 0;
    uint32_t bi_clr_important = 0;
};
#pragma pack(pop)

void push_icon(const pwder_style::icon32 &icon, int xoffset, int yoffset, int scale);
void no_sdcard_crash_screen();
void password_import_screen();
void save_screenshot_bmp();
void connection_init_error();
void splash_screen();
void splash_screen_create_progressbar();
void splash_screen_update_progressbar_percentage(int progress);
void draw_ui();
