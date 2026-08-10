# Microstepping / マイクロステップ

The v2 tachometer and speedometer replace the SwitecX25 final motor-drive layer with a dedicated DRV8833 H-bridge drive for the X27.168 gauge motor. The upper display-control architecture remains based on the established logical gauge-position scale.

V2系のタコメーターとスピードメーターでは、SwitecX25の最終モーター駆動層を、X27.168用DRV8833 Hブリッジ専用駆動へ置き換えています。上位の表示制御は、確立済みの論理メーター位置スケールを維持します。

## v2.0 — 1/4 microstep / v2.0 — 1/4マイクロステップ

v2.0 uses 16 phase positions per electrical cycle. The existing v1 logical scale remains 0 to 720, and the motor boundary converts that logical coordinate into the finer motor coordinate.

v2.0は1電気周期を16位相で駆動します。V1系から継承した0～720の論理位置スケールはそのまま維持し、モーター駆動層の境界で細かいモーター座標へ変換します。

For the current implementation:

現在の実装では、

```text
3 logical steps = 8 quarter-microsteps
720 logical steps = 1920 quarter-microsteps
```

The 1/4-microstep version is retained as the stability-oriented v2 baseline because it has shown good practical behavior in vehicle testing.

1/4マイクロステップ版は、実車評価で良好な実用挙動を示したため、V2系の安定性重視の基準版として残しています。

## v2.1 — 1/16 microstep / v2.1 — 1/16マイクロステップ

v2.1 increases the phase resolution to 64 positions per electrical cycle.

v2.1では、1電気周期の位相分解能を64位置へ高めています。

For the current implementation:

現在の実装では、

```text
3 logical steps = 32 sixteenth-microsteps
720 logical steps = 7680 sixteenth-microsteps
```

The purpose is to reduce visible step granularity and improve smoothness. Finer microstepping also reduces the mechanical movement represented by each emitted motor step, so implementation details such as PWM, available torque, update timing, power supply, motor-driver condition, and direction reversal become more significant.

目的は、見えるステップ粒度を小さくして針の滑らかさを向上させることです。一方、マイクロステップを細かくすると1回のモーターステップが表す機械的移動量も小さくなるため、PWM、利用可能トルク、更新タイミング、電源、モータードライバの状態、方向反転処理などの影響が相対的に大きくなります。

During development, both 1/8 and 1/16 microstepping showed a greater tendency toward accumulated position drift in real driving than the 1/4 baseline, while opening-demo operation itself did not show the same problem. For this reason, v2.1 should be validated on the actual vehicle before being treated as a drop-in replacement for v2.0.

開発評価では、1/8および1/16マイクロステップは、オープニング動作では同様の問題が見られない一方、実走行では1/4版より累積的な位置ずれ傾向が現れやすいことを確認しています。そのため、v2.1はv2.0の単純な置き換えとして扱わず、実車で十分に検証してください。

## Why 1/8 is not a formal release / 1/8を正式版にしない理由

1/8 microstepping was useful as a development comparison, but it did not establish a sufficiently distinct release role. The 1/4 version remains the stability-oriented baseline, while 1/16 represents the higher-smoothness approach. The 1/8 test code therefore remains only in the private development archive.

1/8マイクロステップは開発上の比較には有用でしたが、公開版として独立した役割が十分明確ではありません。1/4版を安定性重視の基準、1/16版を高い滑らかさを狙う方式として位置づけ、1/8試験コードはPrivate開発履歴のみに残します。

## Important implementation principle / 重要な実装原則

The logical gauge-position calculation and the physical microstep coordinate are kept as separate layers. This avoids rewriting the calibrated upper-control logic merely because the motor-drive resolution changes.

論理的なメーター位置計算と、物理的なマイクロステップ座標は別レイヤーとして扱います。これにより、モーター駆動分解能を変更しても、校正済みの上位制御ロジック全体を書き換える必要がありません。
