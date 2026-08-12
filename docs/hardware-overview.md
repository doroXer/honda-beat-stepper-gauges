# Hardware Overview / ハードウェア概要

This repository contains several gauge-control approaches that share the Honda Beat (PP1) instrument-cluster application but use different motor-drive layers.

本リポジトリには、Honda Beat（PP1）のメーターを対象としつつ、異なるモーター駆動方式を用いる複数の制御方式を収録します。

## Common platform / 共通プラットフォーム

- Arduino Pro Mini 5V / 16MHz
- X27.168 or X25-compatible gauge stepper motors / X27.168またはX25互換メーター用ステッピングモーター
- Vehicle-side pulse or sensor inputs appropriate to each gauge / 各メーターに対応する車両側パルス・センサー入力

## Tachometer v1.1 / タコメーター v1.1

- Motor: X27.168 / モーター: X27.168
- Drive: direct Arduino output through SwitecX25 library / 駆動: SwitecX25ライブラリによるArduino直接駆動
- Tach pulse input: D2 / タコ信号入力: D2
- Key input: D3 / キー入力: D3
- Motor outputs: D4-D7 / モーター出力: D4-D7
- Status LED: D13 / 状態LED: D13

Dependency / 依存ライブラリ:
- SwitecX25

## Tachometer v2.0 / v2.1 / タコメーター v2.0 / v2.1

- Motor: X27.168 / モーター: X27.168
- Driver: DRV8833 / ドライバ: DRV8833
- Tach pulse input: D2 / タコ信号入力: D2
- Key input: D3 / キー入力: D3
- D5: AIN2 = A-
- D6: AIN1 = A+
- D7: STBY / nSLEEP
- D9: BIN1 = B+
- D10: BIN2 = B-
- D13: status / 状態表示

v2.0 uses dedicated 1/4 microstepping; v2.1 uses dedicated 1/16 microstepping.

v2.0は専用1/4マイクロステップ、v2.1は専用1/16マイクロステップ駆動です。

## Speedometer v1.1 / スピードメーター v1.1

- Motor: X27.168 compatible / モーター: X27.168互換
- Drive: SwitecX25 library / 駆動: SwitecX25ライブラリ
- Speed pulse input: D2 / 車速パルス入力: D2
- Key input: D3 / キー入力: D3
- Motor outputs: D4-D7 / モーター出力: D4-D7
- Status LED: D13 / 状態LED: D13

Dependency / 依存ライブラリ:
- SwitecX25

## Speedometer v2.0 / v2.1 / スピードメーター v2.0 / v2.1

- Motor: X27.168 / モーター: X27.168
- Driver: DRV8833 / ドライバ: DRV8833
- Speed pulse input: D2 / 車速パルス入力: D2
- Key input: D3 / キー入力: D3
- D5: AIN2 = A-
- D6: AIN1 = A+
- D7: STBY / nSLEEP
- D9: BIN1 = B+
- D10: BIN2 = B-
- D13: status / 状態表示

v2.0 uses 1/4 microstepping and v2.1 uses 1/16 microstepping.

v2.0は1/4マイクロステップ、v2.1は1/16マイクロステップです。

## Fuel and coolant temperature v1.0 / 燃料計・水温計 v1.0

- Arduino Pro Mini 5V / 16MHz
- Coolant-temperature motor: D4-D7 / 水温計モーター: D4-D7
- Fuel motor: D8-D11 / 燃料計モーター: D8-D11
- Coolant sensor input: A0 / 水温センサー入力: A0
- Fuel sensor input: A1 / 燃料センサー入力: A1

Dependencies / 依存ライブラリ:
- SwitecX25
- Arduino_FreeRTOS_Library

The fuel/coolant implementation uses a single motor-drive task that owns both SwitecX25 motor objects, while separate logic tasks calculate the two gauge targets.

燃料計・水温計は、2つのSwitecX25モーターオブジェクトを単一のモーター駆動タスクが管理し、別のロジックタスクが各メーターの目標位置を計算する構成です。

## Calibration / 校正

The conversion constants and ADC-to-angle relationships used by these gauges include project-specific calibration values. See [`calibration.md`](calibration.md) for the tachometer calibration basis, speedometer conversion, and fuel/coolant ADC calibration assumptions.

各メーターの換算定数やADC→角度関係には、本プロジェクト固有の校正値が含まれます。タコメーターの校正根拠、スピードメーターの換算、燃料・水温のADC校正前提は [`calibration.md`](calibration.md) を参照してください。

## Automotive power and signal caution / 車載電源・信号に関する注意

Do not connect raw automotive-voltage signals directly to Arduino input pins unless the interface circuit has been designed for the expected voltage range, transients, reverse polarity, and noise environment. Provide suitable protection, regulation, filtering, grounding, and fusing for the actual vehicle installation.

車両の生電圧信号を、想定電圧範囲、サージ、逆極性、ノイズ環境への対策なしにArduino入力へ直接接続しないでください。実車搭載時は、適切な保護、電圧変換、フィルタ、GND設計、ヒューズを設けてください。
