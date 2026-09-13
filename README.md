# doroXer dark — Honda Beat Stepper Gauges

Honda Beat (PP1) stepper-gauge projects for tachometer, speedometer, fuel gauge, and coolant-temperature gauge.

ホンダ ビート（PP1）のタコメーター、スピードメーター、燃料計、水温計をステッピングモーター化するためのプロジェクトです。

## doroXer dark

`doroXer dark` is the automotive / embedded-electronics side of doroXer. This repository publishes practical vehicle projects with the implementation details required to reproduce and understand them.

`doroXer dark` は、doroXerの自動車・組込み電子工作系の活動区分です。本リポジトリでは、再現・理解に必要な実装情報を含めた実用プロジェクトを整理します。

## Current public gauge generation / 現行公開世代

For the DRV8833 1/16-microstep tachometer and speedometer, **v3.0 is the current public version**. It was derived from stored real-vehicle raw logs, simulation, and subsequent vehicle validation.

DRV8833・1/16マイクロステップ駆動のタコメーター／スピードメーターについては、**v3.0を現行公開版**とします。保存済み実車Rawログ、シミュレーション、その後の実車確認を経て公開へ昇格した版です。

Common v3.0 upper-control values are:

- Control interval: `50 ms`
- TARGET UP/DOWN: `1600/1600`
- Virtual VEL UP/DOWN: `425/160`
- Virtual ACC UP/DOWN: `2600/1500`
- STOP BAND: `0`

Input filtering and Motor limits remain gauge-specific: tachometer = 3-pulse moving average + Motor `4800/12000`; speedometer = 4-pulse moving average + Motor `900/8000`.

v3.0の上位制御は、制御周期 `50 ms`、TARGET `1600/1600`、Virtual VEL `425/160`、Virtual ACC `2600/1500`、STOP BAND `0` を共通化しています。入力フィルタとMotor値はメーター固有で、タコは3パルス移動平均＋Motor `4800/12000`、スピードは4パルス移動平均＋Motor `900/8000` です。

## Included gauges / 収録メーター

### Tachometer / タコメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Previous generation with Original + Tuned configurations / 初期版＋Tuned版を持つ従来世代 |
| **v3.0** | X27.168 + DRV8833, 1/16 microstep | **Current vehicle-validated public version / 実車確認済み現行公開版** |

v3.0 replaces the pulse-by-pulse IIR path with a 3-pulse moving average, changes the control interval to 50 ms, relaxes TARGET to 1600/1600, disables STOP BAND with value 0, and uses Motor `4800/12000`. See `tachometer/drv8833/tach_v3_0/README.md`.

v3.0ではパルス毎IIRを3パルス移動平均へ変更し、制御周期50 ms、TARGET 1600/1600、STOP BAND 0、Motor `4800/12000` としています。詳細は `tachometer/drv8833/tach_v3_0/README.md` を参照してください。

v2.1 remains available as the previous generation because its Original and Tuned configurations document the earlier control architecture and tuning path.

v2.1は、旧制御アーキテクチャとOriginal／Tunedの調整経緯を残す従来世代として引き続き収録します。

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Speedometer / スピードメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Previous generation with Original + Tuned 7:1 + Tuned 3:1 configurations / 初期版＋Tuned 7:1＋Tuned 3:1を持つ従来世代 |
| **v3.0** | X27.168 + DRV8833, 1/16 microstep | **Current vehicle-validated public version / 実車確認済み現行公開版** |

v3.0 replaces the 3:1 IIR with a 4-pulse moving average, relaxes TARGET to 1600/1600, aligns the Virtual layer with tachometer v3.0, disables STOP BAND with value 0, and retains speedometer-specific Motor `900/8000`. See `speedometer/drv8833/speed_v3_0/README.md`.

v3.0では3:1 IIRを4パルス移動平均へ変更し、TARGET 1600/1600、Virtual層をタコv3.0と共通化、STOP BAND 0とし、Motorはスピード固有の `900/8000` を維持しています。詳細は `speedometer/drv8833/speed_v3_0/README.md` を参照してください。

v2.1 remains available because its Original, Tuned 7:1 and Tuned 3:1 configurations document the previous tuning trade-offs.

v2.1は、Original、Tuned 7:1、Tuned 3:1で検討した従来の平滑性・応答性のトレードオフを残す世代として引き続き収録します。

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Fuel & coolant temperature / 燃料計・水温計

Only the latest formal version is included. Experimental and intermediate versions remain in the private development repository.

正式版の最新版のみ収録します。実験版・途中版は開発用Privateリポジトリに残します。

## Version and parameter-variant policy / バージョンとパラメータvariantの方針

Version numbers represent meaningful software-architecture or implementation changes. Parameter-only alternatives inside the same architecture are maintained as `variants/` under the same version instead of incrementing the version number.

バージョン番号は、ソフトウェア構造や実装に意味のある変更がある場合に使用します。同じ制御構造でパラメータだけが異なる場合はバージョン番号を上げず、同一バージョン内の `variants/` として管理します。

v3.0 is a new version because the input filtering, control interval/role of TARGET, STOP BAND behavior, and motion-layer design were reorganized rather than merely retuned.

v3.0は単なるパラメータ変更ではなく、入力フィルタ、制御周期とTARGET層の役割、STOP BANDの扱い、運動層の設計を再整理したため、新バージョンとしています。

## Vehicle pulse input conditioning / 車両パルス入力処理

The tachometer and speedometer software expects a conditioned logic-level pulse at the microcontroller input. It does **not** assume that the original vehicle tachometer or speed signal can be connected directly to an Arduino input pin.

タコメーターおよびスピードメーターのソフトウェアは、マイコン入力に**整形済みのロジックレベルパルス**が入力されることを前提としています。車両側のタコ信号またはスピード信号をArduino入力ピンへ直接接続することを前提としていません。

Vehicle signals may exceed the allowable MCU input voltage and may contain negative excursions, transients, ringing, or electrical noise. Appropriate input protection, voltage limiting / level conversion, and waveform shaping must therefore be provided according to the actual signal measured at the installation point.

車両信号はマイコンの許容入力電圧を超える場合があり、負方向の電圧、サージ、リンギング、電気ノイズ等を含む可能性があります。そのため、実際の信号取り出し位置で確認した波形に応じて、入力保護、電圧制限／レベル変換、波形整形を行う必要があります。

The tachometer and speedometer signals do not necessarily have the same electrical characteristics, so a single identical input circuit should not be assumed suitable for both. See `docs/hardware.md` for the hardware-side requirements and design considerations.

タコ信号とスピード信号は電気的特性が同一とは限らないため、両者に同じ入力回路をそのまま使用できるとは限りません。ハードウェア側の要件と設計上の考え方は `docs/hardware.md` を参照してください。

## Key-off power hold requirement / キーOFF後の電源保持要件

The tachometer and speedometer control circuits must not lose power immediately when the ignition key is switched OFF. The controller and motor-driver power supply must be held for a short period after key-off so that the software can detect the key-off condition and complete its required shutdown / needle-position handling. The key-off signal must therefore be detected independently of the held power rail.

タコメーターおよびスピードメーターの制御回路は、イグニッションキーをOFFにした瞬間に電源が失われない構成としてください。ソフトウェアがキーOFFを検出し、必要な終了処理・針位置処理を完了できるよう、キーOFF後も短時間、マイコンおよびモータードライバへの電源を保持する必要があります。そのため、キーOFF検出信号は電源保持される系統とは独立して検出できる構成が必要です。

A capacitor, EDLC, or equivalent hold-up circuit may be used. The required hold time depends on the actual circuit, motor drive, and software version, so it must be verified on the completed hardware.

電源保持にはコンデンサ、EDLC、または同等の保持回路を使用できます。必要な保持時間は実際の回路、モーター駆動条件、ソフトウェアバージョンによって異なるため、完成したハードウェアで確認してください。

See `docs/hardware.md` for the hardware-side implementation notes.

ハードウェア側の実装上の注意は `docs/hardware.md` を参照してください。

## Repository structure / 構成

```text
tachometer/
  switecx25/
    tach_v1_1/
      tach_v1_1.ino
  drv8833/
    tach_v2_0/
      tach_v2_0.ino
    tach_v2_1/
      tach_v2_1.ino
      README.md
      variants/
        README.md
        tach_v2_1_tuned/
          tach_v2_1_tuned.ino
    tach_v3_0/
      README.md
      tach_v3_0.ino
speedometer/
  switecx25/
    speed_v1_1/
      speed_v1_1.ino
  drv8833/
    speed_v2_0/
      speed_v2_0.ino
    speed_v2_1/
      speed_v2_1.ino
      README.md
      variants/
        README.md
        speed_v2_1_tuned_7to1/
          speed_v2_1_tuned_7to1.ino
        speed_v2_1_tuned_3to1/
          speed_v2_1_tuned_3to1.ino
    speed_v3_0/
      README.md
      speed_v3_0.ino
fuel-coolant/
  fuel_temp_v1_0/
    fuel_temp_v1_0.ino
docs/
```

Each Arduino sketch directory uses the same base name as its main `.ino` file, so the sketches can be opened directly in the Arduino IDE.

各Arduinoスケッチは、フォルダ名とメイン`.ino`ファイル名のベース名を一致させており、そのままArduino IDEで開ける構成です。

Version numbers and maintained parameter configurations are kept consistent between the private development repository and this release-oriented repository.

Private開発リポジトリと本リポジトリで、バージョン番号と維持対象のパラメータ構成を対応させます。

## Development and release policy / 開発・公開方針

The private `honda-beat-arduino-projects` repository is the development archive and contains logging tools, analysis records, simulation-derived tests, debug builds, rejected approaches, and maintained versions. This repository contains only the selected usable configurations and documentation needed by users.

Privateの `honda-beat-arduino-projects` は開発母艦で、ログ取得、解析記録、シミュレーション由来のテスト、デバッグ版、不採用案、維持版を保持します。本リポジトリには、その中から選定した利用可能な構成と利用者向け説明だけを整理して収録します。

## Safety and disclaimer / 安全上の注意・免責

These projects are personal experimental and research results for automotive electronics. Before using them in a vehicle, verify electrical safety, mechanical safety, functional safety, and compliance with all applicable laws and regulations for your own installation. The author provides no warranty and accepts no responsibility for vehicle damage, accidents, injury, legal non-compliance, or other loss arising from use of this project.

本プロジェクトは個人による自動車電子工作の実験・研究成果です。車両へ適用する場合は、電気的安全性、機械的安全性、機能安全性、および適用される法令・規則への適合を利用者自身で確認してください。本プロジェクトの使用によって生じた車両故障、事故、負傷、法令不適合、その他の損害について、作者は保証・責任を負いません。

## License / ライセンス

Software in this repository is released under the MIT License unless otherwise stated. Third-party libraries retain their original licenses.

本リポジトリのソフトウェアは、特記のない限りMIT Licenseで公開します。第三者ライブラリには各ライブラリ固有のライセンスが適用されます。
