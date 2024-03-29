#include "ptmn_disp.h"

PtmnDisp ptmn_disp;

SSD1306_I2C::SSD1306_I2C() {
    {
        auto cfg = _bus_instance.config();      // バス設定用の構造体を取得
        cfg.i2c_port = 0;                       // 使用するI2Cポートを選択 (0 or 1)
        cfg.freq_write = 400000;                // 送信時のクロック
        cfg.freq_read = 400000;                 // 受信時のクロック
        cfg.pin_sda = SDA;                      // SDAを接続しているピン番号
        cfg.pin_scl = SCL;                      // SCLを接続しているピン番号
        cfg.i2c_addr = 0x3C;                    // I2Cデバイスのアドレス
                                                // 基板に書かれた0x78はLSBにR/Wビットがふくまれているので1ビット右にシフトした値
        _bus_instance.config(cfg);              // 設定値を反映
        _panel_instance.setBus(&_bus_instance); // バスをパネルにセット
    }
    {                                        // 表示パネル制御の設定
        auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得
        cfg.panel_width = 128;               // 実際に表示可能な幅
        cfg.panel_height = 64;               // 実際に表示可能な高さ
        cfg.offset_x = 0;                    // パネルのX方向オフセット量
        cfg.offset_y = 0;                    // パネルのY方向オフセット量
        cfg.offset_rotation = 2;             // 回転方向の値のオフセット 0~7 (4~7は上下反転)

        _panel_instance.config(cfg); // 設定値を反映
    }
    setPanel(&_panel_instance); // 使用するパネルをセット
}

/// @brief ディスプレイの開始
void PtmnDisp::begin() {
    oled.init();            // 初期化
    oled.setBrightness(64); // 明るさ 0~255
    oled.setColorDepth(1);  // 階調bit数
    oled.setFont(FONT);
    oled.fillScreen(TFT_BLACK);
    oled.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    oled.setCursor(0, 0);
}

/// @brief ディスプレイをクリアしてカーソルを0,0にセット
void PtmnDisp::clear() {
    oled.fillScreen(TFT_BLACK);
    oled.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    oled.setCursor(0, 0);
}

/// @brief 指定したスプライトにキーマップを登録
/// @param keymap_num スプライトの指定
/// @param key_text キーマップに表示するテキストが入った二次元配列
/// @param key_pos キーマップの画面上のキーのxy座標
/// @param num_keys キーの数
void PtmnDisp::set_keymap(int keymap_num, char key_text[][MAX_KEY_DISP_BYTES], pos key_pos[], int num_keys) {
    keymap_sprites[keymap_num].setColorDepth(1);
    keymap_sprites[keymap_num].createSprite(oled.width(), oled.height());
    keymap_sprites[keymap_num].setFont(FONT);
    keymap_sprites[keymap_num].fillScreen(TFT_BLACK);
    keymap_sprites[keymap_num].setTextColor(TFT_WHITE, TFT_BLACK);
    for (int key_num = 0; key_num < num_keys; key_num++) {
        keymap_sprites[keymap_num].drawString(key_text[key_num], key_pos[key_num].x, key_pos[key_num].y);
    }
}

/// @brief 指定したスプライトのキーマップを表示
/// @param keymap_num スプライトの指定
void PtmnDisp::display_keymap(int keymap_num) {
    keymap_sprites[keymap_num].pushSprite(&oled, 0, 0);
}

/// @brief メニュー用のスプライトの初期化
void PtmnDisp::init_menu() {
    menu_header.setColorDepth(1);
    menu_header.createSprite(oled.width(), FONT_HIGHT);
    menu_header.setFont(FONT);
    menu_header.fillScreen(TFT_BLACK);
    menu_header.setTextColor(TFT_WHITE, TFT_BLACK);
    for (int i = 0; i < MAX_MENU_PAGES; i++) {
        menu_body[i].setColorDepth(1);
        menu_body[i].createSprite(oled.width(), (FONT_HIGHT + 1) * MAX_BODY_ROWS);
        menu_body[i].setFont(FONT);
        menu_body[i].fillScreen(TFT_BLACK);
        menu_body[i].setTextColor(TFT_WHITE, TFT_BLACK);
    }
    menu_footer.setColorDepth(1);
    menu_footer.createSprite(oled.width(), FONT_HIGHT);
    menu_footer.setFont(FONT);
    menu_footer.fillScreen(TFT_BLACK);
    menu_footer.setTextColor(TFT_WHITE, TFT_BLACK);
}

/// @brief メニューヘッダーを設定
/// @param text メニューヘッダーの表示内容（1行まで)
void PtmnDisp::set_menu_header(const char *text) {
    menu_header.drawString(text, 0, 0);
}

/// @brief メニューの選択行を設定
/// @param row_num 行番号
/// @param text 行内容（1行まで)
void PtmnDisp::set_menu_row(int row_num, const char *text) {
    menu_body[row_num / MAX_BODY_ROWS].drawString(text, 0, (row_num % MAX_BODY_ROWS) * (FONT_HIGHT + 1));
}

/// @brief メニューのテキストを登録
/// @param begin_row_num 開始行番号
/// @param text 内容(複数行可)
/// @return 終了行
int PtmnDisp::set_menu_text(int begin_row_num, const char *text) {
    menu_body[begin_row_num / MAX_BODY_ROWS].setCursor(0, (begin_row_num % MAX_BODY_ROWS) * (FONT_HIGHT + 1));
    menu_body[begin_row_num / MAX_BODY_ROWS].print(text);
    int cursor_y = menu_body[begin_row_num / MAX_BODY_ROWS].getCursorY();
    return begin_row_num + cursor_y / (FONT_HIGHT + 1) - begin_row_num % MAX_BODY_ROWS;
}

/// @brief メニューフッターを設定
/// @param text メニューフッターの表示内容(1行まで)
void PtmnDisp::set_menu_footer(const char *text) {
    menu_footer.drawString(text, 0, 0);
}

/// @brief 登録したヘッダー、ボディ、フッターを表示
void PtmnDisp::display_menu() {
    menu_header.pushSprite(&oled, 0, 0);
    menu_body[0].pushSprite(&oled, FONT_WIDTH, FONT_HIGHT + 1);
    menu_footer.pushSprite(&oled, 0, oled.height() - FONT_HIGHT);
}

/// @brief 指定した行番号の左端に●印をつける
/// @param row_num 行番号
void PtmnDisp::menu_select(int row_num) {
    menu_body[row_num / MAX_BODY_ROWS].pushSprite(&oled, 0, FONT_HIGHT + 1);
    oled.drawString("●", 0, (row_num % MAX_BODY_ROWS + 1) * (FONT_HIGHT + 1));
}

/// @brief 現在のカーソル位置に文字を表示
void PtmnDisp::print(const char *text) {
    int i = 0;
    while (text[i] != '\0') {
        int right_margin = oled.width() - oled.getCursorX();
        if (right_margin < FONT_WIDTH && text[i] != '\n') {
            oled.print("\n");
        }
        int bottom_margin = oled.height() - oled.getCursorY();
        if (bottom_margin < FONT_HIGHT) {
            oled.scroll(0, bottom_margin - FONT_HIGHT - 1);
            oled.setCursor(0, oled.height() - FONT_HIGHT);
        }
        oled.print(text[i]);
        i++;
    }
}
/// @brief 現在のカーソル位置に文字を表示
void PtmnDisp::print(const uint8_t *text) {
    int i = 0;
    while (text[i] != '\0') {
        int right_margin = oled.width() - oled.getCursorX();
        if (right_margin < FONT_WIDTH && text[i] != '\n') {
            oled.print("\n");
        }
        int bottom_margin = oled.height() - oled.getCursorY();
        if (bottom_margin < FONT_HIGHT) {
            oled.scroll(0, bottom_margin - FONT_HIGHT);
            oled.setCursor(0, oled.height() - FONT_HIGHT);
        }
        oled.print(text[i]);
        i++;
    }
}
/// @brief 現在のカーソル位置に文字を表示
void PtmnDisp::print(String string) {
    char text[string.length() + 1];
    string.toCharArray(text, string.length() + 1);
    PtmnDisp::print(text);
}
/// @brief 現在のカーソル位置に数字を表示
void PtmnDisp::print(const int num) {
    char text[11];
    sprintf(text, "%d", num);
    PtmnDisp::print(text);
}
/// @brief 現在のカーソル位置に数字を表示
void PtmnDisp::print(const uint8_t num) {
    PtmnDisp::print((int)num);
}
/// @brief 現在のカーソル位置に文字を表示して改行
void PtmnDisp::println(const char *text) {
    PtmnDisp::print(text);
    PtmnDisp::print("\n");
}
/// @brief 現在のカーソル位置に文字を表示して改行
void PtmnDisp::println(const uint8_t *text) {
    PtmnDisp::print(text);
    PtmnDisp::print("\n");
}
/// @brief 現在のカーソル位置に文字を表示して改行
void PtmnDisp::println(String string) {
    PtmnDisp::print(string);
    PtmnDisp::print("\n");
}
/// @brief 現在のカーソル位置に数字を表示して改行
void PtmnDisp::println(const int num) {
    PtmnDisp::print(num);
    PtmnDisp::print("\n");
}
/// @brief 現在のカーソル位置に数字を表示して改行
void PtmnDisp::println(const uint8_t num) {
    PtmnDisp::print(num);
    PtmnDisp::print("\n");
}

void PtmnDisp::print_word_pos(File file, const char *word) {
    PtmnDisp::println(file.path());
    PtmnDisp::print(":");
    PtmnDisp::print((int)file.position());
    PtmnDisp::print(":");
    PtmnDisp::println(word);
}