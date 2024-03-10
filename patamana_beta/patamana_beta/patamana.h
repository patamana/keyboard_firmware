#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "patamana_control_code.h"
#include <BleKeyboard.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>

#ifndef patamana_h
#define patamana_h

// キー数の設定
#define NUM_KEYS 64      // 左右合計でのキーの数  通信規格上の上限は左右64ずつで合計128
#define NUM_LEFT_KEYS 32 // 左側のキーの数  通信規格上の上限は64

// Active Low / High の設定
#define PUSHED LOW
#define RELEASED HIGH

// 上限の設定
#define MAX_LAYERS 16         // キーマップに登録できるレイヤーの最大数
#define MAX_MODIFIER_KEYS 8   // レイヤー変換に使う修飾キーの最大数
#define MAX_SEND_KEYS 64      // 送るキーコードの最大数
#define MAX_WORD_LENGTH 66    // キーマップファイルの解析における最大語長
#define TIMEOUT 10000         // uartでの受信待ちのタイムアウト時間(ms)
#define MAX_RETRANSMIT 4      // uartでのパケット再送信の上限回数
#define MAX_SPRITES 4         // oledに表示できるレイヤーの数
#define MAX_FILE_PATH_NAME 64 // ファイルパスの上限文字数

// oledの設定
class LGFX_SSD1306 : public lgfx::LGFX_Device {
    lgfx::Panel_SSD1306 _panel_instance;
    lgfx::Bus_I2C _bus_instance;

public:
    LGFX_SSD1306(void) {
        {
            auto cfg = _bus_instance.config();      // バス設定用の構造体を取得
            cfg.i2c_port = 0;                       // 使用するI2Cポートを選択 (0 or 1)
            cfg.freq_write = 800000;                // 送信時のクロック
            cfg.freq_read = 800000;                 // 受信時のクロック
            cfg.pin_sda = SDA;                      // SDAを接続しているピン番号
            cfg.pin_scl = SCL;                      // SCLを接続しているピン番号
            cfg.i2c_addr = 0x3C;                    // I2Cデバイスのアドレス
                                                    // 基板に書かれた0x78はLSBにR/Wビットがふくまれているので1ビット右にシフトした値
            _bus_instance.config(cfg);              // 設定値を反映
            _panel_instance.setBus(&_bus_instance); // バスをパネルにセット
        }
        {                                        // 表示パネル制御の設定
            auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得
            cfg.panel_width = 128;               // 実際に表示可能な幅
            cfg.panel_height = 64;               // 実際に表示可能な高さ
            cfg.offset_x = 0;                    // パネルのX方向オフセット量
            cfg.offset_y = 0;                    // パネルのY方向オフセット量
            cfg.offset_rotation = 2;             // 回転方向の値のオフセット 0~7 (4~7は上下反転)

            _panel_instance.config(cfg); // 設定値を反映
        }
        setPanel(&_panel_instance); // 使用するパネルをセット
    }
};

// 構造体定義
typedef struct {
    char keymap_file_path[MAX_FILE_PATH_NAME];
} patamana_setting;

typedef struct {
    int num_modifier_keys;
    uint8_t modifier_keys[MAX_MODIFIER_KEYS]; // ここに登録されたキーが全て押されたレイヤーが適用される
    uint8_t keys[NUM_KEYS][MAX_SEND_KEYS];    // 1次元目がキー番号、2次元目が送信するキーコード
} layer;

typedef struct {
    char word[16];
    uint8_t code;
} reserved_word;

// 予約語の設定
reserved_word reserved_words[] = {
    {"shift",           0x81},
    {"shift_left",      0x81},
    {"shift_right",     0x85},
    {"option",          0x82},
    {"option_left",     0x82},
    {"option_right",    0x86},
    {"command",         0x83},
    {"command_left",    0x83},
    {"command_right",   0x87},
    {"control",         0x80},
    {"control_left",    0x80},
    {"control_right",   0x84},
    {"alt",             0x82},
    {"alt_left",        0x82},
    {"alt_right",       0x86},
    {"cmd",             0x83},
    {"cmd_left",        0x83},
    {"cmd_right",       0x87},
    {"gui",             0x83},
    {"gui_left",        0x83},
    {"gui_right",       0x87},
    {"ctrl",            0x80},
    {"ctrl_left",       0x80},
    {"ctrl_right",      0x84},
    {"space",           0x20},
    {"tab",             0xb3},
    {"return",          0xb0},
    {"enter",           0xb0},
    {"delete",          0xb2},
    {"delete_forward",  0xd4},
    {"insert",          0xd1},
    {"top",             0xd2},
    {"end",             0xd5},
    {"page_up",         0xd3},
    {"page_down",       0xd6},
    {"up_arrow",        0xda},
    {"down_arrow",      0xd9},
    {"left_arrow",      0xd8},
    {"right_arrow",     0xd7},
    {"esc",             0xb1},
    {"f1",              0xc2},
    {"f2",              0xc3},
    {"f3",              0xc4},
    {"f4",              0xc5},
    {"f5",              0xc6},
    {"f6",              0xc7},
    {"f7",              0xc8},
    {"f8",              0xc9},
    {"f9",              0xca},
    {"f10",             0xcb},
    {"f11",             0xcc},
    {"f12",             0xcd},
    {"print_screen",    0xc2},
    {"scroll_lock",     0xcf},
    {"pause",           0xd0}
};

typedef struct {
    char symbol[8];
    uint8_t code;
} display_symbol;

display_symbol display_symbols[] = {
    {"⇧",   0x81},
    {"⌥",   0x82},
    {"⌘",   0x83},
    {"^",   0x80},
    {"↩︎",   0xb0},
    {"⌫",   0xd4},
    {"⌦",   0xb2},
    {"␣",   0x20},
    {"⇥",   0xb3},
    {"↑",   0xda},
    {"↓",   0xd9},
    {"←	",  0xd8},
    {"→	",  0xd7},
    {"↖",   0xd2},
    {"↘",   0xd5},
    {"esc",  0xb1},
    {"f1",   0xc2},
    {"f2",   0xc3},
    {"f3",   0xc4},
    {"f4",   0xc5},
    {"f5",   0xc6},
    {"f6",   0xc7},
    {"f7",   0xc8},
    {"f8",   0xc9},
    {"f9",   0xca}
};

typedef struct{
    int y;
    int x;
} position;

position symbol_pos[NUM_KEYS] = {
    // LEFT
    {0, 80},
    {0, 64},
    {0, 48},
    {0, 32},
    {0, 16},
    {0, 0},
    {12, 96},
    {12, 80},
    {12, 64},
    {12, 48},
    {12, 32},
    {12, 16},
    {12, 0},
    {24, 96},
    {24, 80},
    {24, 64},
    {24, 48},
    {24, 32},
    {24, 16},
    {24, 0},
    {36, 96},
    {36, 80},
    {36, 64},
    {36, 48},
    {36, 32},
    {36, 16},
    {36, 0},
    {48, 112},
    {48, 96},
    {48, 80},
    {0, 0}, // 制御ボタン
    {0, 0}, // 制御ボタン
    // Right
    {0, 32},
    {0, 48},
    {0, 64},
    {0, 80},
    {0, 96},
    {0, 112},
    {12, 16},
    {12, 32},
    {12, 48},
    {12, 64},
    {12, 80},
    {12, 96},
    {12, 112},
    {24, 16},
    {24, 32},
    {24, 48},
    {24, 64},
    {24, 80},
    {24, 96},
    {24, 112},
    {36, 16},
    {36, 32},
    {36, 48},
    {36, 64},
    {36, 80},
    {36, 96},
    {36, 112},
    {48, 0},
    {48, 16},
    {48, 32},
    {0, 0}, // 制御ボタン
    {0, 0}  // 制御ボタン
};

#define LEFT 0
#define RIGHT 1

// 左右通信コード規格
#define CODE_RELEASED 0x00
#define CODE_PUSHED 0x40

#define CODE_ALL_KEYS_RELEASED 0x80
#define CODE_TRANSMIT_KEYMAP 0x81
#define CODE_SUCCESS 0x82
#define CODE_FAILURE 0x83
#define CODE_REQUEST_KEYMAP 0x81

#endif