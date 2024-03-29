#include "ptmn_conf.h"
#include "ptmn_disp.h"
#include "ptmn_file.h"
#include "ptmn_prop.h"

void setup() {
    ptmn_disp.begin();
    ptmn_disp.println("\nPatamanaFW 24.04.1\n");
    if (ptmn_file.begin() != 0) {
        delay(1000);
        ESP.restart();
    }
    char path[MAX_PATH_LENGTH + 1];
    if (ptmn_prop.read("keymap", path) == 0) {
        File file = SPIFFS.open(path, "r");
        if (file) {
            // キーマップを読み込む
        }
    } else {
        // 初回起動時や以前のキーマップが削除されている場合はキーマップ選択画面を表示する
    }
    ptmn_disp.set_keymap(0, left_keymap_display, left_key_disp_pos, NUM_KEYS);
    ptmn_disp.display_keymap(0);
    delay(1000);

}

void loop() {
    ptmn_disp.init_menu();
    ptmn_disp.set_menu_header("メニュー");
    ptmn_disp.set_menu_row(0, " keymap変更");
    ptmn_disp.set_menu_row(1, " kaymapアップロード");
    ptmn_disp.set_menu_row(2, " 有線/無線切り替え");
    ptmn_disp.set_menu_row(3, " 4項目目");
    ptmn_disp.set_menu_row(4, " 5項目目");
    ptmn_disp.set_menu_footer("     ↑        ↓     ");
    ptmn_disp.display_menu();
    ptmn_disp.menu_select(0);
    delay(1000);
    ptmn_disp.menu_select(1);
    delay(1000);
    ptmn_disp.menu_select(2);
    delay(1000);
    ptmn_disp.menu_select(3);
    delay(1000);
    ptmn_disp.menu_select(4);
    delay(1000);
    ptmn_disp.menu_select(5);
    delay(1000);
    ptmn_disp.menu_select(2);
    delay(1000);
    ptmn_disp.init_menu();
    ptmn_disp.set_menu_header("有線/無線切り替え");
    ptmn_disp.set_menu_row(0, " 有線");
    ptmn_disp.set_menu_row(1, " 無線");
    ptmn_disp.set_menu_row(2, "   SSID: パタマナ");
    ptmn_disp.set_menu_footer("     ↑         ↓    ");
    ptmn_disp.display_menu();
    ptmn_disp.menu_select(0);
    delay(1000);
    ptmn_disp.menu_select(1);
    delay(1000);
}