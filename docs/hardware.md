# Hardware notes / ハードウェア上の注意

This document describes hardware requirements that are common to the Honda Beat stepper-gauge implementations in this repository.

本ドキュメントでは、このリポジトリに収録するHonda Beat用ステッピングメーターに共通するハードウェア要件を記載します。

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
