#ifndef ssk_settings_h
#define ssk_settings_h

#include "Arduino.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "LovyanGFX.hpp"

// 片側のキースイッチの数
#define NUM_KEYS 30

// チャタリング調整
#define MAX_COUNT 7

// Active Low / High の設定
#define PRESSED LOW
#define RELEASED HIGH

// シフトレジスタの制御pinの設定
#define SR_LD D1
#define SR_CK D2
#define SR_QH D3

#define LEFT 0
#define RIGHT 1

#define FONT &fonts::efontJA_10
#define FONT_HIGHT 10
#define FONT_WIDTH 6

#define MAX_ALL_KEYS_RELEASED_COUNT 10000

// 左右通信のコード設定
#define CODE_RELEASED 0x40
#define CODE_ALL_KEYS_RELEASED 0x80

typedef struct {
    int x;
    int y;
} pos;

extern uint8_t left_phisical_layout[NUM_KEYS];
extern uint8_t right_phisical_layout[NUM_KEYS];
extern uint8_t left_keymap[NUM_KEYS];
extern uint8_t right_keymap[NUM_KEYS];
extern char left_keymap_display[NUM_KEYS][4];
extern char right_keymap_display[NUM_KEYS][4];
extern pos left_key_disp_pos[NUM_KEYS];
extern pos right_key_disp_pos[NUM_KEYS];

// oledの設定
class SSD1306_I2C : public lgfx::LGFX_Device {
    lgfx::Panel_SSD1306 _panel_instance;
    lgfx::Bus_I2C _bus_instance;
    public:
        SSD1306_I2C();
};

#endif