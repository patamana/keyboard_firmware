#ifndef ptmn_code_h
#define ptmn_code_h

#include "ptmn_conf.h"
#include "ptmn_disp.h"
#include "ptmn_key.h"
#include <Arduino.h>
#include <SPIFFS.h>

// 左右通信のコード設定
#define CODE_RELEASED 0x40
#define CODE_ALL_KEYS_RELEASED 0x80

class PtmnCode {
public:
    void begin();
    void transmit(uint8_t code);
    void receive();
    void flush();
    void receive_pressed(int code);
    void receive_released(int code);
    void receive_all_keys_released();
};

extern PtmnCode ptmn_code;

#endif