#include <SwitecX25.h>

/*
  Honda Beat tachometer prototype / tach_v1_1.

  This sketch keeps the startup opening demo and key-off behavior from tach_v0.5_calibrated,
  but rebuilds the normal needle-control path as a new architecture.

  Control flow:
    pulse interval acquisition
      -> engine-run confirmation
      -> raw tach value conversion
      -> display filter
      -> step conversion
      -> target-step slew-rate limiting
      -> virtual needle control with velocity and acceleration limits
      -> virtual stop band
      -> X27.168 / SwitecX25 motor drive

  ホンダビート タコメーター試作コード / tach_v1_1。

  起動時のオープニング動作とキーオフ時の動作は tach_v0.5_calibrated を継承し、
  通常表示時の針制御を新構成として再構成します。

  制御の流れ:
    パルス間隔取得
      -> 始動確認
      -> raw値化
      -> 表示用フィルタ
      -> step換算
      -> targetStep追従速度制限
      -> 仮想速度・加速度つき針制御
      -> 仮想停止バンド
      -> X27.168 / SwitecX25駆動
*/

const int MOTOR_STEPS = 240 * 3;
const int PIN_MOTOR_1 = 4;
const int PIN_MOTOR_2 = 5;
const int PIN_MOTOR_3 = 6;
const int PIN_MOTOR_4 = 7;
const int PIN_TACH_INPUT = 2;
const int PIN_KEY_ON = 3;
const int PIN_LED = 13;

SwitecX25 tachMotor(MOTOR_STEPS, PIN_MOTOR_1, PIN_MOTOR_2, PIN_MOTOR_3, PIN_MOTOR_4);

/*
  SwitecX25 motor acceleration table.
  This is still used by the library as the final motor-drive layer.
  The new virtual needle layer sits above this and limits the commanded position itself.

  SwitecX25ライブラリ側の加速テーブルです。
  最終的なモーター駆動層として残します。
  v1.0では、この上位に仮想針制御層を置き、指令位置そのものを滑らかにします。
*/
unsigned short tachAccelTable[][2] = {
  { 20, 4000 },
  { 50, 2200 },
  {100, 1600 },
  {150, 1200 },
  {300,  950 }
};
const byte tachAccelTableSize = sizeof(tachAccelTable) / sizeof(tachAccelTable[0]);

/*
  Raw tach value conversion.

  v0.5_calibrated converted pulse interval directly to step with:
    step = STEP_CONVERSION_NUMERATOR / intervalMicros - STEP_OFFSET

  v1.0 splits this into:
    rawTachValue = STEP_CONVERSION_NUMERATOR / intervalMicros
    filteredDisplayValue = displayFilter(rawTachValue)
    targetStep = filteredDisplayValue - STEP_OFFSET

  The raw value is intentionally kept in the same calibrated step-base scale as v0.5.
  This avoids adding an unverified pulse-per-rpm assumption while still separating
  raw acquisition, display filtering, and step conversion.

  v0.5_calibratedでは、パルス間隔から直接stepへ換算していました。
  v1.0では、以下のように分離します。
    rawTachValue = STEP_CONVERSION_NUMERATOR / intervalMicros
    filteredDisplayValue = displayFilter(rawTachValue)
    targetStep = filteredDisplayValue - STEP_OFFSET

  raw値は、v0.5と同じ校正済みstep基準の内部値として扱います。
  これにより、未確認のパルス数/rpm仮定を新たに入れずに、
  raw取得、表示フィルタ、step換算を分離できます。
*/
const int STEP_OFFSET = 23;
const long STEP_CONVERSION_NUMERATOR = 2952000L;

/*
  Display filter.
  This replaces the old pulse-interval filter.
  Larger OLD ratio makes the display target calmer, smaller OLD ratio makes it more responsive.

  表示用フィルタです。
  旧構成のパルス間隔フィルタを置き換えます。
  OLD比率を大きくすると表示目標は穏やかになり、小さくすると応答が速くなります。
*/
const byte DISPLAY_FILTER_OLD = 7;
const byte DISPLAY_FILTER_NEW = 1;

/*
  Virtual needle control layer.

  The filtered target step is not sent directly to the motor.
  A virtual needle position and virtual needle velocity are calculated every 100 ms.
  Velocity and acceleration are limited here, so the old step deadband,
  high-rpm drop limiter, and separate up/down step limits are intentionally removed.

  仮想針制御層です。

  フィルタ後の目標stepをそのままモーターへ渡さず、100msごとに
  仮想針位置と仮想針速度を計算します。
  速度制限・加速度制限をここに集約するため、旧構成のstepデッドバンド、
  高回転急落制限、上昇/下降step制限は意図的に外しています。
*/
const unsigned long CONTROL_UPDATE_INTERVAL_US = 100000UL;
const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC = 150.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC =150.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2 = 500.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 = 500.0f;
const float VIRTUAL_STOP_BAND_STEP = 5.0f;

/*
  Target-step slew-rate limiting.

  The filtered raw target is not sent directly to the virtual needle.
  Instead, the target step itself moves toward the raw target with limited speed.
  This follows the same direction as speed_v1 target-slew testing, adjusted for tachometer use.

  targetStep追従速度制限です。

  フィルタ後のraw targetを仮想針へ直接渡さず、targetStep自体を速度制限つきで
  raw targetへ追従させます。speed_v1で良好だったtarget slewの考え方を、
  タコメーター用に上昇30step/s、下降45step/sとして追加します。
*/
const float TARGET_STEP_MAX_UP_PER_SEC = 100.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC = 100.0f;

/*
  Startup and stop behavior.
  This block is kept from the stable architecture.

  起動・停止時の動作です。
  stable版の考え方を継承します。
*/
const bool ENABLE_OPENING_DEMO = true;
const bool FORCE_ZERO_AFTER_OPENING = true;
const int STARTUP_KEY_CHECK_COUNT = 5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS = 20;
const unsigned int STARTUP_KEY_SETTLE_DELAY_MS = 300;
const unsigned long NO_PULSE_STOP_TIMEOUT_US = 1000000UL;

/*
  Key re-entry confirmation and managed motor update interval.

  EDLC hold-up can keep the MCU alive while the key is switched OFF and ON again.
  This version detects confirmed key-off -> confirmed key-on re-entry and runs
  the opening demo without relying on setup() being called again.

  motor.update() is also called at an explicit 1ms interval, following the speed_v1
  stabilization test.

  キーON再復帰確認と motor.update() 周期管理です。

  EDLC保持中はキーOFF→ONしてもMCUがリセットされない場合があるため、
  setup()の再実行に頼らず、OFF確定→ON確定を検出してオープニングを実行します。

  また、speed_v1の安定化方針と同様に、motor.update()を明示的に1ms周期で呼びます。
*/
const unsigned long KEY_REENTRY_OFF_CONFIRM_MS = 100UL;
const unsigned long KEY_REENTRY_ON_CONFIRM_MS  = 100UL;
const unsigned long MOTOR_UPDATE_INTERVAL_US   = 1000UL;


/*
  Engine-run confirmation for cranking noise rejection.
  Tach pulses are not accepted into the display filter until several valid
  pulse intervals are seen consecutively. This prevents the first short or noisy
  cranking interval from becoming the initial display-filter value.

  クランキング時ノイズ対策用の始動確認です。
  妥当なパルス間隔が連続して入るまで、表示用フィルタへ値を入れません。
  これにより、クランキング初期の短いintervalやノイズが初期表示値になることを防ぎます。
*/
const byte ENGINE_RUN_CONFIRM_COUNT = 20;
const unsigned long MIN_VALID_INTERVAL_US = 3000UL;
const unsigned long MAX_VALID_INTERVAL_US = 300000UL;

volatile unsigned long lastPulseMicros = 0;
volatile unsigned long pulseIntervalMicros = 0;
volatile bool pulseIntervalReady = false;

unsigned long rawTachValue = 0;
float filteredDisplayValue = 0.0f;
int rawTargetStepFromDisplay = 0;
float stabilizedTargetStep = 0.0f;
int targetStepFromDisplay = 0;
bool targetSlewInitialized = false;

float virtualNeedleStep = 0.0f;
float virtualNeedleVelocity = 0.0f;
int lastCommandedStep = -1;

unsigned long lastControlUpdateMicros = 0;
unsigned long lastMotorUpdateMicros = 0;
bool keyWasOff = false;
bool keyOffConfirmActive = false;
unsigned long keyOffConfirmStartMs = 0;
bool keyOnConfirmActive = false;
unsigned long keyOnConfirmStartMs = 0;
bool keyOffZeroCommanded = false;
bool engineStopZeroDone = false;
bool engineRunConfirmed = false;
byte engineRunConfirmCounter = 0;

void setup() {
  pinMode(PIN_KEY_ON, INPUT_PULLUP);
  pinMode(PIN_TACH_INPUT, INPUT);
  pinMode(PIN_LED, OUTPUT);

  resetMotorDriveParameters();

  delay(STARTUP_KEY_SETTLE_DELAY_MS);
  forceZeroToStop();
  resetDisplayState();

  bool keyOffAtStartup = isKeyOffConfirmedAtStartup();
  keyWasOff = keyOffAtStartup;

  if (keyOffAtStartup) {
    returnNeedleToZero();
  } else {
    if (ENABLE_OPENING_DEMO) {
      openingDemo();
    }

    if (FORCE_ZERO_AFTER_OPENING) {
      forceZeroToStop();
    }

    resetDisplayState();
  }

  attachInterrupt(digitalPinToInterrupt(PIN_TACH_INPUT), onTachPulse, RISING);
}

void loop() {
  unsigned long nowMicros = micros();
  updateMotorAtManagedInterval(nowMicros);

  unsigned long nowMs = millis();

  if (!isKeyOn()) {
    // Key is currently OFF. Command zero return only once, then keep calling
    // tachMotor.update() at the managed 1ms interval from the top of loop().
    // Do not reset display/motor timing repeatedly while the key remains OFF.
    // 現在キーOFF。0戻し指令は最初の1回だけ出し、その後はloop先頭の
    // 1ms周期 tachMotor.update() で針を進めます。キーOFF中に表示状態や
    // motor update時刻を繰り返しリセットしないことが重要です。
    if (!keyOffZeroCommanded) {
      returnNeedleToZero();
      keyOffZeroCommanded = true;
    }

    digitalWrite(PIN_LED, HIGH);
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
  // 現在キーON。OFF確定候補は解除します。
  keyOffConfirmActive = false;
  keyOffZeroCommanded = false;

  if (keyWasOff) {
    // Do not run opening from a single HIGH read after key-off. Require the key
    // input to remain ON for a short confirmation period.
    // キーOFF後の単発HIGHではオープニングしません。キーONが一定時間継続してから
    // EDLC保持中の再キーONとして扱います。
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

  digitalWrite(PIN_LED, LOW);

  unsigned long intervalMicros = 0;
  bool hasNewInterval = false;
  unsigned long lastPulseCopy = 0;

  noInterrupts();
  if (pulseIntervalReady) {
    intervalMicros = pulseIntervalMicros;
    pulseIntervalReady = false;
    hasNewInterval = true;
  }
  lastPulseCopy = lastPulseMicros;
  interrupts();

  if (hasNewInterval) {
    engineStopZeroDone = false;

    if (!engineRunConfirmed) {
      if (!updateEngineRunConfirmation(intervalMicros)) {
        return;
      }

      updateRawValueFromPulseInterval(intervalMicros);
      initializeDisplayFilterFromRaw();
      updateTargetStepFromDisplay();
    } else {
      updateRawValueFromPulseInterval(intervalMicros);
      updateDisplayFilter();
      updateTargetStepFromDisplay();
    }
  }

  nowMicros = micros();
  if (lastPulseCopy > 0 && (nowMicros - lastPulseCopy) >= NO_PULSE_STOP_TIMEOUT_US) {
    if (!engineStopZeroDone) {
      handleEngineStopZeroReturn();
      engineStopZeroDone = true;
    }
    return;
  }

  if ((nowMicros - lastControlUpdateMicros) >= CONTROL_UPDATE_INTERVAL_US) {
    updateVirtualNeedleControl(nowMicros);
    lastControlUpdateMicros = nowMicros;
  }
}

bool updateEngineRunConfirmation(unsigned long intervalMicros) {
  if (!isValidTachInterval(intervalMicros)) {
    engineRunConfirmCounter = 0;
    return false;
  }

  if (engineRunConfirmCounter < ENGINE_RUN_CONFIRM_COUNT) {
    engineRunConfirmCounter++;
  }

  if (engineRunConfirmCounter >= ENGINE_RUN_CONFIRM_COUNT) {
    engineRunConfirmed = true;
    return true;
  }

  return false;
}

bool isValidTachInterval(unsigned long intervalMicros) {
  return (intervalMicros >= MIN_VALID_INTERVAL_US &&
          intervalMicros <= MAX_VALID_INTERVAL_US);
}

void initializeDisplayFilterFromRaw() {
  filteredDisplayValue = (float)rawTachValue;
}

void updateRawValueFromPulseInterval(unsigned long intervalMicros) {
  if (intervalMicros == 0) {
    return;
  }

  rawTachValue = STEP_CONVERSION_NUMERATOR / intervalMicros;
  rawTachValue = constrain(rawTachValue, 0UL, (unsigned long)(MOTOR_STEPS + STEP_OFFSET));
}

void updateDisplayFilter() {
  if (filteredDisplayValue <= 0.0f) {
    filteredDisplayValue = (float)rawTachValue;
    return;
  }

  filteredDisplayValue =
    (filteredDisplayValue * DISPLAY_FILTER_OLD + (float)rawTachValue * DISPLAY_FILTER_NEW) /
    (DISPLAY_FILTER_OLD + DISPLAY_FILTER_NEW);
}

void updateTargetStepFromDisplay() {
  float calcStep = filteredDisplayValue - (float)STEP_OFFSET;
  rawTargetStepFromDisplay = constrain((int)(calcStep + 0.5f), 0, MOTOR_STEPS);

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

  stabilizedTargetStep = constrainFloat(stabilizedTargetStep, 0.0f, (float)MOTOR_STEPS);
  targetStepFromDisplay = constrain((int)(stabilizedTargetStep + 0.5f), 0, MOTOR_STEPS);
}

void updateVirtualNeedleControl(unsigned long nowMicros) {
  float dtSec = (nowMicros - lastControlUpdateMicros) / 1000000.0f;

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

  virtualNeedleStep = constrainFloat(nextStep, 0.0f, (float)MOTOR_STEPS);
  commandMotorPosition((int)(virtualNeedleStep + 0.5f));
}

void commandMotorPosition(int step) {
  step = constrain(step, 0, MOTOR_STEPS);

  if (step != lastCommandedStep) {
    tachMotor.setPosition(step);
    lastCommandedStep = step;
  }
}

void handleEngineStopZeroReturn() {
  // Hold the last commanded position when tach pulses stop.
  // Do not reset display state or command zero here.
  // Clear only the engine-run confirmation so the next cranking phase
  // must be confirmed again before raw values are accepted.

  // タコ信号が途絶えた場合は、最後に指令した位置を保持します。
  // ここでは表示状態をリセットせず、0位置指令も出しません。
  // ただし始動確認だけを解除し、次回クランキング時は再度確認してから
  // raw値を表示へ入れます。
  engineRunConfirmed = false;
  engineRunConfirmCounter = 0;
}

bool isKeyOn() {
  return digitalRead(PIN_KEY_ON) != LOW;
}

void resetMotorDriveParameters() {
  // Restore SwitecX25 drive parameters explicitly because EDLC hold-up can resume
  // without a true MCU reset.
  // EDLC保持中はMCUが完全リセットされずに復帰することがあるため、駆動パラメータを明示的に戻します。
  tachMotor.accelTable = tachAccelTable;
  tachMotor.maxVel = tachAccelTable[tachAccelTableSize - 1][0];
}

void handleKeyOnReentry() {
  keyOffConfirmActive = false;
  keyOnConfirmActive = false;
  keyOffZeroCommanded = false;

  // Key OFF -> ON can happen while EDLC keeps the Pro Mini alive. In that case
  // setup() is not called again, so opening-related state must be reset here.
  // キーOFF→ONがEDLC保持中に起きるとsetup()が再実行されないため、
  // オープニング関連状態をここで必ず初期化します。
  resetMotorDriveParameters();
  resetDisplayState();

  forceZeroToStop();
  resetDisplayState();

  if (ENABLE_OPENING_DEMO) {
    openingDemo();
  }

  if (FORCE_ZERO_AFTER_OPENING) {
    forceZeroToStop();
  }

  resetDisplayState();
}

void returnNeedleToZero() {
  // Called only once when key-off zero return starts.
  // Do not call this repeatedly while key is OFF, because resetDisplayState()
  // also resets lastMotorUpdateMicros and can prevent the managed 1ms motor
  // update from advancing the zero-return motion.
  // キーOFF時0戻し開始時に1回だけ呼びます。キーOFF中に繰り返し呼ぶと
  // resetDisplayState() が lastMotorUpdateMicros も更新し続け、1ms周期の
  // motor.update() が進みにくくなります。
  resetDisplayState();
  tachMotor.setPosition(0);
  lastCommandedStep = 0;
  digitalWrite(PIN_LED, HIGH);
}

void forceZeroToStop() {
  tachMotor.zero();
  tachMotor.updateBlocking();
  tachMotor.setPosition(0);
  lastCommandedStep = 0;
}

void openingDemo() {
  tachMotor.setPosition(MOTOR_STEPS);
  tachMotor.updateBlocking();
  delay(200);

  tachMotor.setPosition(0);
  tachMotor.updateBlocking();
  delay(200);
}

bool isKeyOffConfirmedAtStartup() {
  for (int i = 0; i < STARTUP_KEY_CHECK_COUNT; i++) {
    if (digitalRead(PIN_KEY_ON) == HIGH) {
      return false;
    }
    delay(STARTUP_KEY_CHECK_INTERVAL_MS);
  }
  return true;
}

void resetDisplayState() {
  rawTachValue = 0;
  filteredDisplayValue = 0.0f;
  rawTargetStepFromDisplay = 0;
  stabilizedTargetStep = 0.0f;
  targetStepFromDisplay = 0;
  targetSlewInitialized = false;
  virtualNeedleStep = 0.0f;
  virtualNeedleVelocity = 0.0f;
  unsigned long nowMicros = micros();
  lastControlUpdateMicros = nowMicros;
  lastMotorUpdateMicros = nowMicros;
  engineRunConfirmed = false;
  engineRunConfirmCounter = 0;

  noInterrupts();
  lastPulseMicros = 0;
  pulseIntervalMicros = 0;
  pulseIntervalReady = false;
  interrupts();
}

void onTachPulse() {
  unsigned long nowMicros = micros();

  if (lastPulseMicros > 0) {
    pulseIntervalMicros = nowMicros - lastPulseMicros;
    pulseIntervalReady = true;
  }

  lastPulseMicros = nowMicros;
}

void updateMotorAtManagedInterval(unsigned long nowMicros) {
  // Keep tachMotor.update() from being called back-to-back at uncontrolled loop speed.
  // loop速度任せで連続呼び出しせず、明示した周期でだけ更新します。
  if ((nowMicros - lastMotorUpdateMicros) >= MOTOR_UPDATE_INTERVAL_US) {
    tachMotor.update();
    lastMotorUpdateMicros = nowMicros;
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
