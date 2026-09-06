# Tachometer v2.1 / タコメーター v2.1

v2.1 is the 1/16-microstep DRV8833 tachometer generation. The control architecture is unchanged between the maintained configurations; the difference is parameter selection.

v2.1はDRV8833・1/16マイクロステップ駆動のタコメーター世代です。維持する各構成で制御アーキテクチャは共通で、違いはパラメータ設定です。

## Maintained configurations / 維持する構成

| Configuration | Sketch | Positioning / 位置づけ |
|---|---|---|
| Original v2.1 | `tach_v2_1.ino` | Slower, subjectively smoother / 緩慢で体感上より滑らか |
| Tuned | `variants/tach_v2_1_tuned/tach_v2_1_tuned.ino` | Faster tracking, vehicle-validated / 追従性重視・実車確認済み |

### Parameter comparison / パラメータ比較

| Parameter | Original v2.1 | Tuned |
|---|---:|---:|
| TARGET UP/DOWN | 200 / 100 | **425 / 160** |
| VIRTUAL VEL UP/DOWN | 150 / 150 | **425 / 160** |
| VIRTUAL ACC UP/DOWN | 500 / 500 | **2600 / 1500** |
| MOTOR MAX SPEED | 3600 | **4800** |
| MOTOR ACCEL | 9600 | **32000** |
| Control interval | 100 ms | 100 ms |
| Display IIR | 7:1 per pulse | 7:1 per pulse |

The Tuned configuration was derived from real-vehicle pulse logging and simulation, then accepted after the already completed vehicle test was reviewed together with the later motion-layer analysis. The Original configuration remains available because its slower movement can appear smoother.

Tuned版は、実車パルスログ解析とシミュレーションから導出し、既に実施済みの実車試験結果を今回の運動層解析と合わせて再確認したうえで、検証済み構成として採用しました。Original版は、より緩慢な動きが滑らかに感じられる場合があるため、選択肢として残します。

## Design rationale / 設計根拠

The display path is conceptually:

```text
Tach pulse
  -> pulse-by-pulse IIR
  -> TARGET
  -> Virtual needle
  -> Motor
  -> Physical needle
```

The important design point is that these layers have different responsibilities:

- **IIR** suppresses short-period pulse-to-pulse variation.
- **TARGET** defines the display trajectory that should be shown to the driver.
- **Virtual needle** converts the discrete 100 ms TARGET updates into velocity/acceleration-controlled motion.
- **Motor** should reproduce that motion without becoming an unintended additional low-pass filter.

各層の役割は次のように分けています。

- **IIR**: パルス単位の短周期変動を抑える。
- **TARGET**: ドライバーに見せたい表示軌跡を作る。
- **Virtual needle**: 100 msごとの離散TARGET点を速度・加速度を持つ針運動へ変換する。
- **Motor**: その運動を、下流側の能力不足で余計に鈍らせず実針へ再現する。

Representative real-vehicle log analysis showed approximately `400–425 logical step/s` maximum TARGET velocity and about `2900 logical step/s²` maximum TARGET acceleration. With the v2.1 `32/3` motor-position ratio, these correspond to roughly `4533 microstep/s` and `30933 microstep/s²`. The Tuned motor values `4800 / 32000` were therefore selected to slightly cover the motion already present in the TARGET trajectory.

代表的な実車ログ解析では、TARGET軌跡の最大速度は約 `400–425 logical step/s`、最大加速度は約 `2900 logical step/s²` でした。v2.1の位置換算比 `32/3` でMotor側へ換算すると約 `4533 microstep/s`、`30933 microstep/s²` に相当します。このためTunedでは、TARGET軌跡自体の運動能力をわずかに上回る `4800 / 32000` を採用しています。

A much higher theoretical condition around `9600 / 192000` was also examined for a different objective: reaching and stopping at every newly updated 100 ms TARGET position before the next update. That requirement can reduce positional lag, but it also asks the physical needle to accelerate/decelerate much more aggressively at each discrete update. It is therefore kept only as a theoretical reference, not as the public Tuned setting.

別の要求として、100 msごとに更新されるTARGET位置へ次の更新までに到達・停止する「完全位置追従」も検討しました。この場合は約 `9600 / 192000` が必要になりますが、各離散点へ向けて実針を局所的に強く加速・減速させるため、現在のTuned設定には採用していません。これは理論比較値としてのみ扱います。

For the maintained usable code, use [`variants/tach_v2_1_tuned/`](variants/tach_v2_1_tuned/). See [`variants/README.md`](variants/README.md) for the configuration summary.

維持対象の実用コードは [`variants/tach_v2_1_tuned/`](variants/tach_v2_1_tuned/) を使用してください。構成比較は [`variants/README.md`](variants/README.md) を参照してください。
