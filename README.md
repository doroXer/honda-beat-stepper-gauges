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
