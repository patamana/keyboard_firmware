#ifndef ptmn_lex_h
#define ptmn_lex_h

#include "ptmn_conf.h"
#include <Arduino.h>
#include <SPIFFS.h>

#define PROP_FILE_PATH "/.patamana.txt"

class PtmnLex {
public:
    void ignore_space(File file);
    int ignore_comment(File file);
    int get_word(File file, char *word);
    void ignore_space_and_comment(File file);
    int get_next_word(File file, char *word);
};

extern PtmnLex ptmn_lex;

#endif