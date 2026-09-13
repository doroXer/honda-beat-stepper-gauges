# Version Selection / バージョン選択

This document explains the intended role of each formal gauge version included in this repository.

本ドキュメントでは、本リポジトリに収録する各正式版メーターソフトウェアの位置づけを整理します。

## Tachometer and speedometer / タコメーター・スピードメーター

| Version | Drive method / 駆動方式 | Intended role / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Simple, proven library-based implementation / シンプルで実績のあるライブラリ駆動版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline / 安定性を重視したマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Previous 1/16 generation with original and tuned parameter variants / 初期版・Tuned版を持つ従来1/16世代 |
| v3.0 | X27.168 + DRV8833, 1/16 microstep | Current vehicle-validated redesign using pulse-period moving average, relaxed TARGET and revised motion layering / 周期移動平均・TARGET緩和・運動層再整理を行った実車確認済み現行版 |

### v1.1

v1.1 uses the SwitecX25 library and keeps the motor-drive implementation comparatively simple. It is useful when simplicity, known behavior, and easier reproduction are more important than maximum smoothness.

v1.1はSwitecX25ライブラリを使用し、モーター駆動部分を比較的シンプルに構成しています。最大限の滑らかさより、構成の単純さ、既知の挙動、再現のしやすさを重視する場合に適しています。

v1.0 was an internal development version and is intentionally not included. The release-oriented v1 series starts with v1.1.

v1.0は内部開発版のため意図的に収録していません。公開対象のV1系はv1.1から開始します。

### v2.0 — 1/4 microstep / 1/4マイクロステップ

v2.0 replaces the SwitecX25 final drive layer with a dedicated DRV8833 microstep drive while retaining the established upper-control architecture. It is the stability-oriented microstep baseline.

v2.0は、確立した上位制御アーキテクチャを維持しつつ、SwitecX25の最終駆動層をDRV8833による専用マイクロステップ駆動へ置き換えた版です。安定性を重視したマイクロステップの基準版です。

### v2.1 — 1/16 microstep / 1/16マイクロステップ

v2.1 is the previous 1/16-microstep generation. It remains available because it documents the earlier control architecture and its tuned parameter variants.

v2.1は従来の1/16マイクロステップ世代です。旧制御アーキテクチャとTunedパラメータvariantを残す意味があるため、引き続き収録します。

### v3.0 — current public version / 現行公開版

v3.0 keeps 1/16-microstep DRV8833 drive but reorganizes the signal and motion-control responsibilities using saved real-vehicle raw logs, simulation, and vehicle validation.

Both tachometer and speedometer use a 50 ms control interval, TARGET `1600/1600`, Virtual VEL `425/160`, Virtual ACC `2600/1500`, and STOP BAND `0`. Their Motor limits remain gauge-specific.

The tachometer uses a 3-pulse moving average and Motor `4800/12000`. The speedometer uses a 4-pulse moving average and Motor `900/8000`.

v3.0はDRV8833・1/16マイクロステップ駆動を維持しつつ、保存済み実車Rawログ、シミュレーション、実車確認を基に信号処理と運動制御の役割を再整理した版です。

タコ・スピード共通で、制御周期50 ms、TARGET `1600/1600`、Virtual VEL `425/160`、Virtual ACC `2600/1500`、STOP BAND `0` を採用しています。Motor値は各メーター固有です。

タコは3パルス移動平均＋Motor `4800/12000`、スピードは4パルス移動平均＋Motor `900/8000` です。

## Why 1/8-microstep test versions are not included / 1/8マイクロステップ試験版を収録しない理由

1/8-microstep code was used during development as an experimental comparison. It does not provide a sufficiently distinct release role between the stable 1/4 baseline and the 1/16 versions, so it remains only in the private development archive.

1/8マイクロステップ版は開発中の比較試験として使用しました。安定性重視の1/4版と1/16版の間で、公開版として独立した役割が十分明確ではないため、Private開発履歴のみに残します。

## Fuel and coolant temperature / 燃料計・水温計

Only the latest formal version is included because older versions are superseded rather than representing alternative drive approaches.

燃料計・水温計は旧版が別方式として残るのではなく、最新版によって置き換えられる関係のため、正式版の最新版のみ収録します。
