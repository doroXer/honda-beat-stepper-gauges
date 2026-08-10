# Pinout / ピン配置

This document summarizes the Arduino pin assignments used by the formal versions in this repository. Always confirm the selected version before wiring because v1 and v2 use different motor-drive hardware.

本ドキュメントは、本リポジトリに収録する正式版のArduinoピン配置をまとめたものです。V1系とV2系ではモーター駆動ハードウェアが異なるため、配線前に使用するバージョンを必ず確認してください。

## Tachometer v1.1 / タコメーター v1.1

| Pin | Function / 機能 |
|---|---|
| D2 | Tach pulse input / タコ信号入力 |
| D3 | Key-ON input / キーON入力 |
| D4 | X27.168 motor output 1 / X27.168モーター出力1 |
| D5 | X27.168 motor output 2 / X27.168モーター出力2 |
| D6 | X27.168 motor output 3 / X27.168モーター出力3 |
| D7 | X27.168 motor output 4 / X27.168モーター出力4 |
| D13 | Status LED / 状態LED |

Motor drive / モーター駆動: SwitecX25 library / SwitecX25ライブラリ

## Tachometer v2.0 / v2.1 / タコメーター v2.0 / v2.1

| Pin | Function / 機能 |
|---|---|
| D2 | Tach pulse input / タコ信号入力 |
| D3 | Key-ON input / キーON入力 |
| D5 | DRV8833 AIN2 = A- |
| D6 | DRV8833 AIN1 = A+ |
| D7 | DRV8833 STBY / nSLEEP |
| D9 | DRV8833 BIN1 = B+ |
| D10 | DRV8833 BIN2 = B- |
| D13 | Status LED / 状態LED |

v2.0 uses 1/4 microstepping. v2.1 uses 1/16 microstepping.

v2.0は1/4マイクロステップ、v2.1は1/16マイクロステップです。

## Speedometer v1.1 / スピードメーター v1.1

| Pin | Function / 機能 |
|---|---|
| D2 | Vehicle-speed pulse input / 車速パルス入力 |
| D3 | Key-state input / キー状態入力 |
| D4 | X27.168 motor output 1 / X27.168モーター出力1 |
| D5 | X27.168 motor output 2 / X27.168モーター出力2 |
| D6 | X27.168 motor output 3 / X27.168モーター出力3 |
| D7 | X27.168 motor output 4 / X27.168モーター出力4 |
| D13 | Status LED / 状態LED |

Motor drive / モーター駆動: SwitecX25 library / SwitecX25ライブラリ

## Speedometer v2.0 / v2.1 / スピードメーター v2.0 / v2.1

| Pin | Function / 機能 |
|---|---|
| D2 | Vehicle-speed pulse input / 車速パルス入力 |
| D3 | Key-ON input / キーON入力 |
| D5 | DRV8833 AIN2 = A- |
| D6 | DRV8833 AIN1 = A+ |
| D7 | DRV8833 STBY / nSLEEP |
| D9 | DRV8833 BIN1 = B+ |
| D10 | DRV8833 BIN2 = B- |
| D13 | Status LED / 状態LED |

v2.0 uses 1/4 microstepping. v2.1 uses 1/16 microstepping.

v2.0は1/4マイクロステップ、v2.1は1/16マイクロステップです。

## Fuel & coolant temperature v1.0 / 燃料計・水温計 v1.0

| Pin | Function / 機能 |
|---|---|
| A0 | Coolant-temperature sensor input / 水温センサー入力 |
| A1 | Fuel sensor input / 燃料センサー入力 |
| D4-D7 | Coolant-temperature motor / 水温計モーター |
| D8-D11 | Fuel motor / 燃料計モーター |

Motor drive / モーター駆動: SwitecX25 library / SwitecX25ライブラリ

## Vehicle-side interface caution / 車両側インターフェース注意

The tables above describe Arduino-side logical assignments only. They do not mean that raw vehicle voltage can be connected directly to the microcontroller. Use suitable voltage conversion, transient protection, filtering, grounding, and fusing for the actual vehicle signals and power supply.

上表はArduino側の論理的なピン割り当てを示すものであり、車両の生電圧をマイコンへ直接接続できることを意味しません。実車信号・電源には、適切な電圧変換、サージ保護、フィルタ、GND設計、ヒューズを使用してください。
