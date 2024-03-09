/*
    パタマナキーボードの導通確認用のファームウェアです。
    押されたキーの番号をOLED上に表示します。
    キーを押したのに番号が表示されない場合は導通していません。
*/
#include <LovyanGFX.hpp>

// チャタリング調整
#define MAX_COUNT 7

// キー数の設定
#define NUM_KEYS 32

// Active Low / High の設定
#define PRESSED LOW
#define RELEASED HIGH

// シフトレジスタの制御pinの設定
const int LD = D1;
const int CK = D2;
const int QH = D3;

// OLEDの設定
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
                                                    // 基板に書かれた0x78はLSBにR/Wビットがふくまれているので1ビット右にシフトした値を指定する
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

#define FONT &fonts::efontJA_10
#define FONT_HIGHT 10
#define FONT_WIDTH 6

char key_state[NUM_KEYS];
char temp_key_state[NUM_KEYS];
char pressed_counts[NUM_KEYS];
LGFX_SSD1306 display;

// 起動後、setup()が一度実行される
void setup() {
    // usbシリアル設定
    Serial.begin(115200);
    // ピン設定
    pinMode(LD, OUTPUT);
    pinMode(CK, OUTPUT);
    pinMode(QH, INPUT);
    digitalWrite(LD, HIGH);
    digitalWrite(CK, HIGH);
    // OLEDのセットアップ
    setup_oled();
}

// setup()実行後、loop()の実行が繰り返される
void loop() {
    get_keys_from_SR();
    count_pressed();
    judge_key_state();
}

// シフトレジスタからキーを読み取る
void get_keys_from_SR() {
    int key_num = 0;
    digitalWrite(LD, LOW);                       // LDがLOWのときシフトレジスタにロード
    digitalWrite(LD, HIGH);                      // ロード待機
    temp_key_state[key_num++] = digitalRead(QH); // 最初の信号がすでにQHにでている
    // Serial.print((int)temp_key_state[key_num - 1]);
    while (key_num < NUM_KEYS) {
        digitalWrite(CK, LOW);
        digitalWrite(CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        temp_key_state[key_num++] = digitalRead(QH);
        // Serial.print((int)temp_key_state[key_num - 1]);
    }
}

// チャタリング用のカウントをする
void count_pressed() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (temp_key_state[key_num] == PRESSED) {
            if (pressed_counts[key_num] < MAX_COUNT) {
                pressed_counts[key_num]++;
            }
        } else {
            if (pressed_counts[key_num] > 0) {
                pressed_counts[key_num]--;
            }
        }
    }
}

// 最終的なキーの状態を判断する
void judge_key_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (key_state[key_num] == RELEASED) {
            if (pressed_counts[key_num] == MAX_COUNT) {
                key_state[key_num] = PRESSED;
                printCLI(key_num); printCLI("\n");
                Serial.println(key_num);
            }
        } else {
            if (pressed_counts[key_num] == 0) {
                key_state[key_num] = RELEASED;
            }
        }
    }
}

// OLEDのセットアップ
void setup_oled() {
    display.init();            // 初期化
    display.setBrightness(64); // 明るさ 0~255
    display.setColorDepth(1);  // 階調bit数
    display.setCursor(0, 0);
    display.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    display.setFont(FONT);                      // フォント設定
}

// コマンドライン表示
void printCLI(const char *text) {
    int i = 0;
    while (text[i] != '\0') {
        int right_margin = display.width() - display.getCursorX();
        if (right_margin < FONT_WIDTH && text[i] != '\n') {
            display.print("\n");
        }
        int bottom_margin = display.height() - display.getCursorY();
        if (bottom_margin < FONT_HIGHT) {
            display.scroll(0, bottom_margin - FONT_HIGHT);
            display.setCursor(0, display.height() - FONT_HIGHT);
        }
        display.print(text[i]);
        i++;
    }
}
void printCLI(const uint8_t *text) {
    int i = 0;
    while (text[i] != '\0') {
        printCLI(text[i]);
        i++;
    }
}
void printCLI(const uint8_t text) {
    int right_margin = display.width() - display.getCursorX();
    if (right_margin < FONT_WIDTH && text != '\n') {
        display.print("\n");
    }
    int bottom_margin = display.height() - display.getCursorY();
    if (bottom_margin < FONT_HIGHT) {
        display.scroll(0, bottom_margin - FONT_HIGHT);
        display.setCursor(0, display.height() - FONT_HIGHT);
    }
    display.print(text);
}
void printCLI(const int num) {
    char text[11];
    sprintf(text, "%d", num);
    printCLI(text);
}
