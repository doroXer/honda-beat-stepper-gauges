# Speedometer v2.1 parameter variants / スピードメーター v2.1 パラメータバリエーション

v2.1 is maintained as one software architecture with multiple parameter configurations. Parameter tuning alone does not create a new software version.

v2.1は同一のソフトウェア構造を持つ1世代として扱い、パラメータ調整だけでは新しいバージョン番号を付けません。

| Configuration | TARGET UP/DOWN | Display filter | MOTOR MAX SPEED | MOTOR ACCEL | Character / 特性 |
|---|---:|---:|---:|---:|---|
| Original `speed_v2_1` | 20 / 45 | 7:1 | 3600 | 9600 | Slower; subjectively smoother / 緩慢で体感上より滑らか |
| `speed_v2_1_tuned_7to1` | 45 / 60 | 7:1 | 900 | 8000 | Faster tracking with stronger smoothing / 追従性向上＋平滑性重視 |
| `speed_v2_1_tuned_3to1` | 45 / 60 | 3:1 | 900 | 8000 | Faster display-filter response / 表示フィルタ応答性重視 |

`7:1` and `3:1` mean **previous filtered value : newest raw value**. They do not mean pulse division, mechanical gearing, or motor-position scaling.

`7:1` と `3:1` は **旧フィルタ値：新しい生値** の重み比です。パルス分周、機械減速比、モーター位置倍率を意味しません。

The tuned configurations were derived from real-vehicle logging, analysis and simulation and then checked on the vehicle. Tracking improved and no clear step loss was observed in the current tests. The original remains available because its slower motion can appear smoother. Between the tuned variants, 7:1 and 3:1 trade smoothing against response, so all three configurations are published.

Tuned版は実車ログ、解析・シミュレーションを経て実車確認し、追従性が向上しました。現時点で明確な脱調は確認されていません。一方、初期版の緩慢な動きの方が滑らかに感じられる場合があります。またTuned 7:1と3:1にも平滑性と応答性のトレードオフがあるため、3構成すべてを公開します。
