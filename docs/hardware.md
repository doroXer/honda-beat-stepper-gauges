# Hardware notes / ハードウェア上の注意

This document describes hardware requirements that are common to the Honda Beat stepper-gauge implementations in this repository.

本ドキュメントでは、このリポジトリに収録するHonda Beat用ステッピングメーターに共通するハードウェア要件を記載します。

## Vehicle pulse input conditioning / 車両パルス入力処理

### General requirements / 共通要件

The tachometer and speedometer sketches expect a clean, conditioned logic-level pulse at the microcontroller input. The original vehicle signal must not be assumed to be directly compatible with an Arduino GPIO input.

タコメーターおよびスピードメーターのスケッチは、マイコン入力に明確な整形済みロジックレベルパルスが入力されることを前提としています。車両側の元信号がArduino GPIOへそのまま入力できるとは考えないでください。

Automotive electrical signals can contain voltages above the MCU input range, negative excursions, transients, ringing, and noise. The actual signal should be measured at the intended connection point, and an appropriate interface circuit should be designed before connecting it to the microcontroller.

車載電気信号には、マイコン入力範囲を超える電圧、負方向の電圧、サージ、リンギング、ノイズ等が含まれる場合があります。マイコンへ接続する前に、実際に使用する信号取り出し位置で波形を確認し、それに適したインターフェース回路を設計してください。

A typical functional signal path is:

代表的な機能構成は以下です。

```text
Vehicle signal
      |
Input protection
      |
Voltage limiting / level conversion
      |
Waveform shaping
      |
MCU logic input
```

```text
車両信号
   |
入力保護
   |
電圧制限／レベル変換
   |
波形整形
   |
マイコン・ロジック入力
```

Depending on the measured signal, the interface may use combinations of series resistance, resistor division, clamps, Zener or TVS devices, comparator circuits, Schmitt-trigger inputs, or electrical isolation. These are design options, not universal required parts; the appropriate circuit depends on the actual vehicle signal.

実測した信号に応じて、直列抵抗、抵抗分圧、クランプ、ツェナー／TVS、コンパレータ、シュミットトリガ入力、電気的絶縁等を組み合わせることが考えられます。これらは一律に必要な部品ではなく、実際の車両信号に応じて適切な回路を選定してください。

The tachometer and speedometer signals do not necessarily have the same voltage, source impedance, polarity, waveform, or noise environment. Do not assume that one identical conditioning circuit is suitable for both inputs.

タコ信号とスピード信号は、電圧、出力インピーダンス、極性、波形、ノイズ環境が同一とは限りません。両入力に同じ信号処理回路をそのまま適用できるとは限らない点に注意してください。

### Speedometer input / スピードメーター入力

The speedometer software measures pulse timing / frequency after the vehicle-speed signal has been converted to a valid digital input. Before designing the interface, confirm the HIGH/LOW voltage levels, pulse amplitude, output type, and waveform at the actual vehicle connection point.

スピードメーターのソフトウェアは、車速信号が有効なデジタル入力へ変換された後のパルス周期／周波数を計測します。入力回路を設計する前に、実際の車両接続点におけるHIGH/LOW電圧、パルス振幅、出力方式、波形を確認してください。

If the vehicle signal exceeds the allowable Arduino input range, level conversion or voltage limiting is required. Noise or ringing that produces additional digital edges can be interpreted as additional vehicle-speed pulses and can therefore cause incorrect speed indication and pulse-count-based distance errors.

車両信号がArduinoの許容入力範囲を超える場合は、レベル変換または電圧制限が必要です。また、ノイズやリンギングによって余分なデジタルエッジが発生すると、それを追加の車速パルスとして認識し、速度表示の誤差やパルス積算を利用する距離情報の誤差につながる可能性があります。

The interface should therefore provide a stable logic waveform with one intended digital edge for each vehicle-speed pulse expected by the software.

したがって、ソフトウェアが想定する各車速パルスに対して、意図したデジタルエッジが確実に1回得られる安定したロジック波形へ整形してください。

### Tachometer input / タコメーター入力

The tachometer signal requires particular care because an ignition-system or ECU-derived tach signal is not necessarily a conventional 5 V logic signal. Its amplitude, polarity, pulse shape, and transient content depend on the vehicle and on where the signal is taken from.

タコ信号は特に注意が必要です。点火系またはECU由来のタコ信号は、一般的な5Vロジック信号とは限りません。振幅、極性、パルス形状、過渡成分は車両および信号の取り出し位置によって異なります。

Depending on the source, the signal may contain higher-voltage pulses, negative-going excursions, ringing, or ignition-related noise. Do not connect an unverified tachometer signal directly to the Arduino input. Measure the actual waveform and provide suitable protection, level conversion, and waveform shaping first.

信号源によっては、高い電圧のパルス、負方向の電圧、リンギング、点火系由来のノイズ等を含む可能性があります。未確認のタコ信号をArduino入力へ直接接続しないでください。実際の波形を測定したうえで、適切な入力保護、レベル変換、波形整形を行ってください。

From the software point of view, the required result is a reliable digital edge sequence that represents the intended engine-speed pulse information. The hardware interface is responsible for converting the actual vehicle signal into that form without generating false or missing edges.

ソフトウェア側で必要なのは、エンジン回転情報を正しく表す信頼できるデジタルエッジ列です。実車信号を、誤エッジやパルス欠落を発生させずにその形式へ変換することはハードウェア入力回路側の役割です。

## Tachometer and speedometer: key-off power hold / タコ・スピード：キーOFF後の電源保持

### Requirement / 要件

The tachometer and speedometer controller must remain powered for a short period after the ignition key is switched OFF. The microcontroller and, where required for needle movement, the motor driver must continue operating long enough for the software to detect key-off and complete its shutdown / needle-position handling.

タコメーターおよびスピードメーターの制御回路は、イグニッションキーをOFFにした後も短時間動作できるように電源を保持する必要があります。ソフトウェアがキーOFFを検出し、終了処理・針位置処理を完了できるまで、マイコンと、針を動かすために必要な場合はモータードライバへの給電を継続してください。

### Power architecture / 電源構成

The held controller power and the ignition/key-off detection signal must be separated functionally. If the key-off detection input is connected only to the held rail, the controller cannot determine when the vehicle ignition supply has actually disappeared.

電源保持される制御系電源と、イグニッション／キーOFF検出信号は機能的に分離してください。キーOFF検出入力まで保持電源側だけを監視すると、車両側のイグニッション電源が実際に消失したタイミングをマイコンが判定できません。

A typical concept is:

```text
Vehicle IG power ----+----> Key-off sense input
                     |
                     +----> Regulator / protection ----> hold-up element ----> MCU / motor driver
                                                     (capacitor, EDLC, etc.)
```

代表的な考え方は以下です。

```text
車両IG電源 ----------+----> キーOFF検出入力
                     |
                     +----> レギュレータ／保護回路 ----> 電源保持素子 ----> マイコン／モータードライバ
                                                     （コンデンサ、EDLC等）
```

### Hold time / 保持時間

No universal hold time is specified here. The necessary duration depends on supply voltage, capacitance, load current, motor drive conditions, needle position, and software behavior. Verify on the completed hardware that the required key-off sequence always finishes before the held supply falls below the operating range of the MCU or motor driver.

本ドキュメントでは一律の保持時間は規定しません。必要時間は、電源電圧、容量、負荷電流、モーター駆動条件、針位置、ソフトウェア動作によって変化します。必要なキーOFF処理が完了する前に、保持電圧がマイコンまたはモータードライバの動作範囲を下回らないことを完成状態のハードウェアで確認してください。

### Important / 重要

Do not design the system so that ignition OFF immediately removes power from both the microcontroller and motor driver. The key-off behavior implemented by the gauge software assumes that a short post-key-off operating period is available.

キーOFFと同時にマイコンおよびモータードライバの電源が失われる構成にはしないでください。メーターソフトウェアのキーOFF処理は、キーOFF後にも短時間動作できることを前提としています。
