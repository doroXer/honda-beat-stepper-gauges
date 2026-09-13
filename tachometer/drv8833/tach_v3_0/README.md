# Tachometer v3.0 / タコメーター v3.0

v3.0 is the public 1/16-microstep DRV8833 tachometer release derived from the private v3.0 release candidate and validated in the vehicle.

v3.0は、Private開発版のv3.0公開候補を実車確認後に公開へ昇格した、DRV8833・1/16マイクロステップ駆動のタコメーター版です。

## Main control concept / 主な制御思想

```text
Tach pulse
  -> 3-pulse moving average
  -> 50 ms control tick
  -> TARGET 1600/1600
  -> Virtual needle 425/160, ACC 2600/1500
  -> STOP BAND 0
  -> Motor 4800 / 12000
  -> Physical needle
```

The redesign separates responsibilities more clearly than v2.1: the moving average handles the repeating input variation, TARGET is normally non-dominant, Virtual defines the requested trajectory, and Motor shapes the visible physical motion between 50 ms commands.

v2.1よりも各層の役割を明確に分離しています。入力周期ばらつきは移動平均で処理し、TARGETは通常動作で支配的にならない設定とし、Virtualで要求軌道を作り、50 ms間の実針の見え方をMotor層で整えます。

## Parameters / パラメータ

| Item | v3.0 |
|---|---:|
| Display filter | 3-pulse moving average |
| Control interval | 50 ms |
| TARGET UP / DOWN | 1600 / 1600 logical step/s |
| Virtual VEL UP / DOWN | 425 / 160 logical step/s |
| Virtual ACC UP / DOWN | 2600 / 1500 logical step/s² |
| STOP BAND | 0 logical step |
| Motor position ratio | 32 / 3 |
| Motor MAX SPEED | 4800 motor-position step/s |
| Motor ACCEL | 12000 motor-position step/s² |

`MOTOR_ACCEL=12000` corresponds to 1125 logical step/s² with the 32/3 position conversion.

## Changes from v2.1 Tuned / v2.1 Tunedからの変更

- 7:1 pulse IIR -> 3-pulse moving average
- Control interval 100 ms -> 50 ms
- TARGET 425/160 -> 1600/1600
- STOP BAND 5 -> 0
- Motor ACCEL 32000 -> 12000
- Virtual VEL 425/160 and ACC 2600/1500 are retained
- Motor MAX SPEED 4800 is retained

## Validation / 検証

The filter redesign and motor-acceleration selection were derived from stored raw-pulse logs, control-layer logging, and simulation. The resulting v3.0 configuration was then checked in the vehicle for steady RPM behavior, acceleration/deceleration, startup, engine-stop handling, and key-off return.

フィルタ再設計とMotor加速度は、保存済みRawパルスログ、制御層ログ、シミュレーションを基に選定し、その後、定常回転、加減速、起動、エンジン停止、キーOFF後のゼロ戻しを実車確認しました。

## Note on STOP BAND / STOP BANDについて

The STOP BAND code path remains in the source for future experimentation, but the parameter is set to zero in v3.0. Small target changes therefore go through the normal Virtual/Motor motion path instead of using the previous snap-to-target behavior.

将来の再評価用にSTOP BAND処理自体はコードに残していますが、v3.0では値を0として無効化しています。小さな変化も従来のTargetへのスナップ処理ではなく、通常のVirtual/Motor経路で処理します。

## Missing-pulse handling / パルス抜け補正

Integer-multiple missing-pulse correction is not included. The moving-average path is used without that additional correction logic.

整数倍判定によるパルス抜け補正は実装していません。追加補正を入れず、移動平均による処理を採用しています。
