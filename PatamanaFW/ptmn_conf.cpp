#include "ptmn_conf.h"

// 物理的な配線の設定
uint8_t left_phisical_layout[NUM_KEYS] = {21, 25, 29, 11, 7, 3,
                                          20, 24, 28, 12, 8, 4, 0,
                                          19, 23, 27, 13, 9, 5, 1,
                                          18, 22, 26, 14, 10, 6, 2,
                                          15, 16, 17};

uint8_t right_phisical_layout[NUM_KEYS] = {26, 22, 0, 4, 8, 12,
                                           29, 25, 21, 1, 5, 9, 13,
                                           28, 24, 20, 2, 6, 10, 14,
                                           27, 23, 19, 3, 7, 11, 15,
                                           16, 17, 18};

// 論理配列の設定
uint8_t left_keymap[NUM_KEYS] = {'`', '1', '2', '3', '4', '5',
                                 KEY_ESC, 'q', 'w', 'e', 'r', 't', '[',
                                 KEY_TAB, 'a', 's', 'd', 'f', 'g', KEY_UP_ARROW,
                                 '\\', 'z', 'x', 'c', 'v', 'b', KEY_DOWN_ARROW,
                                 KEY_LEFT_GUI, ' ', KEY_LEFT_CTRL};

uint8_t right_keymap[NUM_KEYS] = {'6', '7', '8', '9', '0', '-',
                                  ']', 'y', 'u', 'i', 'o', 'p', '=',
                                  KEY_LEFT_ARROW, 'h', 'j', 'k', 'l', ';', '\'',
                                  KEY_RIGHT_ARROW, 'n', 'm', ',', '.', '/', KEY_BACKSPACE,
                                  KEY_RIGHT_ALT, KEY_RETURN, KEY_RIGHT_SHIFT};

// OLEDへの表示設定
char left_keymap_display[NUM_KEYS][8] = {"`", "1", "2", "3", "4", "5",
                                         "ES", "q", "w", "e", "r", "t", "[",
                                         "TB", "a", "s", "d", "f", "g", "↑",
                                         "\\", "z", "x", "c", "v", "b", "↓",
                                         "CM", "SP", "CT"};

char right_keymap_display[NUM_KEYS][8] = {"6", "7", "8", "9", "0", "-",
                                          "]", "y", "u", "i", "o", "p", "=",
                                          "←", "h", "j", "k", "l", ";", "\'",
                                          "→", "n", "m", ",", ".", "/", "BS",
                                          "OP", "RE", "SH"};

pos right_key_disp_pos[NUM_KEYS] = {
    {32, 0}, {48, 0}, {64, 0}, {80, 0}, {96, 0}, {112, 0}, {16, 12}, {32, 12}, {48, 12}, {64, 12}, {80, 12}, {96, 12}, {112, 12}, {16, 24}, {32, 24}, {48, 24}, {64, 24}, {80, 24}, {96, 24}, {112, 24}, {16, 36}, {32, 36}, {48, 36}, {64, 36}, {80, 36}, {96, 36}, {112, 36}, {0, 48}, {16, 48}, {32, 48}};

pos left_key_disp_pos[NUM_KEYS] = {
    {0, 0}, {16, 0}, {32, 0}, {48, 0}, {64, 0}, {80, 0}, {0, 12}, {16, 12}, {32, 12}, {48, 12}, {64, 12}, {80, 12}, {96, 12}, {0, 24}, {16, 24}, {32, 24}, {48, 24}, {64, 24}, {80, 24}, {96, 24}, {0, 36}, {16, 36}, {32, 36}, {48, 36}, {64, 36}, {80, 36}, {96, 36}, {80, 48}, {96, 48}, {112, 48}};
