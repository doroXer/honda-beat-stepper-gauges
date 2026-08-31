# Tachometer v2.1 / タコメーター v2.1

v2.1 is the 1/16-microstep DRV8833 tachometer generation. The control architecture is unchanged between the maintained configurations; the difference is parameter selection.

v2.1はDRV8833・1/16マイクロステップ駆動のタコメーター世代です。維持する各構成で制御アーキテクチャは共通で、違いはパラメータ設定です。

## Maintained configurations / 維持する構成

| Configuration | Sketch | Positioning / 位置づけ |
|---|---|---|
| Original v2.1 | `tach_v2_1.ino` | Slower, subjectively smoother / 緩慢で体感上より滑らか |
| Tuned | `variants/tach_v2_1_tuned/tach_v2_1_tuned.ino` | Faster tracking / 追従性重視 |

The tuned variant uses TARGET UP/DOWN `400/180`, VIRTUAL VEL UP/DOWN `400/200`, and VIRTUAL ACC UP/DOWN `2000/1500`, versus `200/100`, `150/150`, and `500/500` in the original. The 100 ms control interval and v2.1 motor layer are unchanged.

Tuned版は、初期版の TARGET `200/100`、VIRTUAL VEL `150/150`、VIRTUAL ACC `500/500` に対し、それぞれ `400/180`、`400/200`、`2000/1500` を使用します。更新周期100 msとv2.1モーター層は共通です。

The tuned parameters were derived from real-vehicle logging, analysis and simulation, and then checked on the vehicle. Tracking improved and no clear step loss was observed at the current test stage. The original is intentionally retained because its slower movement may appear smoother; the tuned configuration is an alternative, not a replacement.

Tuned値は実車ログ、解析・シミュレーションから導出し、実車確認を行っています。追従性は向上し、現時点で明確な脱調は確認されていません。一方、初期版の緩慢な動きの方が滑らかに感じられる場合があるため、Tuned版を初期版の置換とはせず両方を残します。

See [`variants/README.md`](variants/README.md) for the comparison.

比較は [`variants/README.md`](variants/README.md) を参照してください。
