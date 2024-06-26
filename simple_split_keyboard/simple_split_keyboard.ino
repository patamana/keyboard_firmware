#include "ssk_settings.h"

#define THIS_SIDE RIGHT

USBHIDKeyboard keyboard;
SSD1306_I2C display;

char key_state[NUM_KEYS];
char pressed_counts[NUM_KEYS];
char temp_key_state[NUM_KEYS];
char opposite_key_state[NUM_KEYS];

bool is_all_opposite_keys_released = true;

void setup() {
    Serial1.setRxBufferSize(256);
    Serial1.setTxBufferSize(256);
    Serial1.begin(115200, SERIAL_8N1, RX, TX);

    pinMode(SR_LD, OUTPUT);
    pinMode(SR_CK, OUTPUT);
    pinMode(SR_QH, INPUT);
    digitalWrite(SR_LD, HIGH);
    digitalWrite(SR_CK, HIGH);
    
    setup_oled();

    init_key_state();
    arrenge_keymap();

    display_keymap();

    keyboard.begin();
    USB.begin();
}

void loop() {
    get_keys_from_SR();
    count_pressed();
    judge_key_state();
    transmit_all_keys_released();
    receive_code();
}

// OLEDのセットアップ
void setup_oled() {
    display.init();            // 初期化
    display.setBrightness(64); // 明るさ 0~255
    display.setColorDepth(1);  // 階調bit数
    display.setCursor(0, 0);
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    display.setFont(FONT);                      // フォント設定
}

void init_key_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        key_state[key_num] = RELEASED;
        opposite_key_state[key_num] = RELEASED;
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

// OLEDにキーマップを表示
void display_keymap() {
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (THIS_SIDE == LEFT) {
            display.drawString(left_keymap_display[key_num], left_key_disp_pos[key_num].x, left_key_disp_pos[key_num].y);
        } else {
            display.drawString(right_keymap_display[key_num], right_key_disp_pos[key_num].x, right_key_disp_pos[key_num].y);
        }
    }
}

// シフトレジスタからキーを読み取る
void get_keys_from_SR() {
    int key_num = 0;
    digitalWrite(SR_LD, LOW);                       // LDがLOWのときシフトレジスタにロード
    digitalWrite(SR_LD, HIGH);                      // ロード待機
    temp_key_state[key_num++] = digitalRead(SR_QH); // 最初の信号がすでにQHにでている
    while (key_num < NUM_KEYS) {
        digitalWrite(SR_CK, LOW);
        digitalWrite(SR_CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        temp_key_state[key_num++] = digitalRead(SR_QH);
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
            delay(1);
        } else if (key_state[key_num] == PRESSED && pressed_counts[key_num] == 0) {
            key_state[key_num] = RELEASED;
            if (THIS_SIDE == LEFT) {
                keyboard.release(left_keymap[key_num]);
            } else {
                keyboard.release(right_keymap[key_num]);
            }
            Serial1.write(key_num + CODE_RELEASED);
            delay(1);
        }
    }
}

void transmit_all_keys_released() {
    static int all_keys_released_count = 0;
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (key_state[key_num] == PRESSED) {
            return;
        }
    }
    all_keys_released_count++;
    if (all_keys_released_count >= MAX_ALL_KEYS_RELEASED_COUNT) {
        all_keys_released_count = 0;
        Serial1.write(CODE_ALL_KEYS_RELEASED);
        if (is_all_opposite_keys_released == true){
            keyboard.releaseAll();
            delay(1);
        }
    }
}

void receive_code() {
    int code = 0;
    while (Serial1.available()) {
        code = Serial1.read();
        if(code < CODE_RELEASED){
            receive_pressed(code);
        } else if (code < CODE_ALL_KEYS_RELEASED) {
            receive_released(code);
        } else if (code == CODE_ALL_KEYS_RELEASED) {
            receive_all_keys_released();
        } else {
            flush_RX();
        }
    }
}

void receive_pressed(int code){
    opposite_key_state[code] = PRESSED;
    is_all_opposite_keys_released = false;
    if (THIS_SIDE == LEFT) {
        keyboard.press(right_keymap[code]);
    } else {
        keyboard.press(left_keymap[code]);
    }
    delay(1);
}

void receive_released(int code){
    code -= CODE_RELEASED;
    opposite_key_state[code] = RELEASED;
    if (THIS_SIDE == LEFT) {
        keyboard.release(right_keymap[code]);
    } else {
        keyboard.release(left_keymap[code]);
    }
    delay(1);
}

void receive_all_keys_released(){
    is_all_opposite_keys_released = true;
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (opposite_key_state[key_num] == PRESSED) {
            opposite_key_state[key_num] = RELEASED;
            if (THIS_SIDE == LEFT) {
                keyboard.release(right_keymap[key_num]);
            } else {
                keyboard.release(left_keymap[key_num]);
            }
            delay(1);
        }
    }
}

void flush_RX() {
    while (Serial1.available()) {
        Serial1.read();
    }
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
