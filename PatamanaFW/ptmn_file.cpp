#include "ptmn_file.h"

PtmnFile ptmn_file;

int PtmnFile::begin() {
    if (!SPIFFS.begin(true)) {
        ptmn_disp.println("ERR: SPIFFS could not begin");
        return -1;
    }
    get_all_paths();
    return 0;
}

int PtmnFile::get_all_paths(bool show_hidden) {
    File root = SPIFFS.open("/", "r");
    File file = root.openNextFile("r");
    int file_num = 0;
    while (file_num < MAX_FILES) {
        if (!file) {
            break;
        }
        if (file.path()[1] == '.' && !show_hidden) {
            file = root.openNextFile("r");
            continue;
        }
        if(strlen(file.path()) > MAX_PATH_LENGTH) {
            ptmn_disp.println("WARN: path is too long");
            ptmn_disp.println(" ignore this file")
        } else {
            strcpy(paths[file_num], file.path());
            file_num++;
        }
        file = root.openNextFile("r");
    }
    num_files = file_num;
    return 0;
}

/// @return true : success, false : failed
bool PtmnFile::remove(const char *path) {
    return SPIFFS.remove(path);
}
