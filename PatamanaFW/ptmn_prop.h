#ifndef ptmn_prop_h
#define ptmn_prop_h

#include "ptmn_conf.h"
#include "ptmn_disp.h"
#include "ptmn_lexical_analyzer.h"
#include <Arduino.h>
#include <SPIFFS.h>

#define PROP_FILE_PATH "/.patamana.txt"
#define MAX_NUM_PROPERTIES 64

typedef struct {
    char item[MAX_WORD_LENGTH + 1];
    char value[MAX_WORD_LENGTH + 1];
} property;

class PtmnProp {
public:
    int write(const char *item, const char *value);
    int read(const char *item, char *value);
    int init();

private:
    int get_properties();
    int write_properties();
    int num_properties;
    property properties[MAX_NUM_PROPERTIES];
};

extern PtmnProp ptmn_prop;

#endif