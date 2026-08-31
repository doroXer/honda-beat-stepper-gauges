# doroXer dark — Honda Beat Stepper Gauges

Honda Beat (PP1) stepper-gauge projects for tachometer, speedometer, fuel gauge, and coolant-temperature gauge.

ホンダ ビート（PP1）のタコメーター、スピードメーター、燃料計、水温計をステッピングモーター化するためのプロジェクトです。

## doroXer dark

`doroXer dark` is the automotive / embedded-electronics side of doroXer. This repository publishes practical vehicle projects with the implementation details required to reproduce and understand them.

`doroXer dark` は、doroXerの自動車・組込み電子工作系の活動区分です。本リポジトリでは、再現・理解に必要な実装情報を含めた実用プロジェクトを整理します。

## Included gauges / 収録メーター

### Tachometer / タコメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | One software generation with original + tuned parameter configurations / 初期版＋Tuned版を持つ同一ソフト世代 |

v2.1 keeps the original slower parameter set and also publishes a tuned configuration derived from real-vehicle logging, analysis and simulation. The tuned configuration improves tracking and no clear step loss has been observed in the current vehicle test, while the original may appear smoother because of its slower movement. Both are intentionally retained.

v2.1では初期の緩慢なパラメータセットに加え、実車ログ、解析・シミュレーションから導出したTuned構成を公開します。Tuned版は追従性が向上し、現時点で明確な脱調は確認されていません。一方、初期版の緩慢な動きの方が滑らかに感じられる場合があるため、両方を意図的に残します。

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Speedometer / スピードメーター

| Version | Motor drive | Positioning / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Stable library-based implementation / ライブラリ駆動の安定版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性重視のマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | One software generation with original + tuned 7:1 + tuned 3:1 parameter configurations / 初期版＋Tuned 7:1＋Tuned 3:1を持つ同一ソフト世代 |

The tuned speedometer configurations improve tracking and no clear step loss has been observed in the current vehicle tests. The original is retained because its slower movement may appear smoother. Tuned 7:1 and Tuned 3:1 are also both retained because stronger smoothing and faster filter response are a trade-off rather than a simple upgrade path.

スピードメーターのTuned構成は追従性が向上し、現時点で明確な脱調は確認されていません。初期版は緩慢な動きの方が滑らかに感じられる場合があるため残します。またTuned 7:1とTuned 3:1も、平滑性とフィルタ応答性の一長一短があるため両方を残します。

In the speedometer variant names, `7:1` and `3:1` mean the weighting ratio of **previous filtered value : newest raw value**. They do not mean pulse division, mechanical gearing, or motor-position scaling.

スピードメーターの `7:1` / `3:1` は **旧フィルタ値：新しい生値** の表示フィルタ重み比を意味します。パルス分周、機械減速比、モーター位置倍率ではありません。

v1.0 was an internal development version and is not included in this repository. The public-facing v1 series starts with v1.1.

v1.0は内部開発版のため収録しません。V1系の公開対象はv1.1からです。

### Fuel & coolant temperature / 燃料計・水温計

Only the latest formal version is included. Experimental and intermediate versions remain in the private development repository.

正式版の最新版のみ収録します。実験版・途中版は開発用Privateリポジトリに残します。

## Version and parameter-variant policy / バージョンとパラメータvariantの方針

Version numbers represent meaningful software-architecture or implementation changes. Parameter-only alternatives inside the same architecture are maintained as `variants/` under the same version instead of incrementing the version number.

バージョン番号は、ソフトウェア構造や実装に意味のある変更がある場合に使用します。同じ制御構造でパラメータだけが異なる場合はバージョン番号を上げず、同一バージョン内の `variants/` として管理します。

This is why the current tuned tachometer and speedometer configurations remain v2.1 rather than being renamed v2.2.

今回のタコ／スピードのTuned構成も、コード構造の新規性ではなくパラメータ差であるためv2.2とはせずv2.1内に残します。

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
