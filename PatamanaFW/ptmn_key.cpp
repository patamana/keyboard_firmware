#include "ptmn_key.h"

PtmnKey ptmn_sw;
USBHIDKeyboard keyboard;
BleKeyboard ble_keyboard;

void PtmnKey::begin() {
    pinMode(SR_LD, OUTPUT);
    pinMode(SR_CK, OUTPUT);
    pinMode(SR_QH, INPUT);
    digitalWrite(SR_LD, HIGH);
    digitalWrite(SR_CK, HIGH);
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        key_state[key_num] = RELEASED;
        opposite_key_state[key_num] = RELEASED;
        prev_key_state[key_num] = RELEASED;
        opposite_prev_key_state = RELEASED;
    }
    char value[MAX_WORD_LENGTH];
    if(ptmn_prop.read("connection_type", value) == 0){
        if(strcmp(value,"wired")) {
            connection_type = WIRED;
            keyboard.begin();
            USB.begin();
        } else if (strcmp(value,"wireless")) {
            connection_type = WIRELESS;
            ble_keyboard.begin();
        }
    } else {
        ptmn_prop.write("connection_type", "wired");
        connection_type = WIRED;
        keyboard.begin();
        USB.begin();
    }
}

void PtmnKey::read() {
    update_prev_state();
    read_SR();
    count_pressed();
    judge_state();
}

void PtmnKey::read_SR() {
    int key_num = 0;
    digitalWrite(SR_LD, LOW);                       // LDがLOWのときシフトレジスタにロード
    digitalWrite(SR_LD, HIGH);                      // ロード待機
    temp_key_state[key_num++] = digitalRead(SR_QH); // 最初の信号がすでにQHにでている
    while (key_num < NUM_KEYS) {
        digitalWrite(SR_CK, LOW);
        digitalWrite(SR_CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        if(digitalRead(SR_QH) == LOW) {
            temp_key_state[key_num++] == PRESSED;
        } else {
            temp_key_state[key_num++] == RELEASED;
        }
    }
}

void PtmnKey::count_pressed() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (temp_key_state[key_num] == PRESSED && pressed_counts[key_num] < MAX_COUNT) {
            pressed_counts[key_num]++;
        } else if (temp_key_state[key_num] == RELEASED && pressed_counts[key_num] > 0) {
            pressed_counts[key_num]--;
        }
    }
}

void PtmnKey::judge_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (key_state[key_num] == RELEASED && pressed_counts[key_num] == MAX_COUNT) {
            key_state[key_num] = PRESSED;
        } else if (key_state[key_num] == PRESSED && pressed_counts[key_num] == 0) {
            key_state[key_num] = RELEASED;
        }
    }
}

void PtmnKey::receive_pressed(int key_num) {
    opposite_key_state[key_num] = PRESSED;
}

void PtmnKey::receive_released(int key_num) {
    opposite_key_state[key_num] = RELEASED;
}

void PtmnKey::receive_all_keys_released(){
    for (int i = 0; i < NUM_KEYS; i++) {
        opposite_key_state[i] = RELEASED;
    }
}

void PtmnKey::update_prev_state() {
    for (int i = 0; i < NUM_KEYS; i++) {
        prev_key_state[i] = key_state[i];
        opposite_prev_key_state[i] = opposite_prev_key_state[i];
    }
}

void PtmnKey::switch_to_wired() {
    ptmn_prop.write("connection_type", "wired");
    // ble_keyboard.end();が空なので再起動する。
    ptmn_disp.clear();
    ptmn_disp.println("Rebooting");
    ESP.restart();
}

void PtmnKey::switch_to_wireless() {
    ptmn_prop.write("connection_type", "wireless");
    // keyboard.end();が空なので再起動する。
    ptmn_disp.clear();
    ptmn_disp.println("Rebooting");
    ESP.restart();
}
