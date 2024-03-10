#define USE_NIMBLE
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <BleKeyboard.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <esp_now.h>

// シフトレジスタの制御pinの設定
const int LD = D1;
const int CK = D2;
const int QH = D3;

#define NUM_KEYS 30

// Active Low / High の設定
#define PUSHED LOW
#define RELEASED HIGH

#define MAX_COUNT 12

uint8_t left_keymap[NUM_KEYS] = {'5', '4', '3', '2', '1', '\0',
                                 '=', 't', 'r', 'e', 'w', 'q', KEY_ESC,
                                 '{', 'g', 'f', 'd', 's', 'a', KEY_TAB,
                                 '\\', 'b', 'v', 'c', 'x', 'z', '\0',
                                 KEY_LEFT_ALT, ' ', KEY_LEFT_SHIFT};

uint8_t right_keymap[NUM_KEYS] = {'6', '7', '8', '9', '0', '\0',
                                  '-', 'y', 'u', 'i', 'o', 'p', '\0',
                                  '}', 'h', 'j', 'k', 'l', ':', '\"',
                                  '`', 'n', 'm', '<', '>', '?', KEY_BACKSPACE,
                                  KEY_RIGHT_CTRL, KEY_RETURN, KEY_RIGHT_GUI};

typedef struct {
    int y;
    int x;
} position;

position symbol_pos[NUM_KEYS] = {
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
    {48, 80}};

char symbols[NUM_KEYS][4] = {
    "5", "4", "3", "2", "1", "",
    "=", "t", "r", "e", "w", "q", "es",
    "{", "g", "f", "d", "s", "a", "tb",
    "\\", "b", "v", "c", "x", "z", "",
    "Al", " ", "Sh"};

// Bluetooth設定
BleKeyboard bleKeyboard("patamana_keyboard"); // デバイスの名前

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

// espnowの送信先を格納
esp_now_peer_info_t peerInfo;

LGFX_SSD1306 display;

char key_state[NUM_KEYS];
char key_counter[NUM_KEYS];
char temp_key_state[NUM_KEYS];

char right_key_state[NUM_KEYS];

void OnDataRecv(const uint8_t *address, const uint8_t *data, int length) {
    for ( int i = 0 ; i < length ; i++ ) {
        Serial.print(data[i]);
        Serial.print(" ");
    }
    if (data[0] < 0x40) {
        bleKeyboard.release(right_keymap[data[0]]);
        right_key_state[data[0]] = RELEASED;
    } else if (data[0] < 0x80) {
        bleKeyboard.press(right_keymap[data[0] - 0x40]);
        right_key_state[data[0]] = PUSHED;
    } else if (data[0] == 0x80) {
        for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
            if (right_key_state[data[0]] == PUSHED) {
                bleKeyboard.release(right_keymap[data[0]]);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    // ESP-NOWの設定
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_now_init();
    esp_now_register_recv_cb(OnDataRecv); // ESP-NOWでデータ受信した時のコールバック関数を登録

    // ピン設定
    pinMode(LD, OUTPUT);
    pinMode(CK, OUTPUT);
    pinMode(QH, INPUT);
    digitalWrite(LD, HIGH);
    digitalWrite(CK, HIGH);

    bleKeyboard.begin();

    setup_oled();
}

void loop() {
    get_keys_from_SR();
    send_keys();
}

void get_keys_from_SR() {
    int key_num = 0;
    digitalWrite(LD, LOW);                       // LDがLOWのときシフトレジスタにロード
    digitalWrite(LD, HIGH);                      // ロード待機
    temp_key_state[key_num++] = digitalRead(QH); // 最初の信号がすでにQHにでている
    // Serial.print((int)temp_key_state[key_num]);
    while (key_num < NUM_KEYS) {
        digitalWrite(CK, LOW);
        digitalWrite(CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        temp_key_state[key_num++] = digitalRead(QH);
        // Serial.print((int)temp_key_state[key_num]);
    }
}

void send_keys() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (key_state[key_num] == PUSHED) {
            if (temp_key_state[key_num] == PUSHED) {
                if (key_counter[key_num] < MAX_COUNT) {
                    key_counter[key_num]++;
                }
            } else if (temp_key_state[key_num] == RELEASED) {
                if (key_counter[key_num] > 0) {
                    key_counter[key_num]--;
                } else {
                    key_state[key_num] = RELEASED;
                    bleKeyboard.release(left_keymap[key_num]);
                }
            }
        } else if (key_state[key_num] == RELEASED) {
            if (temp_key_state[key_num] == RELEASED) {
                if (key_counter[key_num] > 0) {
                    key_counter[key_num]--;
                }
            } else if (temp_key_state[key_num] == PUSHED) {
                if (key_counter[key_num] < MAX_COUNT) {
                    key_counter[key_num]++;
                } else {
                    key_state[key_num] = PUSHED;
                    bleKeyboard.press(left_keymap[key_num]);
                }
            }
        }
    }
}

void setup_oled() {
    display.init();            // 初期化
    display.setBrightness(64); // 明るさ 0~255
    display.setColorDepth(1);  // 階調bit数
    display.setCursor(0, 0);
    display.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    display.setFont(&fonts::efontJA_10);        // フォント設定
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        display.drawString(symbols[key_num], symbol_pos[key_num].x, symbol_pos[key_num].y);
    }
}
