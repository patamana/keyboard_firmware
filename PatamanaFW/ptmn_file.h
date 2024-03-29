#ifndef ptmn_file_h
#define ptmn_file_h

#include "ptmn_conf.h"
#include "ptmn_disp.h"
#include <Arduino.h>
#include <SPIFFS.h>

#define PROP_FILE_PATH "/.patamana.txt"
#define MAX_FILES 32
#define MAX_PATH_LENGTH MAX_WORD_LENGTH

class PtmnFile {
public:
    int begin();
    int get_all_paths(bool show_hidden = false);
    bool remove(const char *path);
    int num_files;
    char paths[MAX_FILES][MAX_PATH_LENGTH + 1];
};

extern PtmnFile ptmn_file;

#endif