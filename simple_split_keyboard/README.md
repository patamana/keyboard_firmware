# simple_split_keyboad
レイヤー機能やマクロ機能のないシンプルなファームウェアです。<br>
コードも大変短いですので、パタマナボードを使う際に参考にしてください。

## キーマップの変更方法
ssk_settings.cpp で物理レイアウトやキーマップの設定を行います。

left_phisical_layout や right_phisical_layout に、continuity_check ファームウェアを書き込んだ状態でキーを押した時にOLEDに表示される番号を入れてください。物理的な配線の設定をします。

left_keymap や right_keymap でキーマップを設定してください。<br>
KEY_LEFT_SHIFT や KEY_RETURN といったマクロは [USBHIDKeyboard.h](https://github.com/espressif/arduino-esp32/blob/master/libraries/USB/src/USBHIDKeyboard.h) で定義されています。機能は [Keyboard Modifiers and Special Keys](https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/) を参照してください。

---
以下の Arduino Core を使用しています。<br>
[arduino-esp32](https://github.com/espressif/arduino-esp32)

以下のライブラリを使用しています。<br>
[LovyanGFX](https://github.com/lovyan03/LovyanGFX)