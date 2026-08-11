# doroXer dark — Honda Beat Stepper Gauges

Honda Beat (PP1) stepper-gauge projects for tachometer, speedometer, fuel gauge, and coolant-temperature gauge.

ホンダ ビート（PP1）のタコメーター、スピードメーター、燃料計、水温計をステッピングモーター化するためのプロジェクトです。

> **Repository status / リポジトリ状態**  
> This repository is currently private and is being prepared for a future public release.  
> 現在はPrivateで公開準備中です。内容確認後にPublic化する予定です。

## doroXer dark

`doroXer dark` is the automotive / embedded-electronics side of doroXer. This repository publishes practical vehicle projects with the implementation details required to reproduce and understand them.

`doroXer dark` は、doroXerの自動車・組込み電子工作系の活動区分です。本リポジトリでは、再現・理解に必要な実装情報を含めた実用プロジェクトを整理します。

## Included gauges / 収録メーター

### Tachometer / タコメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Smoother microstep evolution / より滑らかな1/16マイクロステップ版 |

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Speedometer / スピードメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Smoother microstep evolution / より滑らかな1/16マイクロステップ版 |

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Fuel & coolant temperature / 燃料計・水温計

Only the latest formal version is included. Experimental and intermediate versions remain in the private development repository.

正式版の最新版のみ収録します。実験版・途中版は開発用Privateリポジトリに残します。

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

A capacitor, EDLC, or equivalent hold-up circuit may be used. The required hold time depends on the actual circuit, motor drive, and software version, so it must be verified on the completed hardware. Do not assume that removing ignition power and controller power at the same instant is acceptable.

電源保持にはコンデンサ、EDLC、または同等の保持回路を使用できます。必要な保持時間は実際の回路、モーター駆動条件、ソフトウェアバージョンによって異なるため、完成したハードウェアで確認してください。イグニッション電源と制御回路電源を同時に遮断する構成は前提としていません。

See `docs/hardware.md` for the hardware-side implementation notes.

ハードウェア側の実装上の注意は `docs/hardware.md` を参照してください。

## Repository structure / 構成

```text
tachometer/
  switecx25/v1.1/
  drv8833/v2.0/
  drv8833/v2.1/
speedometer/
  switecx25/v1.1/
  drv8833/v2.0/
  drv8833/v2.1/
fuel-coolant/
  latest/
docs/
```

Version numbers are preserved between the private development repository and this release-oriented repository. The same version number always refers to the same software code.

Private開発リポジトリと本リポジトリでバージョン番号は共通です。同一バージョン番号は同一のソフトウェアコードを指します。

## Development and release policy / 開発・公開方針

The private `honda-beat-arduino-projects` repository is the development archive and contains experiments, debug builds, rejected approaches, and formal versions. This repository contains only selected formal versions reorganized for users.

Privateの `honda-beat-arduino-projects` は開発母艦で、実験、デバッグ版、不採用案、正式版を保持します。本リポジトリには、その中から選定した正式版だけを利用者向けに再整理して収録します。

## Safety and disclaimer / 安全上の注意・免責

These projects are personal experimental and research results for automotive electronics. Before using them in a vehicle, verify electrical safety, mechanical safety, functional safety, and compliance with all applicable laws and regulations for your own installation. The author provides no warranty and accepts no responsibility for vehicle damage, accidents, injury, legal non-compliance, or other loss arising from use of this project.

本プロジェクトは個人による自動車電子工作の実験・研究成果です。車両へ適用する場合は、電気的安全性、機械的安全性、機能安全性、および適用される法令・規則への適合を利用者自身で確認してください。本プロジェクトの使用によって生じた車両故障、事故、負傷、法令不適合、その他の損害について、作者は保証・責任を負いません。

## License / ライセンス

Software in this repository is released under the MIT License unless otherwise stated. Third-party libraries retain their original licenses.

本リポジトリのソフトウェアは、特記のない限りMIT Licenseで公開します。第三者ライブラリには各ライブラリ固有のライセンスが適用されます。
