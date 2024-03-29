#include "ptmn_code.h"

PtmnCode ptmn_code;

void PtmnCode::begin() {
    Serial1.setRxBufferSize(256);
    Serial1.setTxBufferSize(256);
    Serial1.begin(115200, SERIAL_8N1, RX, TX);
}

void PtmnCode::transmit(uint8_t code) {
    Serial1.write(code);
}
void PtmnCode::receive() {
    int code;
    while (Serial1.available()) {
        code = Serial1.read();
        if (code < CODE_RELEASED) {
            receive_pressed(code);
        } else if (code < CODE_ALL_KEYS_RELEASED) {
            receive_released(code);
        } else if (code == CODE_ALL_KEYS_RELEASED) {
            receive_all_keys_released();
        } else {
            delay(10);
            flush();
        }
    }
}

void PtmnCode::flush() {
    while (Serial1.available()) {
        Serial1.read();
    }
}

void PtmnCode::receive_pressed(int code) {
    if (code >= NUM_KEYS) {
        delay(10);
        flush();
        return;
    }
    ptmn_key.receive_pressed(code);
}

void PtmnCode::receive_released(int code) {
    code -= CODE_RELEASED;
    if (code >= NUM_KEYS) {
        delay(10);
        flush();
        return;
    }
    ptmn_key.receive_released(code);
}

void PtmnCode::receive_all_keys_released() {
    ptmn_key.receive_all_keys_released();
}