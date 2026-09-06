# Tachometer v2.1 parameter variants / タコメーター v2.1 パラメータバリエーション

v2.1 is maintained as one software architecture with multiple parameter configurations. Parameter tuning alone does not create a new software version.

v2.1は同一のソフトウェア構造を持つ1世代として扱い、パラメータ調整だけでは新しいバージョン番号を付けません。

| Configuration | TARGET UP/DOWN | VIRTUAL VEL UP/DOWN | VIRTUAL ACC UP/DOWN | MOTOR SPEED | MOTOR ACCEL | Character / 特性 |
|---|---:|---:|---:|---:|---:|---|
| Original `tach_v2_1` | 200 / 100 | 150 / 150 | 500 / 500 | 3600 | 9600 | Slower; subjectively smoother / 緩慢で体感上より滑らか |
| `tach_v2_1_tuned` | **425 / 160** | **425 / 160** | **2600 / 1500** | **4800** | **32000** | Faster tracking, validated / 追従性重視・検証済み |

Both retain the 100 ms control interval and pulse-by-pulse `7:1` display IIR. The Tuned configuration was derived from real-vehicle pulse logging and simulation and was accepted after the completed vehicle test was reviewed together with the later motion-layer analysis.

両構成とも更新周期100 ms、パルス毎 `7:1` 表示IIRを維持します。Tuned版は実車パルスログ解析・シミュレーションから導出し、既に完了していた実車試験を今回の運動層解析と合わせて再確認したうえで、検証済みの維持対象構成として採用しています。

The main design rationale is that upstream IIR/TARGET processing already creates a sufficiently smooth display trajectory; the motor is therefore given enough speed and acceleration to reproduce that trajectory without becoming an unintended additional smoothing layer. `4800/32000` approximately covers the maximum TARGET velocity/acceleration observed in representative vehicle logs.

設計上の要点は、上流のIIR/TARGET段階ですでに十分滑らかな表示軌跡が作られているため、Motor側ではその軌跡を余計に鈍らせない程度の速度・加速度能力を持たせることです。`4800/32000` は、代表実車ログで観測されたTARGET最大速度・最大加速度を概ねカバーする値です。

The theoretical `9600/192000` condition corresponds to a different objective—100 ms position-perfect tracking—and is not part of the maintained public Tuned configuration.

`9600/192000` は100 ms位置完全追従という別目的の理論参考値であり、公開Tuned構成には含めません。
