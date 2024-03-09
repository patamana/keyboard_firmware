#include "USB.h"
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

// 左側のMACアドレスを設定
uint8_t address[] = {0x30, 0x30, 0xF9, 0x18, 0x26, 0x50};

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
int key_counter[NUM_KEYS];
char temp_key_state[NUM_KEYS];

typedef struct {
    int y;
    int x;
} position;

position symbol_pos[NUM_KEYS] = {
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
    {48, 32}};

char symbols[NUM_KEYS][4] = {
    "6", "7", "8", "9", "0", "",
    "-", "y", "u", "i", "o", "p", "",
    "}", "h", "j", "k", "l", ":", "\"",
    "`", "n", "m", "<", ">", "?", "BS",
    "Ct", "En", "Cm"};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Deli_Success" : "Deli_Fail");
}

void setup() {
    Serial.begin(115200);
    // ESP-NOW設定
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_now_init();
    memcpy(peerInfo.peer_addr, address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    esp_now_register_send_cb(OnDataSent);

    // ピン設定
    pinMode(LD, OUTPUT);
    pinMode(CK, OUTPUT);
    pinMode(QH, INPUT);
    digitalWrite(LD, HIGH);
    digitalWrite(CK, HIGH);

    setup_oled();
}

void loop() {
    get_keys_from_SR();
    send_keys_to_left();
}

void get_keys_from_SR() {
    uint8_t key_num = 0;
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

void send_keys_to_left() {
    for (uint8_t key_num = 0; key_num < NUM_KEYS; key_num++) {
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
                    uint8_t code = key_num;
                    esp_err_t result = esp_now_send(address, &code, sizeof(code));
                    while(result != ESP_OK){
                        result = esp_now_send(address, &code, sizeof(code));
                    }
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
                    uint8_t code = key_num + 0x40;
                    esp_err_t result = esp_now_send(address, &code, sizeof(code));
                    while(result != ESP_OK){
                        result = esp_now_send(address, &code, sizeof(code));
                    }
                    Serial.println("send");
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
