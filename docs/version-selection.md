# Version Selection / バージョン選択

This document explains the intended role of each formal gauge version included in this repository.

本ドキュメントでは、本リポジトリに収録する各正式版メーターソフトウェアの位置づけを整理します。

## Tachometer and speedometer / タコメーター・スピードメーター

| Version | Drive method / 駆動方式 | Intended role / 位置づけ |
|---|---|---|
| v1.1 | X27.168 + SwitecX25 | Simple, proven library-based implementation / シンプルで実績のあるライブラリ駆動版 |
| v2.0 | X27.168 + DRV8833, 1/4 microstep | Stable microstep baseline; current stability-oriented choice / 安定性を重視したマイクロステップ基準版 |
| v2.1 | X27.168 + DRV8833, 1/16 microstep | Higher smoothness, but requires careful vehicle-specific validation / より高い滑らかさを狙う版。実車ごとの十分な検証が必要 |

### v1.1

v1.1 uses the SwitecX25 library and keeps the motor-drive implementation comparatively simple. It is useful when simplicity, known behavior, and easier reproduction are more important than maximum smoothness.

v1.1はSwitecX25ライブラリを使用し、モーター駆動部分を比較的シンプルに構成しています。最大限の滑らかさより、構成の単純さ、既知の挙動、再現のしやすさを重視する場合に適しています。

v1.0 was an internal development version and is intentionally not included. The release-oriented v1 series starts with v1.1.

v1.0は内部開発版のため意図的に収録していません。公開対象のV1系はv1.1から開始します。

### v2.0 — 1/4 microstep / 1/4マイクロステップ

v2.0 replaces the SwitecX25 final drive layer with a dedicated DRV8833 microstep drive while retaining the established upper-control architecture. It is the stability-oriented microstep baseline.

v2.0は、確立した上位制御アーキテクチャを維持しつつ、SwitecX25の最終駆動層をDRV8833による専用マイクロステップ駆動へ置き換えた版です。安定性を重視したマイクロステップの基準版です。

### v2.1 — 1/16 microstep / 1/16マイクロステップ

v2.1 increases microstep resolution to improve visual smoothness. During development, finer microstepping showed greater sensitivity to step-position drift under real driving conditions than the 1/4-microstep baseline. For that reason, v2.1 should be treated as an advanced option that requires validation on the actual gauge, driver module, power supply, and vehicle.

v2.1はマイクロステップ分解能を上げ、見た目の滑らかさを向上させる版です。開発評価では、細かいマイクロステップほど1/4マイクロステップ版に比べ、実走行時の位置ずれに対する感度が高い傾向が確認されています。そのためv2.1は、実際のメーター、ドライバモジュール、電源、車両で十分に評価した上で使用する高度な選択肢として位置づけます。

## Why 1/8-microstep test versions are not included / 1/8マイクロステップ試験版を収録しない理由

1/8-microstep code was used during development as an experimental comparison. It does not provide a sufficiently distinct release role between the stable 1/4 baseline and the smoother 1/16 version, so it remains only in the private development archive.

1/8マイクロステップ版は開発中の比較試験として使用しました。安定性重視の1/4版と滑らかさ重視の1/16版の間で、公開版として独立した役割が十分明確ではないため、Private開発履歴のみに残します。

## Fuel and coolant temperature / 燃料計・水温計

Only the latest formal version is included because older versions are superseded rather than representing alternative drive approaches.

燃料計・水温計は旧版が別方式として残るのではなく、最新版によって置き換えられる関係のため、正式版の最新版のみ収録します。
