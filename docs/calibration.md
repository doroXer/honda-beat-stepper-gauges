# Calibration / 校正

This document records the calibration assumptions that materially affect reproduction of the formal gauge versions in this repository.

本ドキュメントでは、本リポジトリの正式版メーターを再現する際に重要となる校正上の前提をまとめます。

## Tachometer / タコメーター

### Calibration basis / 校正根拠

The v1.1 tachometer retains the pointer-position calibration established during the preceding calibrated development version. The calibration was based on actual gauge indication rather than an unverified generic pulse-per-rpm assumption.

タコメーターv1.1は、先行する校正版で確立した実針位置の校正結果を継承しています。一般的なpulse/rpm関係を仮定して決めたものではなく、本プロジェクトの実メーターで針表示を確認して決定した値です。

Observed reference points before correction / 補正前に確認した代表点:

- Command `58 steps` → actual indication approximately `1100 rpm` / 指令 `58 step` → 実針表示 約 `1100 rpm`
- Command `534 steps` → actual indication approximately `7550 rpm` / 指令 `534 step` → 実針表示 約 `7550 rpm`

Corrected reference positions / 補正後の基準位置:

- `1000 rpm` ≈ `51 logical steps`
- `8000 rpm` ≈ `567 logical steps`

The resulting formal conversion constants are:

正式版で使用する換算定数は以下です。

```cpp
const int STEP_OFFSET = 23;
const long STEP_CONVERSION_NUMERATOR = 2952000L;
```

The v2 DRV8833 versions retain this upper logical gauge scale and calibration concept. Their microstepping changes the motor-coordinate resolution, not the upper calibration basis.

DRV8833を使用するv2系でも、上位の論理メーター座標と校正の考え方は維持しています。マイクロステップ化で変更されるのはモーター座標の分解能であり、この上位校正基準ではありません。

Recalibrate if the pointer, dial, motor, signal interface, or mechanical zero position is changed.

針、文字盤、モーター、信号入力回路、機械的ゼロ位置を変更した場合は再校正してください。

## Speedometer / スピードメーター

The formal v1/v2 speedometer software retains the established vehicle-speed conversion:

正式版v1/v2スピードメーターは、確立済みの以下の車速換算を継承しています。

```text
step = pulseHz * 7.48 - 36
```

which is implemented from pulse interval as:

パルス周期からは以下として実装しています。

```cpp
rawSpeedValue = 7480000L / intervalUs;
targetStep = rawSpeedValue - 36;
```

This relationship depends on the vehicle-speed pulse source and the mechanical gauge installation. Verify indicated speed against a trusted reference after changing the pulse source, interface, wheel/tire rolling circumference, gauge geometry, or pointer installation.

この関係は、車速パルス源とメーターの機械的取り付け条件に依存します。パルス源、入力回路、タイヤ外径、メーター形状、針取り付け等を変更した場合は、信頼できる基準速度に対して表示を確認してください。

## Fuel and coolant temperature / 燃料計・水温計

The v1.0 fuel and coolant-temperature software converts filtered ADC readings directly to gauge angle using calibration relationships established for this project installation.

v1.0の燃料計・水温計は、フィルタ後のADC値から、本プロジェクトの実装条件で校正した関係式を使ってメーター角度へ直接換算します。

Current equations / 現在の換算式:

```cpp
// Coolant temperature / 水温
angleDeg = -76.732 * log(tempAdc) + 520.0;

// Fuel / 燃料
angleDeg = -0.1744 * fuelAdc + 115.87;
```

Both results are limited to `0..115 deg` and converted at `3 steps/degree`.

いずれも結果を `0..115 deg` に制限し、`3 step/degree` でモーター位置へ換算します。

These equations are **installation-specific calibration relationships**, not universal transfer functions for every Honda Beat coolant sensor or fuel sender. ADC readings depend on the complete electrical path, including the sensor/sender, input circuit and resistor values, supply voltage, ADC reference, grounding, wiring resistance, and connector condition.

これらは**本プロジェクトの実装条件に対する校正式**であり、Honda Beatのすべての水温センサー／燃料センダーに共通する普遍的な特性式ではありません。ADC値はセンサー／センダーだけでなく、入力回路や抵抗値、電源電圧、ADC基準電圧、GND、配線抵抗、コネクタ状態を含む電気系全体に依存します。

Recalibrate when changing the sensor/sender, interface circuit, MCU supply or ADC reference, wiring/grounding, stepper motor, pointer installation, dial geometry, or mechanical zero position.

センサー／センダー、入力回路、マイコン電源やADC基準、配線・GND、ステッピングモーター、針取り付け、文字盤形状、機械的ゼロ位置を変更した場合は再校正してください。
