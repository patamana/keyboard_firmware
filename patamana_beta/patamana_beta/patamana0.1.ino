#include "patamana.h"

// 左右の指定(右ならLEFTをRIGHTに変更)
#define THIS_SIDE RIGHT
// チャタリング調整 この回数連続でLOWまたはHIGHなら状態を変える
#define MAX_COUNT 32

// wifi設定 キーマップアップロード時、以下の設定のアクセスポイントとして動作する
const char ssid[] = "patamana_keyboard";
const char pass[] = "patamana";      // パスワードは8文字以上
const IPAddress ip(192, 168, 83, 7); // ブラウザからここにアクセス
const IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);

// Bluetooth設定
BleKeyboard bleKeyboard("patamana_keyboard"); // デバイスの名前

USBHIDKeyboard keyboard;

// シフトレジスタの制御pinの設定
const int LD = D1;
const int CK = D2;
const int QH = D3;

LGFX_SSD1306 display;
LGFX_Sprite keymap_sprites[MAX_SPRITES];

patamana_setting setting;

layer layers[MAX_LAYERS];
int num_layers;
layer tmp_layers[MAX_LAYERS];
int tmp_num_layers;

char current_key_state[NUM_KEYS];
char prev_key_state[NUM_KEYS];
char temp_key_state[NUM_KEYS];
char key_counter[NUM_KEYS];
int current_layer;
int prev_layer;
bool key_state_changed;

void setup() {
    //Serial.begin(115200);
    Serial1.setRxBufferSize(256);
    Serial1.setTxBufferSize(256);
    Serial1.begin(115200, SERIAL_8N1, RX, TX);

    pinMode(LD, OUTPUT);
    pinMode(CK, OUTPUT);
    pinMode(QH, INPUT);

    digitalWrite(LD, HIGH);
    digitalWrite(CK, HIGH);

    // oledのセットアップ
    setup_oled();

    // SPIFFSのセットアップ
    if (!SPIFFS.begin(true)) {
        display.printf("ERR SPIFFS could not begin \n");
    }
    display.printf("SPIFFS started \n");

    if(THIS_SIDE == LEFT){
        // wifiのセットアップ
        if (!WiFi.softAP(ssid, pass)) {
            display.println("ERR WiFi could not set up");
            return;
        }
        delay(100);
        WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定
        IPAddress myIP = WiFi.softAPIP();
        command_line("SSID: patamana_keyboard'\n'");
        command_line("PASS: patamana");
        command_line("Adress: 192.168.83.7");
        configureWebServer();
        server.begin();
    }

    // 前回の設定ファイルのロード
    // layers, num_layers
    load_setting();

    // 初期化
    for (int i = 0; i < NUM_KEYS; i++) {
        current_key_state[i] = RELEASED;
        prev_key_state[i] = RELEASED;
        key_counter[i] = 0;
    }
    keyboard.begin();
    USB.begin();
}

void loop() {
    key_state_changed = false;
    check_receipt();
    if (num_layers == 0) {
        return;
    }
    get_and_send_keys();
    delay(1);
}

void setup_oled() {
    display.init();            // 初期化
    display.setBrightness(64); // 明るさ 0~255
    display.setColorDepth(1);  // 階調bit数
    display.setCursor(0, 0);
    display.setTextColor(TFT_WHITE, TFT_BLACK); // 文字色と背景色の設定
    display.setFont(&fonts::efontJA_16);        // フォント設定
    display.printf("パタマナ\n");
    display.setFont(&fonts::efontJA_10);
    for (int i = 0; i < MAX_SPRITES; i++) {
        keymap_sprites[i].setColorDepth(1);
        keymap_sprites[i].createSprite(display.width(), display.height());
        keymap_sprites[i].setFont(&fonts::efontJA_12);
        keymap_sprites[i].setTextColor(TFT_WHITE, TFT_BLACK);
    }
}

void command_line(const char *text) {
    display.setFont(&fonts::efontJA_10);
    int i = 0;
    while (text[i] != '\0') {
        int right_margin = display.width() - display.getCursorX();
        if (right_margin < 6 && text[i] != '\n') {
            display.print("\n");
        }
        int bottom_margin = display.height() - display.getCursorY();
        if (bottom_margin < 10) {
            display.scroll(0, bottom_margin - 10);
            display.setCursor(0, display.height() - 10);
        }
        display.print(text[i]);
        i++;
    }
}

void command_line(const uint8_t *text) {
    int i = 0;
    while (text[i] != '\0') {
        command_line(text[i]);
        i++;
    }
}

void command_line(const uint8_t text) {
    display.setFont(&fonts::efontJA_10);
    int right_margin = display.width() - display.getCursorX();
    if (right_margin < 6 && text != '\n') {
        display.print("\n");
    }
    int bottom_margin = display.height() - display.getCursorY();
    if (bottom_margin < 10) {
        display.scroll(0, bottom_margin - 10);
        display.setCursor(0, display.height() - 10);
    }
    display.print(text);
}

void command_line(const int num) {
    char text[11];
    sprintf(text, "%d", num);
    command_line(text);
}

void load_setting() {
    // File file = SPIFFS.open("/setting.dat", "r");
    //  display.println("setting file opned.");
    //  file.readBytes(setting.keymap_file_path, sizeof(setting.keymap_file_path));
    //  if (load_keymap(setting.keymap_file_path) != 0) {
    //      // 追加実装 キーマップ選択画面に遷移
    //      return;
    //  }
    //  file.close();

    if (THIS_SIDE == LEFT) {
        if (load_and_transmit_keymap("/template.txt") != 0) {
            return;
        }
    } else {
        flash_RX();
        Serial1.write(CODE_REQUEST_KEYMAP);
    }
}

void flash_RX() {
    while (Serial1.available()) {
        Serial1.read();
    }
}

void check_receipt() {
    int code = 0;
    while (Serial1.available()) {
        code = Serial1.read();
        //Serial.print("receive ");
        //Serial.println(code, BIN);
        if (code < 0x80) {
            handle_received_key(code);
        } else {
            handle_received_control(code);
        }
    }
}

void handle_received_key(int code) {
    if (code < CODE_PUSHED) {
        current_key_state[code] = RELEASED;
        key_state_changed = true;
    } else {
        current_key_state[code - CODE_PUSHED] = PUSHED;
        key_state_changed = true;
    }
}

void handle_received_control(int code) {
    int key_num = 0;
    int num_keys = NUM_LEFT_KEYS;
    if (THIS_SIDE == LEFT) {
        key_num = NUM_LEFT_KEYS;
        num_keys = NUM_KEYS;
    }
    if (code == CODE_ALL_KEYS_RELEASED) {
        while (key_num < num_keys) {
            current_key_state[key_num++] = RELEASED;
        }
    } else if (code == CODE_TRANSMIT_KEYMAP) {
        receive_keymap();
    } else if (code == CODE_REQUEST_KEYMAP) {
        transmit_keymap();
    }
}

void get_and_send_keys() {
    get_keys_from_shift_resister();
    judge_current_key_state();

    get_current_layer();

    switch_keymap_display();
    send_keys();
    update_prev_state();
}

void switch_keymap_display() {
    if (current_layer == prev_layer) {
        return;
    }
    if (current_layer >= MAX_SPRITES) {
        keymap_sprites[0].pushSprite(&display, 0, 0);
        return;
    }
    keymap_sprites[current_layer].pushSprite(&display, 0, 0);
}

void get_keys_from_shift_resister() {
    int key_num = 0;
    int num_keys = NUM_LEFT_KEYS;
    if (THIS_SIDE == RIGHT) {
        key_num = NUM_LEFT_KEYS;
        num_keys = NUM_KEYS;
    }
    digitalWrite(LD, LOW); // LDがLOWのときシフトレジスタにロード
    digitalWrite(LD, HIGH);                    // ロード待機
    temp_key_state[key_num++] = digitalRead(QH); // 最初の信号がすでにQHにでている
    //Serial.print((int)temp_key_state[key_num]);
    while (key_num < num_keys) {
        digitalWrite(CK, LOW);
        digitalWrite(CK, HIGH); // クロックの立ち上がりでデータがシフトされる
        temp_key_state[key_num++] = digitalRead(QH);
        //Serial.print((int)temp_key_state[key_num]);
    }
    //Serial.print("\n");
    // digitalWrite(LD, LOW);
    // delay(1);
    // digitalWrite(LD, HIGH);
    // delay(1);
    // temp_key_state[num_keys - 1] = digitalRead(QH);
    // for (int i = num_keys - 2; i >= key_num; i--) {
    //     digitalWrite(CK, LOW);
    //     delay(1);
    //     digitalWrite(CK, HIGH);
    //     delay(1);
    //     temp_key_state[i] = digitalRead(QH);
    // }
    // for(int i = num_keys - 2; i >= key_num; i--){
    //     //Serial.print((int)temp_key_state[i]);
    // }
    // //Serial.print("\n");
}

void judge_current_key_state() {
    int key_num = 0;
    int num_keys = NUM_LEFT_KEYS;
    if (THIS_SIDE == RIGHT) {
        key_num = NUM_LEFT_KEYS;
        num_keys = NUM_KEYS;
    }
    int num_pushed_keys = 0;
    while (key_num < num_keys) {
        if (temp_key_state[key_num] == PUSHED) {
            if (prev_key_state[key_num] == RELEASED) {
                if (key_counter[key_num] == MAX_COUNT - 1) {
                    current_key_state[key_num] = PUSHED;
                    num_pushed_keys++;
                    key_counter[key_num]++;
                    key_state_changed = true;
                    Serial1.write(CODE_PUSHED + key_num);
                    //Serial.print("transmit ");
                    //Serial.println(CODE_PUSHED + key_num, BIN);
                } else {
                    current_key_state[key_num] = RELEASED;
                    key_counter[key_num]++;
                }
            } else {
                current_key_state[key_num] = PUSHED;
                num_pushed_keys++;
                if (key_counter[key_num] < MAX_COUNT) {
                    key_counter[key_num]++;
                }
            }
        } else {
            if (prev_key_state[key_num] == PUSHED) {
                if (key_counter[key_num] == 1) {
                    current_key_state[key_num] = RELEASED;
                    key_counter[key_num]--;
                    key_state_changed = true;
                    Serial1.write(CODE_RELEASED + key_num);
                    //Serial.print("transmit ");
                    //Serial.println(CODE_RELEASED + key_num, BIN);
                } else {
                    current_key_state[key_num] = PUSHED;
                    num_pushed_keys++;
                    key_counter[key_num]--;
                }
            } else {
                current_key_state[key_num] = RELEASED;
                if (key_counter[key_num] > 0) {
                    key_counter[key_num]--;
                }
            }
        }
        key_num++;
    }

    if (num_pushed_keys == 0) {
        Serial1.write(CODE_ALL_KEYS_RELEASED);
    }
}

void get_current_layer() {
    int modifier_key_num = 0;
    for (int layer_num = num_layers - 1; layer_num > 0; layer_num--) {
        modifier_key_num = 0;
        while (true) {
            if (current_key_state[layers[layer_num].modifier_keys[modifier_key_num]] == RELEASED) {
                break;
            }
            modifier_key_num++;
            if (modifier_key_num >= layers[layer_num].num_modifier_keys) {
                current_layer = layer_num;
                return;
            }
        }
    }
    current_layer = 0;
}

void send_keys() {
    if (!key_state_changed) {
        return;
    }
    // キーに変化があれば一度すべてrelease
    keyboard.releaseAll();
    // 修飾キーしか押されていなければ、baseレイヤーのキーをそのまま送る
    int num_pushed_keys = 0;
    for (int i = 0; i < NUM_KEYS; i++) {
        if (current_key_state[i] == PUSHED) {
            num_pushed_keys++;
        }
    }
    if (num_pushed_keys == layers[current_layer].num_modifier_keys) {
        for (int modifier_key_num = 0; modifier_key_num < layers[current_layer].num_modifier_keys; modifier_key_num++) {
            int modifier_key = layers[current_layer].modifier_keys[modifier_key_num];
            if (layers[0].keys[modifier_key][0] != 0 && layers[0].keys[modifier_key][1] == 0) {
                keyboard.press(layers[0].keys[modifier_key][0]);
                delay(1);
            }
        }
        return;
    }
    // 修飾キー以外が押されていれば、レイヤーに登録されたキーを送る
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (current_key_state[key_num] == PUSHED) {
            for (int i = 0; i < MAX_SEND_KEYS; i++) {
                if (layers[current_layer].keys[key_num][i] == 0) {
                    break;
                }
                keyboard.press(layers[current_layer].keys[key_num][i]);
                delay(1);
            }
        }
    }
}

void update_prev_state() {
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        prev_key_state[key_num] = current_key_state[key_num];
    }
    prev_layer = current_layer;
}

int load_and_transmit_keymap(const char *file_path) {
    if (load_keymap(file_path) != 0) {
        return -1;
    }
    command_line("Loaded all layers\n");
    if (transmit_keymap() != 0) {
        return -1;
    };
    command_line("Transmitted all layers\n");
    create_display_keymap();
    command_line("Ready to display keymap \n");
    keymap_sprites[0].pushSprite(&display, 0, 0);
    return 0;
}

/// @return success:0, failure:-1
int transmit_keymap() {
    flash_RX();
    Serial1.write(CODE_TRANSMIT_KEYMAP);
    if (transmit_num_layers() != 0) {
        // command_line("FAIL transmit the number of layers\n");
        return -1;
    }
    // command_line("transmitted the number of layers\n");
    if (transmit_layers() != 0) {
        // command_line("FAIL transmit the layers\n");
        return -1;
    }
    // command_line("transmitted the layers\n");
    return 0;
}

/// @return success:0, failure:-1
int transmit_num_layers() {
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        flash_RX();
        Serial1.write(num_layers);
        Serial1.write(num_layers);
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == CODE_SUCCESS) {
            return 0;
        }
        command_line("FAIL transmit packet. Retransmit.\n");
    }
    command_line("ERR retransmission count limit\n");
    return -1;
}

/// @return success:0, failure:-1
int transmit_layers() {
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        if (transmit_layer(layer_num) != 0) {
            command_line("FAIL transmit layer");
            command_line(layer_num);
            command_line("\n");
            return -1;
        }
        command_line("transmitted layer");
        command_line(layer_num);
        command_line("\n");
    }
    return 0;
}

int transmit_layer(int layer_num) {
    if (transmit_modifier_keys(layer_num) != 0) {
        // command_line("FAIL transmit modifier keys\n");
        return -1;
    }
    // command_line("transmitted modifier keys\n");
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (transmit_keys(layer_num, key_num) != 0) {
            return -1;
        }
    }
    // command_line("transmitted keys\n");
    return 0;
}

int transmit_modifier_keys(int layer_num) {
    uint8_t checksum;
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        flash_RX();
        checksum = 0;
        Serial1.write(layer_num);
        checksum += layer_num;
        for (int i = 0; i < MAX_MODIFIER_KEYS; i++) {
            Serial1.write(layers[layer_num].modifier_keys[i]);
            checksum += layers[layer_num].modifier_keys[i];
        }
        Serial1.write(checksum);
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == CODE_SUCCESS) {
            return 0;
        }
        command_line("FAIL transmit packet. Retransmit.\n");
    }
    command_line("ERR retransmission count limit\n");
    return -1;
}

int transmit_keys(int layer_num, int key_num) {
    uint8_t checksum;
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        flash_RX();
        checksum = 0;
        Serial1.write(layer_num);
        checksum += layer_num;
        Serial1.write(key_num);
        checksum += key_num;
        for (int i = 0; i < MAX_SEND_KEYS; i++) {
            Serial1.write(layers[layer_num].keys[key_num][i]);
            checksum += layers[layer_num].keys[key_num][i];
        }
        Serial1.write(checksum);
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == CODE_SUCCESS) {
            return 0;
        }
        command_line("FAIL transmit packet. Retransmit.\n");
    }
    command_line("ERR retransmission count limit\n");
    return -1;
}

//////////////////////////////
/// @return success:0, failure:-1
int receive_keymap() {
    if (receive_num_layers() != 0) {
        // command_line("FAIL receive the number of layers\n");
        return -1;
    }
    // command_line("received the number of layers\n");
    if (receive_layers() != 0) {
        // command_line("FAIL receive the layers\n");
        return -1;
    }
    command_line("Received all layers\n");
    num_layers = tmp_num_layers;
    copy_layers();
    //Serial.println("copy_layers");
    for (int i = 0; i < num_layers; i++) {
        for (int j = 0; j < MAX_MODIFIER_KEYS; j++){
            //Serial.print(layers[i].modifier_keys[j]);
            //Serial.print(",");
        }
        //Serial.print("\n");
        for (int j = 0; j < NUM_KEYS; j++){
            for (int k = 0;k <MAX_SEND_KEYS; k++){
                //Serial.print(layers[i].keys[j][k]);
                //Serial.print(",");
            }
            //Serial.print("\n");
        }
        //Serial.print("\n");
    }
    create_display_keymap();
    command_line("Ready to display keymap \n");
    //Serial.println("ready to display keymap");
    keymap_sprites[0].pushSprite(&display, 0, 0);
    //Serial.println("pushed display keymap");
    return 0;
}

/// @return success:0, failure:-1
int receive_num_layers() {
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        if (receive_code(&code) != 0) {
            return -1;
        }
        tmp_num_layers = code;
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == tmp_num_layers) {
            flash_RX();
            Serial1.write(CODE_SUCCESS);
            return 0;
        }
        command_line("FAIL receive packet. Request retransmission.\n");
        flash_RX();
        Serial1.write(CODE_FAILURE);
    }
    command_line("ERR retransmission count limit\n");
    return -1;
}

/// @return success:0, failure:-1
int receive_layers() {
    for (int layer_num = 0; layer_num < tmp_num_layers; layer_num++) {
        if (receive_layer(layer_num) != 0) {
            // command_line("FAIL receive layer"); command_line(layer_num); command_line("\n");
            return -1;
        }
        command_line("received layer");
        command_line(layer_num);
        command_line("\n");
    }
    return 0;
}

int receive_layer(int layer_num) {
    if (receive_modifier_keys(layer_num) != 0) {
        return -1;
    }
    // command_line("received modifier keys\n");
    for (int key_num = 0; key_num < NUM_KEYS; key_num++) {
        if (receive_keys(layer_num, key_num) != 0) {
            return -1;
        }
    }
    // command_line("received keys\n");
    return 0;
}

int receive_modifier_keys(int layer_num) {
    uint8_t checksum;
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        checksum = 0;
        if (receive_code(&code) != 0) {
            return -1;
        }
        checksum += layer_num;
        for (int i = 0; i < MAX_MODIFIER_KEYS; i++) {
            if (receive_code(&code) != 0) {
                return -1;
            }
            tmp_layers[layer_num].modifier_keys[i] = code;
            checksum += code;
        }
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == checksum) {
            flash_RX();
            Serial1.write(CODE_SUCCESS);
            return 0;
        }
        display.println("FAIL receive packet. Request retransmission.");
        flash_RX();
        Serial1.write(CODE_FAILURE);
    }
    display.println("ERR connection error");
    return -1;
}

int receive_keys(int layer_num, int key_num) {
    uint8_t checksum;
    uint8_t code;
    for (int transmit_num = 0; transmit_num <= MAX_RETRANSMIT; transmit_num++) {
        checksum = 0;
        if (receive_code(&code) != 0) {
            return -1;
        }
        checksum += layer_num;
        if (receive_code(&code) != 0) {
            return -1;
        }
        checksum += key_num;
        for (int i = 0; i < MAX_SEND_KEYS; i++) {
            if (receive_code(&code) != 0) {
                return -1;
            }
            tmp_layers[layer_num].keys[key_num][i] = code;
            checksum += code;
        }
        if (receive_code(&code) != 0) {
            return -1;
        }
        if (code == checksum) {
            flash_RX();
            Serial1.write(CODE_SUCCESS);
            return 0;
        }
        display.println("FAIL receive packet. Request retransmission.");
        flash_RX();
        Serial1.write(CODE_FAILURE);
    }
    display.println("ERR connection error");
    return -1;
}

/// @return received_code, timeout:-1
int receive_code(uint8_t *code) {
    for (int i = 0; i < TIMEOUT; i++) {
        if (Serial1.available()) {
            *code = Serial1.read();
            // command_line("receive "); command_line(*code); command_line("\n");
            return 0;
        }
        delay(1);
    }
    command_line("ERR timeout\n");
    return -1;
}

void copy_layers() {
    for (int i = 0; i < num_layers; i++) {
        layers[i].num_modifier_keys = tmp_layers[i].num_modifier_keys;
        for (int j = 0; j < MAX_MODIFIER_KEYS; j++) {
            layers[i].modifier_keys[j] = tmp_layers[i].modifier_keys[j];
        }
        for (int j = 0; j < NUM_KEYS; j++) {
            for (int k = 0; k < MAX_SEND_KEYS; k++) {
                layers[i].keys[j][k] = tmp_layers[i].keys[j][k];
            }
        }
    }
}

/********************************************************************
********************   キーマップの読み込み   ***********************
********************************************************************/
/// @brief loading keymap from file
/// @return success:0, failure:-1
int load_keymap(const char *file_path) {
    File file = SPIFFS.open(file_path, "r");
    if (!file) {
        command_line("ERR file could not be opened\n");
        return -1;
    }
    command_line("opened ");
    command_line(file_path);
    command_line("\n");
    if (load_layers(file) != 0) {
        // command_line("FAIL load the layers\n");
        return -1;
    }
    // command_line("loaded the layers\n");
    return 0;
}

/// @return success:0, failure:-1
int load_layers(File file) {
    if (load_baselayer(file) != 0) {
        command_line("FAIL load layer0\n");
        return -1;
    }
    command_line("loaded layer0\n");
    ignore_space_and_comment(file);
    int layer_num = 1;
    while (file.available()) {
        if (layer_num >= MAX_LAYERS) {
            command_line("ERR too many layers\n");
            command_line("  max number is ");
            command_line(MAX_LAYERS);
            command_line("\n");
            return -1;
        }
        if (load_layer(file, layer_num) != 0) {
            command_line("FAIL load layer\n");
            command_line(layer_num);
            command_line("\n");
            return -1;
        }
        command_line("loaded layer");
        command_line(layer_num);
        command_line("\n");
        layer_num++;
        ignore_space_and_comment(file);
    }
    num_layers = layer_num;
    return 0;
}

/// @return success:0, failure:-1
int load_baselayer(File file) {
    char word[MAX_WORD_LENGTH + 1];
    get_next_word(file, word);
    if (strcmp(word, "base") != 0) {
        command_line("ERR base layer not found\n");
        command_line(" -> ");
        command_line(word);
        command_line("\n");
        return -1;
    }
    get_next_word(file, word);
    if (strcmp(word, "{") != 0) {
        command_line("ERR missing symbol '{' \n");
        command_line(" -> ");
        command_line(word);
        command_line("\n");
        return -1;
    }
    if (register_layer_to_keymap(file, 0) != 0) {
        return -1;
    }
    return 0;
}

/// @return success:0, failure:-1
int load_layer(File file, int layer_num) {
    char word[MAX_WORD_LENGTH + 1];
    int modifier_key_num = 0;
    while (get_next_word(file, word) > 0) {
        if (strcmp(word, "{") == 0) {
            if (register_layer_to_keymap(file, layer_num) != 0) {
                return -1;
            }
            layers[layer_num].num_modifier_keys = modifier_key_num;
            return 0;
        }
        if (modifier_key_num >= MAX_MODIFIER_KEYS) {
            command_line("ERR too many layer modifier keys");
            command_line("  max number is ");
            command_line(MAX_MODIFIER_KEYS);
            command_line("\n");
            return -1;
        }
        if (strcmp(word, "+") == 0) {
            continue;
        }
        int key_num;
        if (word[0] == 'R' || word[0] == 'L') {
            key_num = atoi(&word[1]);
            if (word[0] == 'R') {
                key_num += NUM_LEFT_KEYS;
            }
            if (key_num >= NUM_KEYS) {
                command_line("ERR Key \"");
                command_line(word);
                command_line("\" does not exist\n");
            }
        } else {
            uint8_t converted_word = convert_reserved_word(word);
            if (converted_word == 0) {
                command_line("ERR \"");
                command_line(word);
                command_line("\" is undefined word\n");
                return -1;
            }
            int i = 0;
            while (true) {
                if (layers[0].keys[i][0] == converted_word && layers[0].keys[i][1] == 0) {
                    key_num = i;
                    break;
                } else if (i == NUM_KEYS - 1) {
                    command_line("ERR \"");
                    command_line(word);
                    command_line("\" is not registered in the base layer\n");
                    return -1;
                }
                i++;
            }
        }
        layers[layer_num].modifier_keys[modifier_key_num] = key_num;
        modifier_key_num++;
    }
    command_line("ERR missing symbol '{' \n");
    delay(1);
    return -1;
}

/// @return success:0, failure:-1
int register_layer_to_keymap(File file, int layer_num) {
    char word[MAX_WORD_LENGTH + 1];
    while (get_next_word(file, word) > 0) {
        if (strcmp(word, "}") == 0) {
            return 0;
        }
        if (word[0] != 'R' && word[0] != 'L') {
            command_line("ERR Key does not start with 'L' or 'R'\n");
            command_line(" -> ");
            command_line(word);
            command_line("\n");
            return -1;
        }
        int key_num = atoi(&word[1]);
        if (word[0] == 'R') {
            key_num += NUM_LEFT_KEYS;
        }
        if (key_num >= NUM_KEYS) {
            command_line("ERR Key \"");
            command_line(word);
            command_line("\" does not exist\n");
        }
        get_next_word(file, word);
        if (strcmp(word, ":") != 0) {
            command_line("ERR missing symbol ':'\n");
            command_line(" -> ");
            command_line(word);
            command_line("\n");
            delay(1);
            return -1;
        }
        if (register_key_to_keymap(file, layer_num, key_num) != 0) {
            return -1;
        }
    }
    command_line("ERR missing symbol '}' \n");
    delay(1);
    return -1;
}

/// @return success:0, failure:-1
int register_key_to_keymap(File file, int layer_num, int key_num) {
    char word[MAX_WORD_LENGTH + 1];
    int i = 0;
    while (get_next_word(file, word) > 0) {
        if (strcmp(word, ",") == 0) {
            return 0;
        } else if (i >= MAX_SEND_KEYS) {
            command_line("ERR too many keys to send");
            command_line("   max number is ");
            command_line(MAX_SEND_KEYS);
            command_line("\n");
            command_line(" -> ");
            command_line(word);
            command_line("\n");
            return -1;
        } else if (strcmp(word, "+") == 0) {
            continue;
        } else if (word[0] == '\'') {
            if (word[1] == '\\') {
                layers[layer_num].keys[key_num][i] = word[2];
            }
            layers[layer_num].keys[key_num][i] = word[1];
            i++;
        } else if (word[0] == '\"') {
            int num_characters = strlen(word) - 2;
            if (num_characters > MAX_SEND_KEYS - i) {
                command_line("ERR too many keys to send");
                command_line(" -> ");
                command_line(word);
                command_line("\n");
                return -1;
            }
            for (int j = 0; j < num_characters; j++) {
                layers[layer_num].keys[key_num][i] = word[j + 1];
                i++;
            }
        } else {
            uint8_t converted_word = convert_reserved_word(word);
            if (converted_word == 0) {
                command_line(" -> layer");
                command_line(layer_num);
                command_line("\n");
                return -1;
            }
            layers[layer_num].keys[key_num][i] = converted_word;
            i++;
        }
    }
    command_line("ERR missing symbol ','\n");
    return -1;
}

/// @return converted word
uint8_t convert_reserved_word(char *word) {
    int num_reserved_words = sizeof(reserved_words) / sizeof(reserved_word);
    for (int i = 0; i < num_reserved_words; i++) {
        if (strcmp(word, reserved_words[i].word) == 0) {
            return reserved_words[i].code;
        }
    }
    command_line("ERR \"");
    command_line(word);
    command_line("\" is undefined word");
    return 0;
}

/// @return word length
int get_next_word(File file, char *word) {
    ignore_space_and_comment(file);
    int length = get_word(file, word);
    return length;
}

void ignore_space_and_comment(File file) {
    do {
        ignore_space(file);
    } while (ignore_comment(file) == 1);
}

void ignore_space(File file) {
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

/// @return no comment:0, with comment:1
int ignore_comment(File file) {
    char character;
    if (file.available()) {
        character = file.read();
        if (character != '#') {
            file.seek(-1, SeekCur); // カーソルを現在位置から1バイト前にもどす
            return 0;
        }
    }
    while (file.available()) {
        character = file.read();
        if (character == '\n' || character == '\r') {
            return 1; // コメントがあったら1を返す
        }
    }
    return 0;
}

/// @return word length
int get_word(File file, char *word) {
    char character;
    int i = 0;
    if (!file.available()) {
        word[0] = '\0';
        return 0;
    }
    character = file.read();
    if (character == '{' || character == '}' || character == ':' || character == ',' || character == '+' || character == '#') {
        word[i++] = character;
        word[i] = '\0';
        return i;
    } else if (character == '\'') {
        word[i++] = '\'';
        character = file.read();
        if (character == '\\') {
            character = file.read();
        }
        word[i++] = character;
        character = file.read();
        if (character != '\'') {
            word[i] = '\0';
            command_line("ERR missing symbol \' \n");
            command_line(" -> ");
            command_line(word);
            command_line("  :");
            command_line((int)file.position());
            command_line("\n");
            return -1;
        }
        word[i++] = '\'';
        word[i] = '\0';
        return i;
    } else if (character == '\"') {
        word[i++] = '\"';
        character = file.read();
        while (file.available()) {
            character = file.read();
            if (character == '\"') {
                word[i++] = '\"';
                word[i] = '\0';
                return i;
            } else if (character == '\\') {
                character = file.read();
                word[i++] = character;
            } else {
                word[i++] = character;
            }
            if (i >= MAX_WORD_LENGTH) {
                word[i] = '\0';
                command_line("ERR too long word\n");
                command_line(" -> ");
                command_line(word);
                command_line("  :");
                command_line((int)file.position());
                command_line("\n");
                return -1;
            }
        }
        word[i] = '\0';
        command_line("ERR missing symbol \" \n");
        command_line(" -> ");
        command_line(word);
        command_line("  :");
        command_line((int)file.position());
        command_line("\n");
        return -1;
    } else {
        word[i++] = character;
        while (file.available()) {
            character = file.read();
            if (character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '{' || character == '}' || character == ':' || character == ',' || character == '+' || character == '#' || character == '\'' || character == '\"') {
                file.seek(-1, SeekCur); // カーソルを現在位置から1バイト前にもどす
                word[i] = '\0';
                return i;
            } else {
                word[i++] = character;
            }
            if (i >= MAX_WORD_LENGTH) {
                word[i] = '\0';
                command_line("ERR word length is too long\n");
                command_line("  max length is ");
                command_line(MAX_WORD_LENGTH);
                command_line("\n");
                command_line(" -> ");
                command_line(word);
                command_line("  :");
                command_line((int)file.position());
                command_line("\n");
                return -1;
            }
        }
        word[i] = '\0';
        return i;
    }
}

void create_display_keymap() {
    for (int layer_num = 0; layer_num < MAX_SPRITES; layer_num++) {
        set_keymap_symbols(layer_num);
        command_line("created display layer");
        command_line(layer_num);
        command_line("\n");
    }
}

void set_keymap_symbols(int layer_num) {
    //Serial.println("set_keymap_symbols");
    int key_num = 0;
    int num_keys = NUM_LEFT_KEYS;
    if (THIS_SIDE == RIGHT) {
        key_num = NUM_LEFT_KEYS;
        num_keys = NUM_KEYS;
    }
    keymap_sprites[layer_num].fillScreen(TFT_BLACK);
    keymap_sprites[layer_num].setFont(&fonts::efontJA_12);
    keymap_sprites[layer_num].setTextColor(TFT_WHITE, TFT_BLACK);
    char symbol[9];
    symbol[8] = '\0';
    //Serial.println("symbol");
    while (key_num < num_keys - 2) {
        get_symbols(symbol, layer_num, key_num);
        if (strlen(symbol) != 0) {
            keymap_sprites[layer_num].drawString(symbol, symbol_pos[key_num].x, symbol_pos[key_num].y);
        }
        key_num++;
    }
    keymap_sprites[layer_num].setTextColor(TFT_BLACK, TFT_WHITE);
    if(THIS_SIDE == LEFT){
        int home_keys[5] = { 15, 16, 17, 18, 28 };
        for (int i = 0; i < 5;i++){
            key_num = home_keys[i];
            get_symbols(symbol, layer_num, key_num);
            if (strlen(symbol) != 0) {
                keymap_sprites[layer_num].drawString(symbol, symbol_pos[key_num].x, symbol_pos[key_num].y);
            }
        }
    } else {
        int home_keys[5] = { 47, 48, 49, 50, 60 };
        for (int i = 0; i < 5; i++) {
            key_num = home_keys[i];
            get_symbols(symbol, layer_num, key_num);
            if (strlen(symbol) != 0) {
                keymap_sprites[layer_num].drawString(symbol, symbol_pos[key_num].x, symbol_pos[key_num].y);
            }
        }
    }
}

void get_symbols(char *symbols, int layer_num, int key_num) {
    int num_reserved_symbol = sizeof(display_symbols) / sizeof(display_symbol);
    for (int i = 0; i < num_reserved_symbol; i++) {
        if (layers[layer_num].keys[key_num][0] == display_symbols[i].code) {
            for (int j = 0; j < 8; j++){
                symbols[j] = display_symbols[i].symbol[j];
            }
            // command_line(symbols);
            //Serial.println(symbols);
            return;
        }
    }
    symbols[0] = layers[layer_num].keys[key_num][0];
    symbols[1] = '\0';
    //command_line(symbols);
    //Serial.println(symbols);
    return;
}

/*************************************
******  アクセスポイント関連
*************************************/
void configureWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    // run handleUpload function when any file is uploaded
    server.on(
        "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        handleUpload);
}

String listFiles(bool ishtml) {
    String returnText = "";
    display.println("Listing files stored on SPIFFS");
    File root = SPIFFS.open("/");
    File foundfile = root.openNextFile();
    if (ishtml) {
        returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th></tr>";
    }
    while (foundfile) {
        if (ishtml) {
            returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td></tr>";
        } else {
            returnText += "File: " + String(foundfile.name()) + "\n";
        }
        foundfile = root.openNextFile();
    }
    if (ishtml) {
        returnText += "</table>";
    }
    root.close();
    foundfile.close();
    return returnText;
}

String humanReadableSize(const size_t bytes) {
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        return String(bytes / 1024.0 / 1024.0) + " MB";
    else
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (!index) {
        logmessage = "Upload Start: " + String(filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SPIFFS.open("/" + filename, "w");
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    }

    if (final) {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        request->redirect("/");
    }
}

String processor(const String &var) {
    if (var == "FILELIST") {
        return listFiles(true);
    }
    if (var == "FREESPIFFS") {
        return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
    }
    if (var == "USEDSPIFFS") {
        return humanReadableSize(SPIFFS.usedBytes());
    }
    if (var == "TOTALSPIFFS") {
        return humanReadableSize(SPIFFS.totalBytes());
    }
    return String();
}