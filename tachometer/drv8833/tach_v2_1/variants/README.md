# Tachometer v2.1 parameter variants / タコメーター v2.1 パラメータバリエーション

v2.1 is maintained as one software architecture with multiple parameter configurations. Parameter tuning alone does not create a new software version.

v2.1は同一のソフトウェア構造を持つ1世代として扱い、パラメータ調整だけでは新しいバージョン番号を付けません。

| Configuration | TARGET UP/DOWN | VIRTUAL VEL UP/DOWN | VIRTUAL ACC UP/DOWN | Character / 特性 |
|---|---:|---:|---:|---|
| Original `tach_v2_1` | 200 / 100 | 150 / 150 | 500 / 500 | Slower; subjectively smoother / 緩慢で体感上より滑らか |
| `tach_v2_1_tuned` | 400 / 180 | 400 / 200 | 2000 / 1500 | Faster tracking / 追従性重視 |

Both use the same 100 ms control interval and the same v2.1 motor layer. The tuned configuration was derived from real-vehicle logging, analysis and simulation and then checked on the vehicle. Tracking improved and no clear step loss was observed in the current test. The original remains available because its slower motion can appear smoother.

両構成とも更新周期100 msと同一のv2.1モーター層を使用します。Tuned版は実車ログ、解析・シミュレーションを経て実車確認し、追従性が向上しました。現時点で明確な脱調は確認されていません。一方、初期版の緩慢な動きの方が滑らかに感じられる場合があるため、両方を公開します。
