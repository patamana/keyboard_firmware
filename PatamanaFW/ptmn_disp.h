#ifndef ptmn_disp_h
#define ptmn_disp_h

#include "ptmn_conf.h"
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SPIFFS.h>

#define MAX_KEY_DISP_BYTES 8
#define MAX_KEYMAP_SPRITES 8
#define MAX_MENU_PAGES 4
#define MAX_BODY_ROWS 3

#define FONT &fonts::efontJA_12
#define FONT_HIGHT 12
#define FONT_WIDTH 6

// oledの設定
class SSD1306_I2C : public lgfx::LGFX_Device {
    lgfx::Panel_SSD1306 _panel_instance;
    lgfx::Bus_I2C _bus_instance;

public:
    SSD1306_I2C();
};

class PtmnDisp {
public:
    void begin();
    void clear();

    void set_keymap(int keymap_num, char key_text[][MAX_KEY_DISP_BYTES], pos key_pos[], int num_keys);
    void display_keymap(int keymap_num);

    void init_menu();
    void set_menu_header(const char *text);
    void set_menu_row(int row_num, const char *text);
    int set_menu_text(int start_row_num, const char *text);
    void set_menu_footer(const char *text);
    void display_menu();
    void menu_select(int row_num);

    void print(const char *text);
    void print(const uint8_t *text);
    void print(String string);
    void print(const int num);
    void print(const uint8_t num);
    void println(const char *text);
    void println(const uint8_t *text);
    void println(String string);
    void println(const int num);
    void println(const uint8_t num);

    void print_word_pos(File file, const char *word);

private:
    SSD1306_I2C oled;
    LGFX_Sprite keymap_sprites[MAX_KEYMAP_SPRITES];
    LGFX_Sprite menu_header;
    LGFX_Sprite menu_body[MAX_MENU_PAGES];
    LGFX_Sprite menu_footer;
};

extern PtmnDisp ptmn_disp;

#endif