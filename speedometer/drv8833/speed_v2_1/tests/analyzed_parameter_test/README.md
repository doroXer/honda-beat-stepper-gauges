# Speedometer v2.1 analyzed parameter test / スピードメーター v2.1 解析パラメータテスト

## Purpose / 目的

This test sketch validates parameter candidates derived from real-vehicle log analysis and simulation. The production `speed_v2_1.ino` is not modified.

このテストコードは、実車ログ解析とシミュレーションから導出した候補パラメータを実車で検証するためのものです。現行の `speed_v2_1.ino` 本体は変更していません。

## Base / ベース

- `speedometer/drv8833/speed_v2_1/speed_v2_1.ino`
- DRV8833 + X27.168, 1/16 microstep / DRV8833 + X27.168、1/16マイクロステップ

## Parameter changes / パラメータ変更

| Parameter | v2.1 baseline | Test value |
|---|---:|---:|
| TARGET UP | 20 | 45 |
| TARGET DOWN | 45 | 60 |
| VIRTUAL VEL UP | 500 | 500 |
| VIRTUAL VEL DOWN | 500 | 500 |
| VIRTUAL ACC UP | 6000 | 6000 |
| VIRTUAL ACC DOWN | 6000 | 6000 |
| VIRTUAL STOP BAND | 3 | 3 |
| MOTOR MAX SPEED | 3600 microstep/s | 900 microstep/s |
| MOTOR ACCEL | 9600 microstep/s² | 8000 microstep/s² |
| Control update interval | 50 ms | 50 ms |

The virtual-needle parameters are intentionally unchanged; the test focuses on the analyzed target slew rates and motor-layer limits.

Virtual needle層の値は意図的に変更せず、解析で得たtarget slewとモーター層の候補値を主に検証します。

## Evaluation / 評価

Use this sketch for real-vehicle verification before deciding whether the analyzed parameters should be incorporated into the production version.

解析値を本体へ採用するか判断する前の実車検証用として使用します。
