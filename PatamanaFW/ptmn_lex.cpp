#include "ptmn_lex.h"
#include "ptmn_disp.h"

PtmnLex ptmn_lex;

/// @brief 現在のカーソル位置から次のwordを取得する
/// @param file File構造体
/// @param word wordを入れるための配列
/// @return 取得したwordの長さ
int PtmnLex::get_next_word(File file, char *word) {
    ignore_space_and_comment(file);
    int length = get_word(file, word);
    return length;
}

/// @brief スペース、タブ、改行、コメントを無視してカーソルを進める
/// @param file File構造体
void PtmnLex::ignore_space_and_comment(File file) {
    do {
        ignore_space(file);
    } while (ignore_comment(file) == 1);
}

/// @brief スペース、タブ、改行を無視してカーソルを進める
/// @param file File構造体
void PtmnLex::ignore_space(File file) {
    char character;
    while (file.available()) {
        character = file.read();
        if (character == ' ' || character == '\t' || character == '\n' || character == '\r') {
            continue;
        } else {
            file.seek(-1, SeekCur); // カーソルを現在位置から1バイト前にもどす
            return;
        }
    }
    return;
}

/// @brief コメントを無視してカーソルを進める
/// @param file File構造体
/// @return 0 : コメントなし、1 : コメントあり
int PtmnLex::ignore_comment(File file) {
    char character;
    if (file.available()) {
        character = file.read();
        if (character != '#') {
            file.seek(-1, SeekCur); // カーソルを現在位置から1バイト前にもどす
            return 0;
        }
    }
    // 次の行までカーソルを進める
    while (file.available()) {
        character = file.read();
        if (character == '\n' || character == '\r') {
            return 1; // コメントがあったら1を返す
        }
    }
    return 0;
}

/// @brief カーソルの次の記号や文字列を取得する
/// @param file File構造体
/// @param word 取得したwordを入れるための配列
/// @return 取得したwordの文字数
int PtmnLex::get_word(File file, char *word) {
    char character;
    int i = 0;
    if (!file.available()) {
        word[0] = '\0';
        return 0;
    }
    character = file.read();
    if (character == '{' || character == '}' || character == ':' || character == ';' ||
        character == '+' || character == '=' || character == ',') {
        // これらの記号は一文字を返す
        word[i++] = character;
        word[i] = '\0';
        return i;
    } else if (character == '\'') {
        // シングルクォーテーションはキーコードとして一文字取り出す
        word[i++] = '\'';
        character = file.read();
        if (character == '\\') {
            // バックスラッシュはその後の一文字をキーコードとして取り出す
            character = file.read();
        }
        word[i++] = character;
        character = file.read();
        if (character != '\'') {
            word[i] = '\0';
            ptmn_disp.println("ERR missing '\''");
            ptmn_disp.print_word_pos(file, word);
            return -1;
        }
        word[i++] = '\'';
        word[i] = '\0';
        return i;
    } else if (character == '\"') {
        // ダブルクォーテーションは文字列として取り出す
        word[i++] = '\"';
        character = file.read();
        while (file.available()) {
            character = file.read();
            if (character == '\"') {
                word[i++] = '\"';
                word[i] = '\0';
                return i;
            } else if (character == '\\') {
                // バックスラッシュはその後の一文字を文字として取り出す
                character = file.read();
                word[i++] = character;
            } else {
                word[i++] = character;
            }
            if (i >= MAX_WORD_LENGTH) {
                word[i] = '\0';
                ptmn_disp.println("ERR too long word");
                ptmn_disp.print_word_pos(file, word);
                return -1;
            }
        }
        word[i] = '\0';
        ptmn_disp.println("ERR missing '\"'");
        ptmn_disp.print_word_pos(file, word);
        return -1;
    } else {
        // 予約語
        word[i++] = character;
        while (file.available()) {
            character = file.read();
            if (character == ' ' || character == '\t' || character == '\n' || character == '\r' ||
                character == '{' || character == '}' || character == ':' || character == ';' ||
                character == ',' || character == '+' || character == '=' || character == '#' ||
                character == '\'' || character == '\"') {
                file.seek(-1, SeekCur); // カーソルを現在位置から1バイト前にもどす
                word[i] = '\0';
                return i;
            } else {
                word[i++] = character;
            }
            if (i >= MAX_WORD_LENGTH) {
                word[i] = '\0';
                ptmn_disp.println("ERR too long word");
                ptmn_disp.print_word_pos(file, word);
                return -1;
            }
        }
        word[i] = '\0';
        return i;
    }
}