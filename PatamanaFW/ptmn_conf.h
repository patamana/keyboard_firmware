#ifndef ptmn_conf_h
#define ptmn_conf_h

#define USE_NIMBLE
#include <Arduino.h>
#include <BleKeyboard.h>
#include <LovyanGFX.hpp>
#include <SPIFFS.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

// 片側のキースイッチの数
#define NUM_KEYS 30

// チャタリング調整
#define MAX_COUNT 7

// シフトレジスタの制御pinの設定
#define SR_LD D1
#define SR_CK D2
#define SR_QH D3

#define LEFT 0
#define RIGHT 1

#define THIS_SIDE LEFT

#define MAX_ALL_KEYS_RELEASED_COUNT 10000

#define MAX_WORD_LENGTH 63

typedef struct {
    int x;
    int y;
} pos;

extern uint8_t left_phisical_layout[NUM_KEYS];
extern uint8_t right_phisical_layout[NUM_KEYS];
extern uint8_t left_keymap[NUM_KEYS];
extern uint8_t right_keymap[NUM_KEYS];
extern char left_keymap_display[NUM_KEYS][8];
extern char right_keymap_display[NUM_KEYS][8];
extern pos left_key_disp_pos[NUM_KEYS];
extern pos right_key_disp_pos[NUM_KEYS];

#endif