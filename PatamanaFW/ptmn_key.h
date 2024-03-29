#ifndef ptmn_key_h
#define ptmn_key_h

#include "ptmn_code.h"
#include "ptmn_conf.h"
#include "ptmn_disp.h"
#include "ptmn_prop.h"
#include <Arduino.h>
#include <SPIFFS.h>

#define USE_NIMBLE
#include <BleKeyboard.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

#define UNCHANGED 0
#define PRESSED 1
#define RELEASED 2

#define WIRED 0
#define WIRELESS 1

class PtmnKey {
public:
    void begin();
    void read();
    void read_SR();
    void count_pressed();
    void judge_state();
    void send();
    void receive_pressed(int key_num);
    void receive_released(int key_num);
    void receive_all_keys_released();
    void update_prev_state();
    void switch_to_wireless();
    void switch_to_wired();
    char temp_key_state[NUM_KEYS];
    char pressed_counts[NUM_KEYS];
    char key_state[NUM_KEYS];
    char opposite_key_state[NUM_KEYS];
    char prev_key_state[NUM_KEYS];
    char opposite_prev_key_state[NUM_KEYS];
    BLEKeyReport report;
    int connection_type;
};

extern PtmnKey ptmn_key;

#endif