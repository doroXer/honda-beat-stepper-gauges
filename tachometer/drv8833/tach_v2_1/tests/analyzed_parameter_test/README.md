# Tachometer v2.1 analyzed parameter test / タコメーター v2.1 解析パラメータテスト

## Purpose / 目的

This test sketch validates parameter candidates derived from real-vehicle log analysis and simulation. The production `tach_v2_1.ino` is not modified.

このテストコードは、実車ログ解析とシミュレーションから導出した候補パラメータを実車で検証するためのものです。現行の `tach_v2_1.ino` 本体は変更していません。

## Base / ベース

- `tachometer/drv8833/tach_v2_1/tach_v2_1.ino`
- DRV8833 + X27.168, 1/16 microstep / DRV8833 + X27.168、1/16マイクロステップ

## Parameter changes / パラメータ変更

| Parameter | v2.1 baseline | Test value |
|---|---:|---:|
| TARGET UP | 200 | 400 |
| TARGET DOWN | 100 | 180 |
| VIRTUAL VEL UP | 150 | 400 |
| VIRTUAL VEL DOWN | 150 | 200 |
| VIRTUAL ACC UP | 500 | 2000 |
| VIRTUAL ACC DOWN | 500 | 1500 |
| Control update interval | 100 ms | 100 ms |

`VIRTUAL_STOP_BAND_STEP = 5.0` and the motor-layer parameters remain unchanged from the v2.1 baseline.

`VIRTUAL_STOP_BAND_STEP = 5.0` およびモーター層のパラメータは v2.1 本体から変更していません。

## Evaluation / 評価

Use this sketch for real-vehicle verification before deciding whether the analyzed parameters should be incorporated into the production version.

解析値を本体へ採用するか判断する前の実車検証用として使用します。
