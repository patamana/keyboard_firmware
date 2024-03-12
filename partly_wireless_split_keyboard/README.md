# partly_wireless_split_keyboad
端末との接続は無線、左右の接続は有線の部分的無線接続分割キーボードです。

## ライブラリの変更点
ESP32-BLE-Keyboard.hに変更が必要です。

BleKeyboard.cpp の pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND); を pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); に変更してください。

キーマクロを統一するため、
BleKeyboard.h の const uint8_t をコメントアウトして、USBHIDKeyboard.h を include してください。

BleKeyboard.h および BleKeyboard.cpp のKeyReport を BLEKeyReport に変更してください。

## キーマップの変更方法
pwsk_settins.h で物理レイアウトやキーマップの設定を行います。

left_phisical_layout や right_phisical_layout に、continuity_check ファームウェアを書き込んだ状態でキーを押した時にOLEDに表示される番号を入れてください。物理的な配線の設定をします。

left_keymap や right_keymap でキーマップを設定してください。<br>
KEY_LEFT_SHIFT や KEY_RETURN といったマクロは [USBHIDKeyboard.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/USB/src/USBHIDKeyboard.h) で定義されています。機能は [Keyboard Modifiers and Special Keys](https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/) を参照してください。

---
以下の Arduino Core を使用しています。<br>
[arduino-esp32](https://github.com/espressif/arduino-esp32)

以下のライブラリを使用しています。<br>
[ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) <br>
[NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)<br>
[LovyanGFX](https://github.com/lovyan03/LovyanGFX)