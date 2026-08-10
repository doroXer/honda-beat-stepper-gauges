# Changelog / 変更履歴

This changelog lists only the formal versions selected for the release-oriented repository. Intermediate experiments are intentionally excluded.

本変更履歴は、公開準備用リポジトリへ選定した正式版のみを記載します。途中の実験版は意図的に除外しています。

## Tachometer / タコメーター

### v2.1

- Changed the DRV8833 motor layer from 1/4 to 1/16 microstepping while retaining the established upper-control architecture.
- Increased phase resolution to 64 positions per electrical cycle.

- 確立した上位制御アーキテクチャを維持し、DRV8833モーター駆動層を1/4から1/16マイクロステップへ変更。
- 1電気周期の位相分解能を64位置へ拡大。

### v2.0

- Replaced the SwitecX25 final motor-drive layer with a dedicated DRV8833 1/4-microstep drive.
- Retained the v1.1 logical display-control architecture.

- SwitecX25の最終モーター駆動層をDRV8833専用1/4マイクロステップ駆動へ変更。
- v1.1の論理表示制御アーキテクチャを維持。

### v1.1

- Added managed 1 ms timing for `tachMotor.update()`.
- Added key-off to key-on re-entry handling for EDLC hold-up operation.
- Kept the layered v1 normal-display architecture.

- `tachMotor.update()`を1ms周期で明示管理。
- EDLC保持中のキーOFF→キーON再復帰処理を追加。
- v1系の階層化された通常表示制御を維持。

> v1.0 was an internal development version and is not included.  
> v1.0は内部開発版のため収録していません。

## Speedometer / スピードメーター

### v2.1

- Evolved the DRV8833 motor layer to 1/16 microstepping.
- Retained the established pulse-processing and upper needle-control architecture.

- DRV8833モーター駆動層を1/16マイクロステップへ発展。
- 確立したパルス処理・上位針制御アーキテクチャを維持。

### v2.0

- Introduced DRV8833 1/4-microstep motor drive.
- Retained the v1.1 upper-control concept.

- DRV8833による1/4マイクロステップ駆動を導入。
- v1.1の上位制御思想を維持。

### v1.1

- Changed `motor.update()` from uncontrolled every-loop calls to a managed 1 ms interval.
- Retained key-off/key-on re-entry handling for EDLC hold-up operation.

- `motor.update()`を毎loop呼び出しから1ms周期管理へ変更。
- EDLC保持中のキーOFF→キーON復帰処理を維持。

> v1.0 was an internal development version and is not included.  
> v1.0は内部開発版のため収録していません。

## Fuel & coolant temperature / 燃料計・水温計

### v1.0

- FreeRTOS-based implementation using one gauge-drive task for both motors and separate logic tasks for fuel and coolant-temperature target calculation.
- Both gauges perform the visible opening sweep together, followed by sequential zero calibration.

- 2つのモーターを単一GaugeDriveタスクで駆動し、燃料・水温の目標計算を別ロジックタスクで行うFreeRTOS版。
- 見えるオープニング動作は2メーター同時、その後のゼロ点校正は順次実施。
