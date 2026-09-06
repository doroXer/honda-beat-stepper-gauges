# Speedometer v2.1 parameter variants / スピードメーター v2.1 パラメータバリエーション

v2.1 is maintained as one software architecture with multiple parameter configurations. Parameter tuning alone does not create a new software version.

v2.1は同一のソフトウェア構造を持つ1世代として扱い、パラメータ調整だけでは新しいバージョン番号を付けません。

| Configuration | TARGET UP/DOWN | Display filter | MOTOR MAX SPEED | MOTOR ACCEL | Character / 特性 |
|---|---:|---:|---:|---:|---|
| Original `speed_v2_1` | 20 / 45 | 7:1 | 3600 | 9600 | Slower; subjectively smoother / 緩慢で体感上より滑らか |
| `speed_v2_1_tuned_7to1` | 45 / 60 | 7:1 | 900 | 8000 | Faster tracking with stronger smoothing / 追従性向上＋平滑性重視 |
| `speed_v2_1_tuned_3to1` | 45 / 60 | 3:1 | 900 | 8000 | Faster display-filter response / 表示フィルタ応答性重視 |

`7:1` and `3:1` mean **previous filtered value : newest raw value**. They do not mean pulse division, mechanical gearing, or motor-position scaling.

`7:1` と `3:1` は **旧フィルタ値：新しいRaw値** の重み比です。パルス分周、機械減速比、モーター位置倍率を意味しません。

## Design interpretation / 設計上の整理

Real-vehicle log analysis showed that the speedometer differs from the tachometer in its main bottleneck. The dominant response limitation was the **display IIR**, while motor maximum speed already had adequate margin.

実車ログ解析では、スピードメーターはタコメーターと主なボトルネックが異なり、応答を主に支配していたのは **表示IIR** でした。Motor最大速度には既に十分な余裕があります。

Representative acceleration analysis gave approximately:

```text
7:1 IIR: MAE vs raw ≈ 12.7 step
3:1 IIR: MAE vs raw ≈  6.4 step
```

The maximum TARGET velocity corresponds to only about `640 microstep/s`, below the maintained motor maximum speed of `900 microstep/s`.

TARGET最大速度をMotor換算すると約 `640 microstep/s` であり、維持値 `900 microstep/s` を下回ります。

A TARGET-trajectory-matched motor acceleration would be approximately `16000 microstep/s²`, but simulation showed only a small improvement over the maintained `8000 microstep/s²` (`6.38 -> 5.98 step` MAE in the representative 3:1 acceleration analysis). Therefore `900/8000` remains the practical maintained motor setting, while the 3:1 filter is the main response-oriented change.

TARGET軌跡の最大加速度まで能力上合わせるならMotor加速度は約 `16000 microstep/s²` ですが、代表3:1加速区間での改善は `6.38 -> 5.98 step` MAE程度と小さい結果でした。このためMotorは `900/8000` を実用維持値とし、3:1フィルタを主な応答改善と位置づけます。

The Original remains available because its slower motion can appear smoother. Tuned 7:1 remains available for stronger smoothing, and Tuned 3:1 is the response-oriented maintained configuration.

Originalは緩慢な動きが体感上滑らかに見える場合があるため残します。Tuned 7:1はより強い平滑性を重視する選択肢、Tuned 3:1は応答性重視の維持構成です。

Detailed rationale and representative real-log figure:

- [`speed_v2_1_tuned_3to1/README.md`](speed_v2_1_tuned_3to1/README.md)

詳細な設計根拠と代表実車ログFigure:

- [`speed_v2_1_tuned_3to1/README.md`](speed_v2_1_tuned_3to1/README.md)
