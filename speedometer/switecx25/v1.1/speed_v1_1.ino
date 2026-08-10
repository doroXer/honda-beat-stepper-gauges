#include "SwitecX25.h"

/*
  speed_v1.1
  Honda Beat speedometer stepper motor driver
  ホンダ・ビート用 スピードメーター ステッピングモーター駆動コード

  Hardware / ハード構成:
    - Arduino Pro Mini 5V / 16MHz
    - X27.168 compatible gauge stepper motor / X27.168系メーター用ステッピングモーター
    - SwitecX25 library / SwitecX25ライブラリ

  Control concept / 制御方針:
    This version keeps the stable startup opening behavior and key-off zero-return behavior
    from speed_v0.6, but rebuilds the normal display path as a new architecture.

    speed_v0.6の起動時オープニング動作とキーオフ時0戻し動作は維持し、
    通常表示中の針制御だけを新構成として再構成する。

  Control flow / 制御フロー:
    pulse interval acquisition
      -> raw speed value conversion from latest valid interval
      -> display filter
      -> step conversion
      -> target-step slew-rate limiting
      -> virtual needle control with velocity and acceleration limits
      -> X27.168 / SwitecX25 motor drive

    パルス周期取得
      -> 最新の有効パルス周期からraw値換算
      -> 表示用フィルタ
      -> step換算
      -> targetStep追従速度制限
      -> 仮想速度・加速度つき針制御
      -> X27.168 / SwitecX25駆動

  Version note / バージョン注記:
    This is the v1.1 architecture for the Honda Beat speedometer.
    Compared with v1.0, motor.update() is no longer called on every loop;
    it is called at a managed 1 ms interval to stabilize pointer motion.
    This version also keeps the key-off/key-on re-entry handling for EDLC hold-up operation.

    Honda Beatスピードメーター用のv1.1構成。
    v1.0に対して、motor.update()を毎loop呼び出しから1ms周期管理へ変更し、
    針の動きを安定化する。EDLC保持中のキーOFF→キーON復帰処理も保持する。
*/

// -----------------------------------------------------------------------------
// Pin assignment / ピン設定
// -----------------------------------------------------------------------------
const byte SPEED_PULSE_PIN = 2;  // Vehicle speed pulse input, external interrupt 0 / 車速パルス入力、外部割り込み0
const byte KEY_STATE_PIN   = 3;  // HIGH: key on, LOW: key off / HIGH: キーオン、LOW: キーオフ
const byte STATUS_LED_PIN  = 13; // Debug/status LED / デバッグ・状態表示LED

// -----------------------------------------------------------------------------
// Gauge motor / メーターモーター
// -----------------------------------------------------------------------------
const int MAX_METER_STEP = 240 * 3;  // X27.168: 3 steps/degree, 240 degree range / X27.168: 3step/度、240度範囲
SwitecX25 motor1(MAX_METER_STEP, 4, 5, 6, 7);

// -----------------------------------------------------------------------------
// Motor acceleration table / モーター加速度テーブル
// -----------------------------------------------------------------------------
// This is still used by SwitecX25 as the final motor-drive layer.
// v1.0 adds a virtual needle layer above this table.
//
// SwitecX25ライブラリ側の最終駆動層として残す。
// v1.0では、この上位に仮想針制御層を置く。
unsigned short accelTable[][2] = {
  { 20, 4000},
  { 50, 2200},
  {100, 1600},
  {150, 1200},
  {300,  950}
};
const byte accelTableSize = sizeof(accelTable) / sizeof(accelTable[0]);

// -----------------------------------------------------------------------------
// Speed conversion / 車速換算
// -----------------------------------------------------------------------------
// Derived from the original pulse-count calibration:
//   step = pulseHz * 7.48 - 36
// Because pulseHz = 1,000,000 / intervalUs:
//   step = 7,480,000 / intervalUs - 36
//
// v1.0 splits this into:
//   rawSpeedValue = 7,480,000 / intervalUs
//   filteredDisplayValue = displayFilter(rawSpeedValue)
//   targetStep = filteredDisplayValue - 36
//
// 元のパルスカウント方式の換算:
//   step = pulseHz * 7.48 - 36
// pulseHz = 1,000,000 / intervalUs なので:
//   step = 7,480,000 / intervalUs - 36
//
// v1.0ではこれを以下に分離する:
//   rawSpeedValue = 7,480,000 / intervalUs
//   filteredDisplayValue = displayFilter(rawSpeedValue)
//   targetStep = filteredDisplayValue - 36
const long SPEED_STEP_NUMERATOR = 7480000L;
const int  SPEED_STEP_OFFSET    = 36;

// -----------------------------------------------------------------------------
// Display filter / 表示用フィルタ
// -----------------------------------------------------------------------------
// Replaces the old pulse-interval filter and target-step limiter.
// Larger OLD ratio makes the displayed target calmer, smaller OLD ratio makes it more responsive.
//
// 旧構成のパルス間隔平均化と目標step整形を置き換える表示用フィルタ。
// OLD比率を大きくすると穏やかになり、小さくすると応答が速くなる。
const byte DISPLAY_FILTER_OLD = 7;
const byte DISPLAY_FILTER_NEW = 1;

// -----------------------------------------------------------------------------
// Virtual needle control / 仮想針制御
// -----------------------------------------------------------------------------
// The filtered target step is not sent directly to the motor.
// A virtual needle position and virtual needle velocity are updated at a fixed interval.
// Velocity and acceleration are limited here, so the old STEP_DEADBAND and per-update
// step limits from speed_v0.6 are intentionally removed.
//
// フィルタ後の目標stepをそのままモーターへ渡さず、一定周期で仮想針位置と
// 仮想針速度を更新する。速度・加速度制限をここに集約するため、speed_v0.6の
// STEP_DEADBANDと1回あたりstep変化制限は意図的に外す。
const unsigned long CONTROL_UPDATE_INTERVAL_US = 50000UL;  // 50 ms

// Keep the virtual needle fast and symmetric. The target itself is already smoothed
// by target-step slew-rate limiting, so the virtual layer should mainly make the
// motor motion natural without adding directional bias.
// targetStep側で追従速度制限を行うため、仮想針側は上下対称で速めにする。
const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC   = 500.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC = 500.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2    = 6000.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2  = 6000.0f;
const float VIRTUAL_STOP_BAND_STEP            = 3.0f;

// -----------------------------------------------------------------------------
// Pulse handling / パルス処理
// -----------------------------------------------------------------------------
// If no speed pulse arrives for this period, the raw value is treated as zero.
// この時間パルスが来なければ、車速0相当として扱う。
const unsigned long SPEED_PULSE_TIMEOUT_US = 500000UL;  // 500 ms

// Very short pulse intervals are ignored as noise.
// 極端に短いパルス間隔はノイズとして無視する。
const unsigned long MIN_VALID_INTERVAL_US = 1000UL;

// -----------------------------------------------------------------------------
// Target-step slew-rate limiting / targetStep追従速度制限
// -----------------------------------------------------------------------------
// The raw target step is not held by a dead band. Instead, the target sent to the
// virtual needle always moves toward the raw target, but its movement speed is
// limited. This avoids the stick-slip behavior that can occur with hold-band logic
// while still preventing slow target wobble from going directly to the needle.
//
// raw targetをデッドバンドで保持せず、仮想針へ渡すtargetStepを常にraw targetへ
// 向かわせる。ただしtargetStep自体の移動速度を制限することで、保持band方式で
// 起きやすい「溜めて放す」挙動を避けつつ、低周波の目標揺れを抑える。
const float TARGET_STEP_MAX_UP_PER_SEC   = 20.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC = 45.0f;

// -----------------------------------------------------------------------------
// Startup and zero-return behavior / 起動時・0点出し動作
// -----------------------------------------------------------------------------
// Kept from speed_v0.6.
// speed_v0.6から継承。
const unsigned long STARTUP_ZERO_DELAY_MS = 100UL;
const unsigned long STARTUP_KEY_STABILIZE_DELAY_MS = 100UL;
const int STARTUP_KEY_CHECK_COUNT = 5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS = 20;
const bool ENABLE_OPENING_DEMO = true;
const bool FORCE_ZERO_AFTER_OPENING = true;
const unsigned int OPENING_HOLD_MS = 200;

// -----------------------------------------------------------------------------
// Key re-entry confirmation / キーON再復帰確認
// -----------------------------------------------------------------------------
// EDLC hold-up can keep the MCU alive while the key input is unstable.
// Do not start opening from a single HIGH reading after key-off; require stable
// OFF first and stable ON afterward.
// EDLC保持中はキー入力が不安定な瞬間があるため、キーOFF後の単発HIGHで
// オープニングに入らない。OFF確定後、ONも一定時間連続確認してから再始動扱いにする。
const unsigned long KEY_REENTRY_OFF_CONFIRM_MS = 100UL;
const unsigned long KEY_REENTRY_ON_CONFIRM_MS  = 100UL;


// -----------------------------------------------------------------------------
// Motor update interval management / motor.update() 呼び出し間隔管理
// -----------------------------------------------------------------------------
// Do not use the temporary loop-end delay. Instead, call SwitecX25::update()
// at an explicit interval so the motor drive update timing is repeatable.
//
// 暫定のloop末尾delayは使わず、SwitecX25::update() を明示周期で呼ぶ。
// まずは 1000us = 1ms。必要に応じて 500us / 2000us で比較する。
const unsigned long MOTOR_UPDATE_INTERVAL_US = 1000UL;

// Minimal pulse counters retained only for normal pulse handling.
// 通常のパルス処理に必要な最小限のカウンタだけ残す。
volatile unsigned long speedPulseCountTotal = 0;
volatile unsigned long speedPulseCountValid = 0;
volatile unsigned long speedPulseCountRejectedShort = 0;
volatile unsigned long latestRawPulseIntervalMicros = 0;

// -----------------------------------------------------------------------------
// State variables / 状態変数
// -----------------------------------------------------------------------------
volatile unsigned long lastPulseMicros = 0;
volatile unsigned long lastPulseSeenMicros = 0;
volatile unsigned long latestPulseIntervalMicros = 0;
volatile bool pulseIntervalUpdated = false;

unsigned long rawSpeedValue = 0;      // Raw value before offset / offset前のraw値
float filteredDisplayValue = 0.0f;    // Filtered raw value before offset / offset前の表示用フィルタ値
int rawTargetStepFromDisplay = 0;     // Target step after offset before stabilization / 安定化前の目標step
float stabilizedTargetStep = 0.0f;    // Stabilized target step / 安定化後の目標step
int targetStepFromDisplay = 0;        // Final target step for virtual needle / 仮想針へ渡す最終目標step
bool targetSlewInitialized = false;

float virtualNeedleStep = 0.0f;
float virtualNeedleVelocity = 0.0f;
int lastCommandedStep = -1;

unsigned long lastControlUpdateUs = 0;
unsigned long lastMotorUpdateUs = 0;
bool speedTimeoutZeroActive = false;
bool keyWasOff = false;  // Tracks confirmed EDLC hold-up key-off -> key-on re-entry / EDLC保持中のキーOFF→ON復帰検出

bool keyOffConfirmActive = false;
unsigned long keyOffConfirmStartMs = 0;
bool keyOnConfirmActive = false;
unsigned long keyOnConfirmStartMs = 0;

void setup() {
  pinMode(SPEED_PULSE_PIN, INPUT);
  pinMode(KEY_STATE_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  motor1.accelTable = accelTable;
  motor1.maxVel = accelTable[accelTableSize - 1][0];

  initializeNeedleAtStartup();

  resetSpeedState();
  keyWasOff = !isKeyOn();
  attachInterrupt(digitalPinToInterrupt(SPEED_PULSE_PIN), onSpeedPulse, RISING);
}

void loop() {
  unsigned long nowUs = micros();
  updateMotorAtManagedInterval(nowUs);

  unsigned long nowMs = millis();

  if (!isKeyOn()) {
    // Key is currently OFF. Keep zero-return behavior immediately, but do not
    // mark this as a confirmed key-off until the input has stayed OFF long enough.
    // 現在キーOFF。0戻しは即時実行するが、一定時間OFFが継続するまで
    // 「再キーON待ち状態」としては確定しない。
    handleKeyOffZeroReturn();

    keyOnConfirmActive = false;

    if (!keyOffConfirmActive) {
      keyOffConfirmActive = true;
      keyOffConfirmStartMs = nowMs;
    }

    if ((nowMs - keyOffConfirmStartMs) >= KEY_REENTRY_OFF_CONFIRM_MS) {
      keyWasOff = true;
    }

    return;
  }

  // Key is currently ON. Clear the OFF confirmation candidate.
  // 現在キーON。OFF確定候補は解除する。
  keyOffConfirmActive = false;

  if (keyWasOff) {
    // Do not run opening from a single HIGH read after key-off. Require the key
    // input to remain ON for a short confirmation period.
    // キーOFF後の単発HIGHではオープニングしない。キーONが一定時間継続してから
    // EDLC保持中の再キーONとして扱う。
    if (!keyOnConfirmActive) {
      keyOnConfirmActive = true;
      keyOnConfirmStartMs = nowMs;
      return;
    }

    if ((nowMs - keyOnConfirmStartMs) < KEY_REENTRY_ON_CONFIRM_MS) {
      return;
    }

    keyOnConfirmActive = false;
    handleKeyOnReentry();
    keyWasOff = false;
  }

  digitalWrite(STATUS_LED_PIN, LOW);

  updateRawSpeedValueFromPulseInterval();

  nowUs = micros();
  if ((nowUs - lastControlUpdateUs) >= CONTROL_UPDATE_INTERVAL_US) {
    updateDisplayFilter();
    updateTargetStepFromDisplay();
    updateVirtualNeedleControl(nowUs);
    lastControlUpdateUs = nowUs;
  }
}

bool isKeyOn() {
  return digitalRead(KEY_STATE_PIN) != LOW;
}

bool isKeyOffConfirmedAtStartup() {
  for (int i = 0; i < STARTUP_KEY_CHECK_COUNT; i++) {
    if (isKeyOn()) {
      return false;
    }
    delay(STARTUP_KEY_CHECK_INTERVAL_MS);
  }
  return true;
}

void resetMotorDriveParameters() {
  // Restore SwitecX25 drive parameters explicitly because EDLC hold-up can resume
  // without a true MCU reset.
  // EDLC保持中はMCUが完全リセットされずに復帰することがあるため、駆動パラメータを明示的に戻す。
  motor1.accelTable = accelTable;
  motor1.maxVel = accelTable[accelTableSize - 1][0];
}

void resetNeedleRuntimeStateToZero() {
  // Align all upper-layer needle states to mechanical zero.
  // 上位の仮想針・target制御状態を機械的0点に揃える。
  rawSpeedValue = 0;
  filteredDisplayValue = 0.0f;
  rawTargetStepFromDisplay = 0;
  stabilizedTargetStep = 0.0f;
  targetStepFromDisplay = 0;
  targetSlewInitialized = false;
  virtualNeedleStep = 0.0f;
  virtualNeedleVelocity = 0.0f;
  lastCommandedStep = 0;
  lastControlUpdateUs = micros();
  lastMotorUpdateUs = micros();
  speedTimeoutZeroActive = false;
}

void handleKeyOnReentry() {
  keyOffConfirmActive = false;
  keyOnConfirmActive = false;
  // Key OFF -> ON can happen while EDLC keeps the Pro Mini alive. In that case
  // setup() is not called again, so opening-related state must be reset here.
  // キーOFF→ONがEDLC保持中に起きるとsetup()が再実行されないため、
  // オープニング関連状態をここで必ず初期化する。
  resetMotorDriveParameters();
  resetSpeedState();

  delay(STARTUP_ZERO_DELAY_MS);  // requested 100 ms / 指定の100ms
  forceZeroToStop();
  resetNeedleRuntimeStateToZero();

  if (ENABLE_OPENING_DEMO) {
    openingDemo();
  }

  if (FORCE_ZERO_AFTER_OPENING) {
    forceZeroToStop();
  }

  resetSpeedState();
  resetNeedleRuntimeStateToZero();
}

void initializeNeedleAtStartup() {
  resetMotorDriveParameters();

  // Wait a short time before reading KEY_STATE_PIN so the key signal and 5V rail can settle.
  // KEY_STATE_PINと5V電源が安定するまで短時間待ってから起動判定する。
  delay(STARTUP_KEY_STABILIZE_DELAY_MS);

  // Always establish mechanical zero first. Do not decide opening/no-opening from
  // a single key-input read at the exact power-up moment.
  // 起動直後の1回読みでオープニング可否を決めず、まず機械的0点を確立する。
  delay(STARTUP_ZERO_DELAY_MS);
  forceZeroToStop();
  resetSpeedState();
  resetNeedleRuntimeStateToZero();

  // If key-off is confirmed repeatedly at startup, skip opening and keep zero.
  // 起動時にキーOFFが複数回連続で確認された場合だけ、オープニングを中止して0保持する。
  if (isKeyOffConfirmedAtStartup()) {
    handleKeyOffZeroReturn();
    return;
  }

  if (ENABLE_OPENING_DEMO) {
    openingDemo();
  }

  if (FORCE_ZERO_AFTER_OPENING) {
    forceZeroToStop();
  }

  resetSpeedState();
  resetNeedleRuntimeStateToZero();
}

void forceZeroToStop() {
  // Physically press the needle against the zero stopper, then align software state to zero.
  // 針を物理ストッパーへ押し当てた後、ソフト側の状態も0に揃える。
  motor1.zero();
  motor1.updateBlocking();
  motor1.setPosition(0);
  lastCommandedStep = 0;
}

void openingDemo() {
  // Full-scale opening sweep. The final forceZeroToStop() corrects any slip after this movement.
  // 最大側まで振るオープニング動作。後段のforceZeroToStop()でズレを補正する。
  motor1.setPosition(MAX_METER_STEP);
  motor1.updateBlocking();
  delay(OPENING_HOLD_MS);

  motor1.setPosition(0);
  motor1.updateBlocking();
  delay(OPENING_HOLD_MS);
}

void resetSpeedState() {
  noInterrupts();
  lastPulseMicros = 0;
  lastPulseSeenMicros = 0;
  latestPulseIntervalMicros = 0;
  latestRawPulseIntervalMicros = 0;
  pulseIntervalUpdated = false;
  interrupts();

  rawSpeedValue = 0;
  filteredDisplayValue = 0.0f;
  rawTargetStepFromDisplay = 0;
  stabilizedTargetStep = 0.0f;
  targetStepFromDisplay = 0;
  targetSlewInitialized = false;
  virtualNeedleStep = 0.0f;
  virtualNeedleVelocity = 0.0f;
  lastControlUpdateUs = micros();
  lastMotorUpdateUs = micros();
  speedTimeoutZeroActive = false;
}

void handleKeyOffZeroReturn() {
  // During key-off, clear speed-related state every loop and keep issuing zero command.
  // キーオフ中は車速関連状態を毎回クリアし、0戻し指令を継続する。
  // This preserves the stable speed_v0.6 behavior during capacitor hold-up.
  // キャパシタ保持中の安定したspeed_v0.6相当の挙動を維持する。
  resetSpeedState();

  motor1.setPosition(0);
  lastCommandedStep = 0;
  updateMotorAtManagedInterval(micros());

  digitalWrite(STATUS_LED_PIN, HIGH);
}

void updateRawSpeedValueFromPulseInterval() {
  unsigned long nowUs = micros();
  unsigned long intervalCopy = 0;
  unsigned long lastSeenCopy = 0;
  bool hasUpdatedInterval = false;

  // Snapshot the latest valid pulse interval from ISR.
  // ISR側で更新される最新の有効パルス周期をスナップショットする。
  noInterrupts();
  intervalCopy = latestPulseIntervalMicros;
  lastSeenCopy = lastPulseSeenMicros;
  hasUpdatedInterval = pulseIntervalUpdated;
  pulseIntervalUpdated = false;
  interrupts();

  unsigned long ageUs = 0;
  if (lastSeenCopy == 0) {
    ageUs = 0;
  } else if (nowUs >= lastSeenCopy) {
    ageUs = nowUs - lastSeenCopy;
  } else {
    // Guard against ISR timing around micros() snapshot.
    // micros()取得前後のISR更新によるunsignedアンダーフロー対策。
    ageUs = 0;
  }

  bool timeoutNow = (lastSeenCopy == 0 || ageUs > SPEED_PULSE_TIMEOUT_US);
  unsigned long rawBeforeTimeout = rawSpeedValue;
  unsigned long rawAfterTimeout = rawSpeedValue;

  // No pulse has arrived yet, or no pulse for timeout period: speed is treated as zero.
  // まだパルスが来ていない、または一定時間パルスがない場合は0扱いにする。
  if (timeoutNow) {
    rawAfterTimeout = 0;
    rawSpeedValue = 0;
    speedTimeoutZeroActive = true;
  } else if (hasUpdatedInterval && intervalCopy >= MIN_VALID_INTERVAL_US) {
    // No input cleanup is applied in this observation version.
    // この観測版では入力補正を一切かけない。
    unsigned long value = SPEED_STEP_NUMERATOR / intervalCopy;
    rawBeforeTimeout = constrain(value, 0UL, (unsigned long)(MAX_METER_STEP + SPEED_STEP_OFFSET));
    rawAfterTimeout = rawBeforeTimeout;
    rawSpeedValue = rawAfterTimeout;
    speedTimeoutZeroActive = false;
  }

}

void updateDisplayFilter() {
  // When speed is zero, let the filter decay toward zero instead of forcing the needle instantly.
  // 車速0時も瞬時に針を落とさず、表示フィルタを通して0へ近づける。
  if (filteredDisplayValue <= 0.0f) {
    filteredDisplayValue = (float)rawSpeedValue;
    return;
  }

  filteredDisplayValue =
    (filteredDisplayValue * DISPLAY_FILTER_OLD + (float)rawSpeedValue * DISPLAY_FILTER_NEW) /
    (DISPLAY_FILTER_OLD + DISPLAY_FILTER_NEW);

  if (speedTimeoutZeroActive && filteredDisplayValue < 0.5f) {
    filteredDisplayValue = 0.0f;
  }
}

void updateTargetStepFromDisplay() {
  float calcStep = filteredDisplayValue - (float)SPEED_STEP_OFFSET;
  rawTargetStepFromDisplay = constrain((int)(calcStep + 0.5f), 0, MAX_METER_STEP);

  // Initialize the slew-limited target cleanly on first valid update.
  // 最初の有効更新では、追従制限targetを素直に初期化する。
  if (!targetSlewInitialized) {
    stabilizedTargetStep = (float)rawTargetStepFromDisplay;
    targetSlewInitialized = true;
  } else {
    const float dtSec = CONTROL_UPDATE_INTERVAL_US / 1000000.0f;
    float diff = (float)rawTargetStepFromDisplay - stabilizedTargetStep;

    if (diff > 0.0f) {
      float maxUp = TARGET_STEP_MAX_UP_PER_SEC * dtSec;
      stabilizedTargetStep += minFloat(diff, maxUp);
    } else if (diff < 0.0f) {
      float maxDown = TARGET_STEP_MAX_DOWN_PER_SEC * dtSec;
      stabilizedTargetStep -= minFloat(-diff, maxDown);
    }
  }

  stabilizedTargetStep = constrainFloat(stabilizedTargetStep, 0.0f, (float)MAX_METER_STEP);
  targetStepFromDisplay = constrain((int)(stabilizedTargetStep + 0.5f), 0, MAX_METER_STEP);
}

void updateVirtualNeedleControl(unsigned long nowUs) {
  float dtSec = (nowUs - lastControlUpdateUs) / 1000000.0f;

  if (dtSec <= 0.0f || dtSec > 0.5f) {
    dtSec = CONTROL_UPDATE_INTERVAL_US / 1000000.0f;
  }

  float diff = (float)targetStepFromDisplay - virtualNeedleStep;

  if (absFloat(diff) <= VIRTUAL_STOP_BAND_STEP && absFloat(virtualNeedleVelocity) < 1.0f) {
    virtualNeedleStep = (float)targetStepFromDisplay;
    virtualNeedleVelocity = 0.0f;
    commandMotorPosition(targetStepFromDisplay);
    return;
  }

  float desiredVelocity = diff / dtSec;
  desiredVelocity = constrainFloat(
    desiredVelocity,
    -VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC,
     VIRTUAL_MAX_VEL_UP_STEP_PER_SEC
  );

  float maxVelocityChange;
  if (desiredVelocity > virtualNeedleVelocity) {
    maxVelocityChange = VIRTUAL_ACCEL_UP_STEP_PER_SEC2 * dtSec;
    virtualNeedleVelocity += minFloat(maxVelocityChange, desiredVelocity - virtualNeedleVelocity);
  } else {
    maxVelocityChange = VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 * dtSec;
    virtualNeedleVelocity -= minFloat(maxVelocityChange, virtualNeedleVelocity - desiredVelocity);
  }

  float nextStep = virtualNeedleStep + virtualNeedleVelocity * dtSec;

  if ((diff > 0.0f && nextStep > (float)targetStepFromDisplay) ||
      (diff < 0.0f && nextStep < (float)targetStepFromDisplay)) {
    nextStep = (float)targetStepFromDisplay;
    virtualNeedleVelocity = 0.0f;
  }

  virtualNeedleStep = constrainFloat(nextStep, 0.0f, (float)MAX_METER_STEP);
  commandMotorPosition((int)(virtualNeedleStep + 0.5f));
}

void commandMotorPosition(int step) {
  step = constrain(step, 0, MAX_METER_STEP);

  if (step != lastCommandedStep) {
    motor1.setPosition(step);
    lastCommandedStep = step;
  }
}

void onSpeedPulse() {
  // ISR records the latest raw interval and the latest valid pulse interval.
  // ISRでは最新raw周期と最新の有効パルス周期を記録する。
  unsigned long nowUs = micros();
  speedPulseCountTotal++;

  if (lastPulseMicros != 0) {
    unsigned long interval = nowUs - lastPulseMicros;
    latestRawPulseIntervalMicros = interval;

    if (interval >= MIN_VALID_INTERVAL_US) {
      latestPulseIntervalMicros = interval;
      pulseIntervalUpdated = true;
      speedPulseCountValid++;

      // No history storage is used in v1.1.
      // v1.1では履歴保存は使用しない。
    } else {
      speedPulseCountRejectedShort++;
    }
  }

  lastPulseMicros = nowUs;
  lastPulseSeenMicros = nowUs;
}

void updateMotorAtManagedInterval(unsigned long nowUs) {
  // Keep motor.update() from being called back-to-back at uncontrolled loop speed.
  // loop速度任せで連続呼び出しせず、明示した周期でだけ更新する。
  if ((nowUs - lastMotorUpdateUs) >= MOTOR_UPDATE_INTERVAL_US) {
    motor1.update();
    lastMotorUpdateUs = nowUs;
  }
}
float constrainFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float minFloat(float a, float b) {
  return (a < b) ? a : b;
}

float absFloat(float value) {
  return (value < 0.0f) ? -value : value;
}
