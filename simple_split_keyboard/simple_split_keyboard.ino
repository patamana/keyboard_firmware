#include "ssk_settings.h"

#define THIS_SIDE RIGHT

USBHIDKeyboard keyboard;
LGFX_SSD1306 display;

char key_state[NUM_KEYS];
char pressed_counts[NUM_KEYS];
char temp_key_state[NUM_KEYS];
char opposite_key_state[NUM_KEYS];

void setup() {
    Serial1.setRxBufferSize(256);
    Serial1.setTxBufferSize(256);
    Serial1.begin(115200, SERIAL_8N1, RX, TX);

    pinMode(LD, OUTPUT);
    pinMode(CK, OUTPUT);
    pinMode(QH, INPUT);
    digitalWrite(LD, HIGH);
    digitalWrite(CK, HIGH);

    setup_oled();

    init_key_state();
    arrenge_keymap();
    
    keyboard.begin();
    USB.begin();
}

void loop() {
    get_keys_from_SR();
    count_pressed();
    judge_key_state();
    receive_code();
}

void init_key_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        key_state[key_num] = RELEASED;
    }
}

// 配線に合わせてキーマップを調整する
void arrenge_keymap(){
    uint8_t temp_left_keymap[NUM_KEYS];
    uint8_t temp_right_keymap[NUM_KEYS];
    for (int key_num = 0; key_num < NUM_KEYS; key_num++){
        temp_left_keymap[left_phisical_layout[key_num]] = left_keymap[key_num];
        temp_right_keymap[right_phisical_layout[key_num]] = right_keymap[key_num];
    }
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        left_keymap[key_num] = temp_left_keymap[key_num];
        right_keymap[key_num] = temp_right_keymap[key_num];
    }
}

// シフトレジスタからキーを読み取る
void get_keys_from_SR() {
    int key_num = 0;
    digitalWrite(LD, LOW);                       // LDがLOWのときシフトレジスタにロード
    digitalWrite(LD, HIGH);                      // ロード待機
    temp_key_state[key_num++] = digitalRead(QH); // 最初の信号がすでにQHにでている
    while (key_num < NUM_KEYS) {
        digitalWrite(CK, LOW);
        digitalWrite(CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        temp_key_state[key_num++] = digitalRead(QH);
    }
}

// チャタリング用のカウントをする
void count_pressed() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (temp_key_state[key_num] == PRESSED && pressed_counts[key_num] < MAX_COUNT) {
            pressed_counts[key_num]++;
        } else if (temp_key_state[key_num] == RELEASED && pressed_counts[key_num] > 0) {
            pressed_counts[key_num]--;
        }
    }
}

// 最終的なキーの状態を判断する
void judge_key_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (key_state[key_num] == RELEASED && pressed_counts[key_num] == MAX_COUNT) {
            key_state[key_num] = PRESSED;
            if (THIS_SIDE == LEFT) {
                keyboard.press(left_keymap[key_num]);
            } else {
                keyboard.press(right_keymap[key_num]);
            }
            Serial1.write(key_num);
        } else if (key_state[key_num] == PRESSED && pressed_counts[key_num] == 0) {
            key_state[key_num] = RELEASED;
            if (THIS_SIDE == LEFT) {
                keyboard.release(left_keymap[key_num]);
            } else {
                keyboard.release(right_keymap[key_num]);
            }
            Serial1.write(key_num + 0x80);
        }
    }
}

void receive_code() {
    int code = 0;
    while (Serial1.available()) {
        code = Serial1.read();
        if(code < 0x80){
            if (THIS_SIDE == LEFT) {
                keyboard.press(right_keymap[code]);
            } else {
                keyboard.press(left_keymap[code]);
            }
        } else {
            if (THIS_SIDE == LEFT) {
                keyboard.release(right_keymap[code - 0x80]);
            } else {
                keyboard.release(left_keymap[code - 0x80]);
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
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if(THIS_SIDE == LEFT){
            display.drawString(left_keymap_display[key_num], left_display_pos[key_num].x, left_display_pos[key_num].y);
        } else {
            display.drawString(right_keymap_display[key_num], right_display_pos[key_num].x, right_display_pos[key_num].y);
        }
    }
}

