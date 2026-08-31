# Changelog / 変更履歴

This changelog lists the formal versions and maintained parameter configurations selected for the release-oriented repository. Intermediate experiments remain in the private development repository.

本変更履歴は、公開対象として選定した正式版と維持対象のパラメータ構成を記載します。途中の実験版はPrivate開発リポジトリに残します。

## Tachometer / タコメーター

### v2.1

- Changed the DRV8833 motor layer from 1/4 to 1/16 microstepping while retaining the established upper-control architecture.
- Increased phase resolution to 64 positions per electrical cycle.
- Retained the original v2.1 parameter configuration as the slower, subjectively smoother option.
- Added maintained variant `tach_v2_1_tuned` without changing the v2.1 architecture: TARGET `400/180`, VIRTUAL VEL `400/200`, VIRTUAL ACC `2000/1500`; control interval and motor layer are unchanged.
- The tuned parameters were derived from real-vehicle logging, analysis and simulation, then checked on the vehicle. Tracking improved and no clear step loss was observed at the current test stage.
- Both configurations remain available because the slower original can appear smoother.

- 確立した上位制御アーキテクチャを維持し、DRV8833モーター駆動層を1/4から1/16マイクロステップへ変更。
- 1電気周期の位相分解能を64位置へ拡大。
- 初期v2.1パラメータを、緩慢だが体感上より滑らかな選択肢として継続公開。
- v2.1の制御構造を変えず、維持variant `tach_v2_1_tuned` を追加。TARGET `400/180`、VIRTUAL VEL `400/200`、VIRTUAL ACC `2000/1500`。更新周期とモーター層は共通。
- Tuned値は実車ログ、解析・シミュレーションから導出し実車確認済み。追従性は向上し、現時点で明確な脱調は確認されていない。
- 初期版の方が滑らかに感じられる場合があるため両構成を公開。

### v2.0

- Replaced the SwitecX25 final motor-drive layer with a dedicated DRV8833 1/4-microstep drive.
- Retained the v1.1 logical display-control architecture.

- SwitecX25の最終モーター駆動層をDRV8833専用1/4マイクロステップ駆動へ変更。
- v1.1の論理表示制御アーキテクチャを維持。

### v1.1

- Added managed 1 ms timing for `tachMotor.update()`.
- Added key-off to key-on re-entry handling for EDLC hold-up operation.

- `tachMotor.update()`を1ms周期で明示管理。
- EDLC保持中のキーOFF→キーON再復帰処理を追加。

> v1.0 was an internal development version and is not included.  
> v1.0は内部開発版のため収録していません。

## Speedometer / スピードメーター

### v2.1

- Evolved the DRV8833 motor layer to 1/16 microstepping while retaining the established pulse-processing and upper needle-control architecture.
- Retained the original v2.1 parameters as the slower, subjectively smoother option.
- Added `speed_v2_1_tuned_7to1`: TARGET `45/60`, MOTOR MAX SPEED `900`, MOTOR ACCEL `8000`, display filter `7:1`.
- Added `speed_v2_1_tuned_3to1` with the same tuned control parameters and only the display-filter weighting changed from `7:1` to `3:1`.
- `7:1` and `3:1` mean **previous filtered value : newest raw value**; they are not pulse division or mechanical/motor gearing ratios.
- The tuned parameters were derived from real-vehicle logging, analysis and simulation, then checked on the vehicle. Tracking improved and no clear step loss was observed at the current test stage.
- All three configurations remain available because the original may appear smoother and tuned 7:1 / 3:1 trade stronger smoothing against faster filter response.

- 確立したパルス処理・上位針制御アーキテクチャを維持し、DRV8833モーター駆動層を1/16マイクロステップへ発展。
- 初期v2.1パラメータを、緩慢だが体感上より滑らかな選択肢として継続公開。
- `speed_v2_1_tuned_7to1` を追加。TARGET `45/60`、MOTOR MAX SPEED `900`、MOTOR ACCEL `8000`、表示フィルタ `7:1`。
- 同じTuned制御値で表示フィルタ重みだけ `7:1` から `3:1` にした `speed_v2_1_tuned_3to1` を追加。
- `7:1` / `3:1` は **旧フィルタ値：新しい生値** の重み比であり、パルス分周や機械／モーター減速比ではない。
- Tuned値は実車ログ、解析・シミュレーションから導出し実車確認済み。追従性は向上し、現時点で明確な脱調は確認されていない。
- 初期版の方が滑らかに感じられる場合があり、Tuned 7:1 / 3:1にも平滑性と応答性の一長一短があるため、3構成すべてを公開。

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
