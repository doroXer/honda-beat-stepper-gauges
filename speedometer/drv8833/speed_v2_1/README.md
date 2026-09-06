# Speedometer v2.1 / スピードメーター v2.1

v2.1 is the 1/16-microstep DRV8833 speedometer generation. The control architecture is unchanged between the maintained configurations; the differences are parameter selection and display-filter weighting.

v2.1はDRV8833・1/16マイクロステップ駆動のスピードメーター世代です。維持する各構成で制御アーキテクチャは共通で、違いはパラメータ設定と表示フィルタ重みです。

## Maintained configurations / 維持する構成

| Configuration | Sketch | Positioning / 位置づけ |
|---|---|---|
| Original v2.1 | `speed_v2_1.ino` | Slower, subjectively smoother / 緩慢で体感上より滑らか |
| Tuned 7:1 | `variants/speed_v2_1_tuned_7to1/speed_v2_1_tuned_7to1.ino` | Faster tracking + stronger smoothing / 追従性向上＋平滑性重視 |
| Tuned 3:1 | `variants/speed_v2_1_tuned_3to1/speed_v2_1_tuned_3to1.ino` | Faster display-filter response / 表示フィルタ応答性重視 |

The tuned variants use TARGET UP/DOWN `45/60`, VIRTUAL VEL `500/500`, VIRTUAL ACC `6000/6000`, MOTOR MAX SPEED `900`, MOTOR ACCEL `8000`, STOP BAND `3`, and a `50 ms` control interval. Tuned 7:1 keeps the stronger display smoothing, while Tuned 3:1 changes the display-filter weighting from `7:1` to `3:1`.

Tuned版は TARGET `45/60`、VIRTUAL VEL `500/500`、VIRTUAL ACC `6000/6000`、MOTOR MAX SPEED `900`、MOTOR ACCEL `8000`、STOP BAND `3`、更新周期 `50 ms` を使用します。Tuned 7:1はより強い表示平滑化を維持し、Tuned 3:1は表示フィルタ重みを `7:1` から `3:1` へ変更します。

`7:1` / `3:1` mean **previous filtered value : newest raw value**. They are not pulse division, mechanical gearing, or motor-position scaling.

`7:1` / `3:1` は **旧フィルタ値：新しいRaw値** の重み比であり、パルス分周、機械減速比、モーター位置倍率ではありません。

## Design rationale / 設計根拠

The normal display path is:

```text
Speed pulse
    -> Raw speed value
    -> Fixed-period IIR filter
    -> TARGET
    -> Virtual needle
    -> Motor
    -> Physical needle
```

The real-vehicle log analysis showed that the dominant speedometer response limitation is the **display IIR**, not the motor maximum speed.

実車ログ解析では、スピードメーターの応答を主に支配していたのは **Motor最大速度ではなく表示IIR** でした。

Representative acceleration analysis gave approximately:

```text
7:1 IIR: MAE vs raw ≈ 12.7 logical step
3:1 IIR: MAE vs raw ≈  6.4 logical step
```

Thus the `7:1 -> 3:1` change produced the dominant response improvement.

したがって `7:1 -> 3:1` の変更が代表区間では最も大きな応答改善をもたらしました。

The TARGET maximum speed is only `45/60 logical step/s`. With the motor-position ratio `32/3`, the maximum motor-speed requirement is approximately `640 microstep/s`; the maintained `900 microstep/s` therefore already has adequate speed margin.

TARGET最大速度は `45/60 logical step/s` であり、Motor換算比 `32/3` では最大速度要求は約 `640 microstep/s` です。維持値 `900 microstep/s` は速度について既に十分な余裕があります。

Representative 3:1 analysis showed a local TARGET acceleration requirement around `15000–16000 microstep/s²`. A trajectory-matched motor candidate would therefore be about `900 / 16000`, but simulation showed only a small improvement over the maintained `900 / 8000`:

3:1の代表加速解析では、局所的なTARGET加速度要求は約 `15000–16000 microstep/s²` でした。この意味では `900 / 16000` が軌跡能力に整合しますが、維持値 `900 / 8000` に対する改善幅は小さい結果でした。

```text
3:1 IIR + Motor 900 / 8000  : MAE ≈ 6.38 step
3:1 IIR + Motor 900 / 16000 : MAE ≈ 5.98 step
```

For that reason, the practical maintained motor setting remains `900 / 8000`, and the IIR change is treated as the primary response improvement. Increasing motor acceleration would also transmit more small steady-state variation to the physical needle.

このため、実用維持値は `900 / 8000` のままとし、応答改善の主手段を3:1 IIRとします。Motor加速度を上げると定常時の細かな変動も実針へ伝わりやすくなるため、数値誤差だけでなく見え方も考慮しています。

See [`variants/speed_v2_1_tuned_3to1/README.md`](variants/speed_v2_1_tuned_3to1/README.md) for the detailed rationale and representative real-log figure.

詳細な設計根拠と代表実車ログFigureは [`variants/speed_v2_1_tuned_3to1/README.md`](variants/speed_v2_1_tuned_3to1/README.md) を参照してください。

## Validation / 検証

The tuned configurations were derived from real-vehicle logging, analysis and simulation and then checked on the vehicle. Tracking improved and no clear step loss was observed in the current tests. The Original remains available because its slower motion can appear smoother; Tuned 7:1 and Tuned 3:1 also remain available because stronger smoothing and faster response are a trade-off rather than a simple upgrade path.

Tuned構成は実車ログ、解析・シミュレーションを経て実車確認しています。追従性は向上し、現時点で明確な脱調は確認されていません。Originalは緩慢な動きの方が滑らかに感じられる場合があるため残し、Tuned 7:1とTuned 3:1も平滑性と応答性のトレードオフとして併存させます。

See [`variants/README.md`](variants/README.md) for the configuration comparison.

構成比較は [`variants/README.md`](variants/README.md) を参照してください。
