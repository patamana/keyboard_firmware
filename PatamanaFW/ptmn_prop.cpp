/*
    プロパティファイルの構造

        プロパティ名1 = 設定値1;
        #コメント
        プロパティ名2 = 設定値2;
*/

#include "ptmn_prop.h"

PtmnProp ptmn_prop;

/// @brief 指定した項目の値を設定する
/// @return 0 : 成功, -1 : 失敗
int PtmnProp::write(const char *item, const char *value) {
    if (PtmnProp::get_properties() != 0) {
        init();
        return -1;
    }
    // すでに設定されていれば書き直す
    for (int i = 0; i < num_properties; i++) {
        if (strcmp(item, properties[i].item) == 0) {
            strcpy(properties[i].value, value);
            if (write_properties() != 0) {
                return -1;
            }
            return 0;
        }
    }
    // 初めて設定する場合
    strcpy(properties[num_properties].item, item);
    strcpy(properties[num_properties].value, value);
    num_properties++;
    if (write_properties() != 0) {
        return -1;
    }
    return 0;
}

/// @brief 指定した項目の値を読み取る
/// @return 0 : 成功, -1 : 項目が設定されていない
int PtmnProp::read(const char *item, char *value) {
    if(PtmnProp::get_properties() != 0) {
        init();
        return -1;
    }
    for (int i = 0; i < num_properties; i++) {
        if (strcmp(item, properties[i].item) == 0) {
            strcpy(value, properties[i].value);
            return 0;
        }
    }
    return -1;
}

int PtmnProp::init () {
    File file = SPIFFS.open(PROP_FILE_PATH, "w");
    file.close();
    ptmn_disp("Initialized properties");
}
 
/// @brief ファイルから項目をすべて読み出す
/// @param file File構造体
/// @return 0 : 成功, -1 : 異常
int PtmnProp::get_properties() {
    File file = SPIFFS.open(PROP_FILE_PATH, "r");
    if (!file) {
        num_properties = 0;
        return 0;
    }
    char word[MAX_WORD_LENGTH + 1];
    for (int i = 0; i < MAX_NUM_PROPERTIES; i++) {
        if (ptmn_lex.get_next_word(file, word) == 0) {
            num_properties = i;
            file.close();
            return 0;
        }
        if (strcmp(word, "=") == 0 || strcmp(word, ";") == 0){
            ptmn_disp.println("ERR missing item");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        strcpy(properties[i].item, word);
        if (ptmn_lex.get_next_word(file, word) == 0) {
            ptmn_disp.println("ERR missing '='");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        if (strcmp(word, "=") != 0) {
            ptmn_disp.println("ERR missing '='");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        if (ptmn_lex.get_next_word(file, word) == 0) {
            ptmn_disp.println("ERR missing value");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        if (strcmp(word, "=") == 0 || strcmp(word, ";") == 0) {
            ptmn_disp.println("ERR missing value");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        strcpy(properties[i].value, word);
        if (ptmn_lex.get_next_word(file, word) == 0) {
            ptmn_disp.println("ERR missing ';'");
            ptmn_disp.print_word_pos(file, word);
            num_properties = i;
            file.close();
            return -1;
        }
        if (strcmp(word, ";") != 0) {
            ptmn_disp.println("ERR missing ';'");
            ptmn_disp.print_word_pos(file, word);
            file.close();
            return -1;
        }
    }
    if (ptmn_lex.get_next_word(file, word) != 0) {
        ptmn_disp.println("ERR too many properties");
        ptmn_disp.print_word_pos(file, word);
        file.close();
        return -1;
    }
    num_properties = MAX_NUM_PROPERTIES;
    file.close();
    return 0;
}

/// @brief すべての項目を設定ファイルに書き出す
/// @return 0 : 成功, -1 : 失敗
int PtmnProp::write_properties() {
    File file = SPIFFS.open(PROP_FILE_PATH, "w", true);
    if (!file) {
        ptmn_disp.println("ERR could not write properties");
        return -1;
    }
    for (int i = 0; i < num_properties; i++) {
        file.print(properties[i].item);
        file.print(" = ");
        file.print(properties[i].value);
        file.println(";");
    }
    return 0;
}