# Speedometer v3.0 / スピードメーター v3.0

v3.0 is the public 1/16-microstep DRV8833 speedometer release derived from the private v3.0 release candidate and validated in the vehicle.

v3.0は、Private開発版のv3.0公開候補を実車確認後に公開へ昇格した、DRV8833・1/16マイクロステップ駆動のスピードメーター版です。

## Main control concept / 主な制御思想

```text
Speed pulse
  -> 4-pulse moving average
  -> 50 ms control tick
  -> TARGET 1600/1600
  -> Virtual needle 425/160, ACC 2600/1500
  -> STOP BAND 0
  -> Motor 900 / 8000
  -> Physical needle
```

The upstream and Virtual-layer concepts are aligned with tachometer v3.0, while the Motor limits remain speedometer-specific because the speedometer hardware showed different stable operating limits.

上流処理とVirtual層の設計思想はタコメーターv3.0と共通化しています。一方、Motor層はスピードメーター実機で確認した安定動作範囲が異なるため、スピードメーター固有値を維持しています。

## Parameters / パラメータ

| Item | v3.0 |
|---|---:|
| Display filter | 4-pulse moving average |
| Control interval | 50 ms |
| TARGET UP / DOWN | 1600 / 1600 logical step/s |
| Virtual VEL UP / DOWN | 425 / 160 logical step/s |
| Virtual ACC UP / DOWN | 2600 / 1500 logical step/s² |
| STOP BAND | 0 logical step |
| Motor position ratio | 32 / 3 |
| Motor MAX SPEED | 900 motor-position step/s |
| Motor ACCEL | 8000 motor-position step/s² |

## Changes from v2.1 Tuned 3:1 / v2.1 Tuned 3:1からの変更

- 3:1 IIR @ 50 ms -> 4-pulse moving average per accepted pulse
- TARGET 45/60 -> 1600/1600
- Virtual VEL 500/500 -> 425/160
- Virtual ACC 6000/6000 -> 2600/1500
- STOP BAND 3 -> 0
- Motor MAX SPEED 900 is retained
- Motor ACCEL 8000 is retained

## Why the Virtual values changed / Virtual値を変更した理由

Saved raw speed-pulse logs were re-simulated using the v3.0 4-pulse moving-average path. The previous speedometer Virtual values (VEL 500/500, ACC 6000/6000, STOP 3) were compared with the tachometer-aligned values.

Because the speedometer Motor layer is already the dominant physical limit, changing the Virtual values to 425/160 and 2600/1500 produced almost the same physical Motor trajectory in large transients. In steady sections, STOP BAND 0 slightly reduced repeated direction changes without worsening the overall movement.

保存済みRawスピードパルスログをv3.0の4パルス移動平均経路で再シミュレーションし、従来のVirtual値（VEL 500/500、ACC 6000/6000、STOP 3）と、タコメーターに合わせた値を比較しました。

スピードメーターではMotor層が既に実針の主要な制約となっているため、Virtualを425/160、2600/1500へ変更しても、大きな過渡変化で実Motor軌跡はほぼ同じでした。定常区間ではSTOP BAND 0により細かな方向反転がわずかに減少し、全体挙動の悪化は見られませんでした。

## Validation / 検証

The resulting v3.0 configuration was checked in the vehicle for normal operation, steady-speed behavior, acceleration/deceleration, startup, zero return, and key-off behavior.

v3.0構成について、通常動作、定常速度、加減速、起動、ゼロ戻し、キーOFF時の動作を実車確認しています。

## Missing-pulse handling / パルス抜け補正

Integer-multiple missing-pulse correction is not included. The moving-average path is used without that additional correction logic.

整数倍判定によるパルス抜け補正は実装していません。追加補正を入れず、移動平均による処理を採用しています。
