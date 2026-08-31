# Speedometer v2.1 / スピードメーター v2.1

v2.1 is the 1/16-microstep DRV8833 speedometer generation. The control architecture is unchanged between the maintained configurations; the differences are parameter selection and, for one variant, display-filter weighting.

v2.1はDRV8833・1/16マイクロステップ駆動のスピードメーター世代です。維持する各構成で制御アーキテクチャは共通で、違いはパラメータ設定と、一構成のみ表示フィルタの重みです。

## Maintained configurations / 維持する構成

| Configuration | Sketch | Positioning / 位置づけ |
|---|---|---|
| Original v2.1 | `speed_v2_1.ino` | Slower, subjectively smoother / 緩慢で体感上より滑らか |
| Tuned 7:1 | `variants/speed_v2_1_tuned_7to1/speed_v2_1_tuned_7to1.ino` | Faster tracking + stronger smoothing / 追従性向上＋平滑性重視 |
| Tuned 3:1 | `variants/speed_v2_1_tuned_3to1/speed_v2_1_tuned_3to1.ino` | Faster display-filter response / 表示フィルタ応答性重視 |

The tuned variants use TARGET UP/DOWN `45/60`, MOTOR MAX SPEED `900`, and MOTOR ACCEL `8000`, versus `20/45`, `3600`, and `9600` in the original. Virtual-needle velocity/acceleration, stop band, speed conversion and 50 ms control interval remain unchanged.

Tuned版は、初期版の TARGET `20/45`、MOTOR MAX SPEED `3600`、MOTOR ACCEL `9600` に対し、それぞれ `45/60`、`900`、`8000` を使用します。仮想針速度・加速度、STOP BAND、速度換算、更新周期50 msは共通です。

Tuned 7:1 keeps the original display filter weighting. Tuned 3:1 changes only the weighting from `7:1` to `3:1`, where the notation means **previous filtered value : newest raw value**. It is not pulse division or a mechanical/motor gearing ratio.

Tuned 7:1は初期版と同じ表示フィルタ重みを使用します。Tuned 3:1はその重みだけを `7:1` から `3:1` に変更します。ここでの比率は **旧フィルタ値：新しい生値** であり、パルス分周や機械／モーターの減速比ではありません。

The tuned configurations were derived from real-vehicle logging, analysis and simulation, and then checked on the vehicle. Tracking improved and no clear step loss was observed at the current test stage. The original is intentionally retained because its slower movement may appear smoother. The 7:1 and 3:1 tuned filters also represent a smoothing-versus-response trade-off, so all three are maintained.

Tuned値は実車ログ、解析・シミュレーションから導出し、実車確認を行っています。追従性は向上し、現時点で明確な脱調は確認されていません。一方、初期版の緩慢な動きの方が滑らかに感じられる場合があります。またTuned 7:1と3:1にも平滑性と応答性のトレードオフがあるため、3構成すべてを残します。

See [`variants/README.md`](variants/README.md) for the comparison.

比較は [`variants/README.md`](variants/README.md) を参照してください。
