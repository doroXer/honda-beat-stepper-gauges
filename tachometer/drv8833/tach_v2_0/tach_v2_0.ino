#include <Arduino.h>
#include <math.h>

// ============================================================
// Forward declarations used across layers
// ============================================================

float constrainFloat(float value, float minValue, float maxValue);
float minFloat(float a, float b);
float absFloat(float value);

bool readKeyOnRaw();
bool updateAndReadKeyOn();
bool isKeyOnConfirmedAtStartup();
void startGaugeFromDriverOff();
void beginKeyOffReturn();
void resumeRunningDuringKeyOffReturn();

void runV1UpperControl();
bool updateEngineRunConfirmation(unsigned long intervalMicros);
bool isValidTachInterval(unsigned long intervalMicros);
void initializeDisplayFilterFromRaw();
void updateRawValueFromPulseInterval(unsigned long intervalMicros);
void updateDisplayFilter();
void updateTargetStepFromDisplay();
void updateVirtualNeedleControl(unsigned long nowMicros);
void commandMotorPosition(float logicalStep);
void handleEngineStopZeroReturn();

void forceZeroToStop();
void openingDemo();
void resetDisplayState();
void resetDisplayStateAtPosition(float logicalPosition);
void onTachPulse();

/*
  Honda Beat Tachometer v2.0 / ホンダビート タコメーター v2.0
  ---------------------------------------------------------------

  Initial 1/4-microstep release based on the v1.1 upper-control architecture.
  v1.1の上位制御アーキテクチャをベースにした1/4マイクロステップ初期版です。

  v1.1 upper-control behavior retained / v1.1上位制御を維持:
    pulse interval acquisition
      -> engine-run confirmation
      -> calibrated integer raw tach value
      -> 7:1 display filter
      -> calibrated logical-step conversion
      -> target-step slew-rate limiting
      -> virtual needle velocity / acceleration control
      -> virtual stop band
      -> hold last position when tach pulses stop

  v2.0 changes from v1.1 / v1.1からのv2.0変更:
    - SwitecX25 direct drive is replaced by DRV8833.
    - X27.168 is driven by a dedicated 1/4-microstep layer.
    - Position values remain float logical steps through the upper layer.
    - Only the motor-layer boundary converts logical steps to integer
      quarter-microsteps using the 8/3 physical-angle ratio.
    - Key ON/OFF behavior uses DRV8833 STBY to avoid unnecessary excitation.
    - On key-OFF, motor velocity and residual fractional steps are cleared
      before commanding zero, while the actual current coordinate is retained.

  Board:
    Arduino Pro Mini 5 V / 16 MHz

  Pin assignment:
    D2  : tach pulse input
    D3  : key ON input
    D5  : DRV8833 AIN2 = A-
    D6  : DRV8833 AIN1 = A+
    D7  : DRV8833 STBY / nSLEEP
    D9  : DRV8833 BIN1 = B+
    D10 : DRV8833 BIN2 = B-
    D13 : key-OFF / driver-OFF indicator

  Important:
    - v2.0 is intentionally dedicated to 1/4 microstepping.
    - No untested 1/2, 1/8, or 1/16 mode selection is provided.
    - SwitecX25 advances through six electrical states per cycle, while
      this quarter-step layer uses sixteen phase positions per cycle.
      Therefore three v1 logical steps correspond to eight quarter-steps.
    - The v1 upper-control scale remains 0..720 logical steps.
*/

// ============================================================
// v1 logical scale and pin assignment
// ============================================================

const int MOTOR_STEPS = 240 * 3;  // v1 logical-step full scale: 720

const uint8_t PIN_TACH_INPUT = 2;
const uint8_t PIN_KEY_ON = 3;
const uint8_t PIN_LED = 13;

const uint8_t PIN_A_NEG = 5;   // D5  -> AIN2 -> A-
const uint8_t PIN_A_POS = 6;   // D6  -> AIN1 -> A+
const uint8_t PIN_STBY  = 7;   // D7  -> STBY / nSLEEP
const uint8_t PIN_B_POS = 9;   // D9  -> BIN1 -> B+
const uint8_t PIN_B_NEG = 10;  // D10 -> BIN2 -> B-

// Existing vehicle-side key circuit:
//   HIGH = key ON
//   LOW  = key OFF
const uint8_t KEY_ON_ACTIVE_LEVEL = HIGH;

// Verified physical direction for the current wiring.
// Logical + direction must move the pointer up the scale.
const int8_t SCALE_DIRECTION = -1;

// ============================================================
// v1 raw tach conversion and filtering
// ============================================================

const int STEP_OFFSET = 23;
const long STEP_CONVERSION_NUMERATOR = 2952000L;

const byte DISPLAY_FILTER_OLD = 7;
const byte DISPLAY_FILTER_NEW = 1;

// ============================================================
// v1 virtual needle control
// ============================================================

const unsigned long CONTROL_UPDATE_INTERVAL_US = 100000UL;

const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC = 150.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC = 150.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2 = 500.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 = 500.0f;
const float VIRTUAL_STOP_BAND_STEP = 5.0f;

const float TARGET_STEP_MAX_UP_PER_SEC = 100.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC = 100.0f;

// ============================================================
// v1 startup and tach-input behavior
// ============================================================

const bool ENABLE_OPENING_DEMO = true;
const bool FORCE_ZERO_AFTER_OPENING = true;

const int STARTUP_KEY_CHECK_COUNT = 5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS = 20;
const unsigned int STARTUP_KEY_SETTLE_DELAY_MS = 300;

const unsigned long NO_PULSE_STOP_TIMEOUT_US = 1000000UL;

const byte ENGINE_RUN_CONFIRM_COUNT = 20;
const unsigned long MIN_VALID_INTERVAL_US = 3000UL;
const unsigned long MAX_VALID_INTERVAL_US = 300000UL;

// ============================================================
// v1 logical-step -> v2 quarter-step boundary conversion
// ============================================================

/*
  SwitecX25 uses six drive states per electrical cycle.
  The dedicated v2 quarter-step phase table uses sixteen positions.

    3 v1 logical steps = 8 v2 quarter-steps
    1 v1 logical step  = 8 / 3 v2 quarter-steps

  Keep this ratio explicit. The v1 upper-control scale stays unchanged,
  and the only conversion into the new motor coordinate happens here.
*/
const int32_t MOTOR_POSITION_RATIO_NUMERATOR = 8;
const int32_t MOTOR_POSITION_RATIO_DENOMINATOR = 3;

const int32_t MOTOR_MAX_QUARTER_STEPS =
    ((int32_t)MOTOR_STEPS * MOTOR_POSITION_RATIO_NUMERATOR) /
    MOTOR_POSITION_RATIO_DENOMINATOR;  // 720 -> 1920

int32_t logicalStepToMotorPosition(float logicalStep)
{
  logicalStep = constrainFloat(logicalStep, 0.0f, (float)MOTOR_STEPS);

  return lroundf(
      logicalStep *
      (float)MOTOR_POSITION_RATIO_NUMERATOR /
      (float)MOTOR_POSITION_RATIO_DENOMINATOR);
}

float motorPositionToLogicalStep(int32_t motorPosition)
{
  motorPosition = constrain(
      motorPosition,
      (int32_t)0,
      MOTOR_MAX_QUARTER_STEPS);

  return
      (float)motorPosition *
      (float)MOTOR_POSITION_RATIO_DENOMINATOR /
      (float)MOTOR_POSITION_RATIO_NUMERATOR;
}

// ============================================================
// DRV8833 / X27.168 quarter-microstep settings
// ============================================================

const uint8_t RUN_PWM_MAX = 255;
const uint8_t HOLD_PWM_MAX = 150;
const uint8_t ZERO_PWM_MAX = 200;

// Normal-running motor layer.
// These are intentionally above the v1 upper-control limits so that
// the motor layer follows the virtual needle rather than becoming the
// dominant filter.
const float MOTOR_MAX_SPEED_QSTEP_PER_SEC = 900.0f;
const float MOTOR_ACCEL_QSTEP_PER_SEC2 = 2400.0f;

// Opening/updateBlocking limits.
// The phase ratio correction changes a former x4 scale assumption to 8/3.
// The blocking motion is therefore allowed to run faster so the visible
// opening remains close to the v1 opening speed instead of becoming slower.
const float BLOCKING_MAX_SPEED_QSTEP_PER_SEC = 2800.0f;
const float BLOCKING_ACCEL_QSTEP_PER_SEC2 = 10000.0f;

const float MOTOR_MAX_UPDATE_DT_SEC = 0.020f;
const uint8_t MOTOR_MAX_EMITTED_STEPS_PER_UPDATE = 8;

// zero() ignores the stored coordinate and drives farther than full scale.
// 1920 full-scale + 192 margin = approximately 110% of full scale.
const int32_t ZERO_TRAVEL_MARGIN_QSTEPS = 192;
const int32_t ZERO_TRAVEL_QSTEPS =
    MOTOR_MAX_QUARTER_STEPS + ZERO_TRAVEL_MARGIN_QSTEPS;

// At 900 us per quarter-step, a 2112-quarter-step zero travel takes
// approximately 1.90 s. This is intentionally much shorter than the first
// prototype while still guaranteeing mechanical-stop contact.
const uint16_t ZERO_STEP_INTERVAL_US = 900;

// Keep zero travel aligned to one complete 16-phase cycle so zero() returns
// to the same electrical phase before currentPosition_ is redefined as zero.
const int32_t ZERO_TRAVEL_QSTEPS_ALIGNED =
    ((ZERO_TRAVEL_QSTEPS + 15L) / 16L) * 16L;

// Key-OFF sequence:
// return to coordinate zero, hold briefly, then disable DRV8833.
const unsigned long KEYOFF_ZERO_HOLD_MS = 200UL;

// ============================================================
// Quarter-step phase table
// ============================================================

struct PhaseValue {
  int16_t phaseA;
  int16_t phaseB;
};

// 16 phase positions per electrical cycle.
// The table uses sine / cosine values with amplitude 255.
const PhaseValue PHASE_TABLE[16] = {
  {   0,  255},
  {  98,  236},
  { 180,  180},
  { 236,   98},
  { 255,    0},
  { 236,  -98},
  { 180, -180},
  {  98, -236},
  {   0, -255},
  { -98, -236},
  {-180, -180},
  {-236,  -98},
  {-255,    0},
  {-236,   98},
  {-180,  180},
  { -98,  236}
};

// ============================================================
// Dedicated DRV8833 1/4-microstep motor layer
// ============================================================

class X27QuarterStep {
public:
  void begin()
  {
    pinMode(PIN_A_NEG, OUTPUT);
    pinMode(PIN_A_POS, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);
    pinMode(PIN_B_POS, OUTPUT);
    pinMode(PIN_B_NEG, OUTPUT);

    digitalWrite(PIN_STBY, LOW);

    analogWrite(PIN_A_NEG, 0);
    analogWrite(PIN_A_POS, 0);
    analogWrite(PIN_B_POS, 0);
    analogWrite(PIN_B_NEG, 0);

    enabled_ = false;
    currentPosition_ = 0;
    targetPosition_ = 0;
    currentPhase_ = 0;
    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();
    wasMoving_ = false;
  }

  void setPosition(int32_t position)
  {
    targetPosition_ = constrain(
        position,
        (int32_t)0,
        MOTOR_MAX_QUARTER_STEPS);
  }

  int32_t getCurrentPosition() const
  {
    return currentPosition_;
  }

  bool isAtTarget() const
  {
    return
        currentPosition_ == targetPosition_ &&
        fabsf(motorVelocity_) < 0.5f &&
        fabsf(stepAccumulator_) < 1.0f;
  }

  void update()
  {
    updateMotion(
        MOTOR_MAX_SPEED_QSTEP_PER_SEC,
        MOTOR_ACCEL_QSTEP_PER_SEC2);
  }

  void updateBlocking()
  {
    unsigned long safetyStartMs = millis();

    while (!isAtTarget()) {
      updateMotion(
          BLOCKING_MAX_SPEED_QSTEP_PER_SEC,
          BLOCKING_ACCEL_QSTEP_PER_SEC2);

      delayMicroseconds(50);

      if ((unsigned long)(millis() - safetyStartMs) > 10000UL) {
        break;
      }
    }

    applyPhase(currentPhase_, HOLD_PWM_MAX);
  }

  void zero()
  {
    enable();

    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();

    for (int32_t i = 0;
         i < ZERO_TRAVEL_QSTEPS_ALIGNED;
         i++) {

      emitLogicalQuarterStep(-1, ZERO_PWM_MAX);
      delayMicroseconds(ZERO_STEP_INTERVAL_US);
    }

    currentPosition_ = 0;
    targetPosition_ = 0;
    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();
    wasMoving_ = false;

    applyPhase(currentPhase_, HOLD_PWM_MAX);
  }

  void enable()
  {
    if (enabled_) {
      return;
    }

    digitalWrite(PIN_STBY, HIGH);
    enabled_ = true;
    lastMotionUpdateUs_ = micros();

    applyPhase(currentPhase_, HOLD_PWM_MAX);
  }

  void disable()
  {
    analogWrite(PIN_A_NEG, 0);
    analogWrite(PIN_A_POS, 0);
    analogWrite(PIN_B_POS, 0);
    analogWrite(PIN_B_NEG, 0);

    digitalWrite(PIN_STBY, LOW);
    enabled_ = false;
    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();
    wasMoving_ = false;
  }

  bool isEnabled() const
  {
    return enabled_;
  }

  bool isMoving() const
  {
    return !isAtTarget();
  }

  // Clears only the current motion state while preserving the actual
  // stored motor coordinate. This is used before a forced retarget such
  // as key-OFF return, so velocity and fractional steps from the previous
  // direction cannot be mistaken for target arrival.
  void stopMotion()
  {
    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();
    wasMoving_ = false;

    if (enabled_) {
      applyPhase(currentPhase_, HOLD_PWM_MAX);
    }
  }

  // Immediately cancels a key-OFF return and holds the actual stored
  // motor coordinate. Used only for key re-entry while STBY is active.
  void holdCurrentPosition()
  {
    targetPosition_ = currentPosition_;
    stopMotion();
  }

private:
  int32_t currentPosition_ = 0;
  int32_t targetPosition_ = 0;
  int16_t currentPhase_ = 0;

  float motorVelocity_ = 0.0f;
  float stepAccumulator_ = 0.0f;
  unsigned long lastMotionUpdateUs_ = 0UL;

  bool enabled_ = false;
  bool wasMoving_ = false;

  void updateMotion(float maximumSpeed, float acceleration)
  {
    if (!enabled_) {
      return;
    }

    const unsigned long nowUs = micros();
    unsigned long elapsedUs = nowUs - lastMotionUpdateUs_;

    if (elapsedUs == 0UL) {
      return;
    }

    lastMotionUpdateUs_ = nowUs;

    float dtSec = (float)elapsedUs / 1000000.0f;

    if (dtSec > MOTOR_MAX_UPDATE_DT_SEC) {
      dtSec = MOTOR_MAX_UPDATE_DT_SEC;
    }

    const int32_t error = targetPosition_ - currentPosition_;

    if (error == 0) {
      motorVelocity_ = 0.0f;
      stepAccumulator_ = 0.0f;

      if (wasMoving_) {
        applyPhase(currentPhase_, HOLD_PWM_MAX);
      }

      wasMoving_ = false;
      return;
    }

    const int8_t desiredDirection = error > 0 ? 1 : -1;

    const int8_t velocityDirection =
        motorVelocity_ > 0.0f ? 1 :
        motorVelocity_ < 0.0f ? -1 : 0;

    const float speedAbs = fabsf(motorVelocity_);

    const float stoppingDistance =
        (speedAbs * speedAbs) /
        (2.0f * acceleration);

    float accelerationCommand = 0.0f;

    if (velocityDirection != 0 &&
        velocityDirection != desiredDirection) {

      accelerationCommand =
          -(float)velocityDirection * acceleration;
    }
    else if ((float)labs(error) <=
             stoppingDistance + 1.0f) {

      accelerationCommand =
          -(float)velocityDirection * acceleration;
    }
    else {
      accelerationCommand =
          (float)desiredDirection * acceleration;
    }

    motorVelocity_ += accelerationCommand * dtSec;

    motorVelocity_ = constrainFloat(
        motorVelocity_,
        -maximumSpeed,
         maximumSpeed);

    if (desiredDirection > 0 &&
        motorVelocity_ < 0.0f) {

      motorVelocity_ = 0.0f;
    }

    if (desiredDirection < 0 &&
        motorVelocity_ > 0.0f) {

      motorVelocity_ = 0.0f;
    }

    stepAccumulator_ += motorVelocity_ * dtSec;

    uint8_t emittedSteps = 0;

    while (fabsf(stepAccumulator_) >= 1.0f &&
           emittedSteps < MOTOR_MAX_EMITTED_STEPS_PER_UPDATE) {

      const int8_t stepDirection =
          stepAccumulator_ > 0.0f ? 1 : -1;

      if ((stepDirection > 0 &&
           currentPosition_ >= targetPosition_) ||
          (stepDirection < 0 &&
           currentPosition_ <= targetPosition_)) {

        currentPosition_ = targetPosition_;
        motorVelocity_ = 0.0f;
        stepAccumulator_ = 0.0f;
        break;
      }

      emitLogicalQuarterStep(stepDirection, RUN_PWM_MAX);
      currentPosition_ += stepDirection;
      stepAccumulator_ -= (float)stepDirection;

      wasMoving_ = true;
      emittedSteps++;
    }

    if (currentPosition_ == targetPosition_ &&
        fabsf(stepAccumulator_) < 1.0f) {

      motorVelocity_ = 0.0f;
      stepAccumulator_ = 0.0f;
      applyPhase(currentPhase_, HOLD_PWM_MAX);
      wasMoving_ = false;
    }
  }

  void emitLogicalQuarterStep(int8_t logicalDirection, uint8_t pwmMax)
  {
    const int8_t physicalDirection =
        logicalDirection * SCALE_DIRECTION;

    currentPhase_ += physicalDirection;

    if (currentPhase_ >= 16) {
      currentPhase_ = 0;
    }
    else if (currentPhase_ < 0) {
      currentPhase_ = 15;
    }

    applyPhase(currentPhase_, pwmMax);
  }

  void applyPhase(int16_t phaseIndex, uint8_t pwmMax)
  {
    phaseIndex %= 16;

    if (phaseIndex < 0) {
      phaseIndex += 16;
    }

    const PhaseValue phase = PHASE_TABLE[phaseIndex];

    applySignedPhase(
        PIN_A_POS,
        PIN_A_NEG,
        phase.phaseA,
        pwmMax);

    applySignedPhase(
        PIN_B_POS,
        PIN_B_NEG,
        phase.phaseB,
        pwmMax);
  }

  void applySignedPhase(
      uint8_t pinPositive,
      uint8_t pinNegative,
      int16_t signedAmplitude,
      uint8_t pwmMax)
  {
    int16_t magnitude = abs(signedAmplitude);

    if (magnitude > 255) {
      magnitude = 255;
    }

    uint16_t scaled =
        ((uint16_t)magnitude * (uint16_t)pwmMax) / 255U;

    if (signedAmplitude > 0) {
      analogWrite(pinPositive, (uint8_t)scaled);
      analogWrite(pinNegative, 0);
    }
    else if (signedAmplitude < 0) {
      analogWrite(pinPositive, 0);
      analogWrite(pinNegative, (uint8_t)scaled);
    }
    else {
      analogWrite(pinPositive, 0);
      analogWrite(pinNegative, 0);
    }
  }
};

X27QuarterStep tachMotor;

// ============================================================
// Gauge-state management
// ============================================================

enum class GaugeState : uint8_t {
  DriverOff,
  Starting,
  Running,
  ReturningToZero,
  ZeroHold
};

GaugeState gaugeState = GaugeState::DriverOff;

unsigned long zeroHoldStartMs = 0UL;

bool lastConfirmedKeyOn = false;
bool keyOnDebounced = false;
bool lastRawKeyOn = false;
unsigned long keyStateChangedMs = 0UL;

const unsigned long KEY_DEBOUNCE_MS = 50UL;

// ============================================================
// Tach pulse acquisition
// ============================================================

volatile unsigned long latestPulseIntervalUs = 0UL;
volatile unsigned long lastPulseUs = 0UL;
volatile bool newPulseAvailable = false;

unsigned long lastAcceptedPulseUs = 0UL;

// ============================================================
// v1 upper-control state
// ============================================================

long rawTachValue = 0L;
long filteredDisplayValue = 0L;

// Position path changed to float so quarter-step targets survive until
// the dedicated motor-layer boundary.
float rawTargetStepFromDisplay = 0.0f;
float stabilizedTargetStep = 0.0f;
float targetStepFromDisplay = 0.0f;

float virtualNeedleStep = 0.0f;
float virtualNeedleVelocity = 0.0f;

unsigned long lastControlUpdateUs = 0UL;

byte engineRunConfirmCount = 0;
bool engineRunningConfirmed = false;
bool displayFilterInitialized = false;

// ============================================================
// Arduino setup / loop
// ============================================================

void setup()
{
  pinMode(PIN_KEY_ON, INPUT_PULLUP);
  pinMode(PIN_TACH_INPUT, INPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, HIGH);

  tachMotor.begin();

  lastRawKeyOn = readKeyOnRaw();
  keyOnDebounced = lastRawKeyOn;
  lastConfirmedKeyOn = keyOnDebounced;
  keyStateChangedMs = millis();

  attachInterrupt(
      digitalPinToInterrupt(PIN_TACH_INPUT),
      onTachPulse,
      RISING);

  if (isKeyOnConfirmedAtStartup()) {
    startGaugeFromDriverOff();
  }
  else {
    gaugeState = GaugeState::DriverOff;
    tachMotor.disable();
    digitalWrite(PIN_LED, HIGH);
  }
}

void loop()
{
  const bool keyOn = updateAndReadKeyOn();

  switch (gaugeState) {
    case GaugeState::DriverOff:
      if (keyOn) {
        startGaugeFromDriverOff();
      }
      break;

    case GaugeState::Starting:
      break;

    case GaugeState::Running:
      if (!keyOn) {
        beginKeyOffReturn();
      }
      else {
        tachMotor.update();
        runV1UpperControl();
      }
      break;

    case GaugeState::ReturningToZero:
      if (keyOn) {
        resumeRunningDuringKeyOffReturn();
      }
      else {
        tachMotor.update();

        if (tachMotor.isAtTarget()) {
          zeroHoldStartMs = millis();
          gaugeState = GaugeState::ZeroHold;
        }
      }
      break;

    case GaugeState::ZeroHold:
      if (keyOn) {
        resumeRunningDuringKeyOffReturn();
      }
      else if ((unsigned long)(millis() - zeroHoldStartMs) >=
               KEYOFF_ZERO_HOLD_MS) {

        tachMotor.disable();
        digitalWrite(PIN_LED, HIGH);
        gaugeState = GaugeState::DriverOff;
      }
      break;
  }

  lastConfirmedKeyOn = keyOn;
}

// ============================================================
// Key handling
// ============================================================

bool readKeyOnRaw()
{
  return digitalRead(PIN_KEY_ON) == KEY_ON_ACTIVE_LEVEL;
}

bool updateAndReadKeyOn()
{
  const bool rawKeyOn = readKeyOnRaw();
  const unsigned long nowMs = millis();

  if (rawKeyOn != lastRawKeyOn) {
    lastRawKeyOn = rawKeyOn;
    keyStateChangedMs = nowMs;
  }

  if ((unsigned long)(nowMs - keyStateChangedMs) >=
      KEY_DEBOUNCE_MS) {

    keyOnDebounced = rawKeyOn;
  }

  return keyOnDebounced;
}

bool isKeyOnConfirmedAtStartup()
{
  for (int i = 0; i < STARTUP_KEY_CHECK_COUNT; i++) {
    if (!readKeyOnRaw()) {
      return false;
    }

    delay(STARTUP_KEY_CHECK_INTERVAL_MS);
  }

  return true;
}

// ============================================================
// Gauge startup / key-off transitions
// ============================================================

void startGaugeFromDriverOff()
{
  gaugeState = GaugeState::Starting;

  digitalWrite(PIN_LED, LOW);

  tachMotor.enable();
  delay(STARTUP_KEY_SETTLE_DELAY_MS);

  forceZeroToStop();

  if (ENABLE_OPENING_DEMO) {
    openingDemo();
  }

  if (FORCE_ZERO_AFTER_OPENING) {
    forceZeroToStop();
  }

  resetDisplayState();

  gaugeState = GaugeState::Running;
}

void beginKeyOffReturn()
{
  const float currentLogicalPosition =
      motorPositionToLogicalStep(
          tachMotor.getCurrentPosition());

  resetDisplayStateAtPosition(currentLogicalPosition);

  // Key-OFF may occur while the pointer is still moving upward.
  // Preserve the current coordinate, but discard the previous velocity
  // and fractional-step remainder before commanding the return to zero.
  tachMotor.stopMotion();
  commandMotorPosition(0.0f);

  digitalWrite(PIN_LED, HIGH);
  gaugeState = GaugeState::ReturningToZero;
}

void resumeRunningDuringKeyOffReturn()
{
  // Important for re-entry:
  // cancel the zero-return target immediately and hold the actual position
  // before copying it back into the v1 upper-control state.
  tachMotor.holdCurrentPosition();

  const float currentLogicalPosition =
      motorPositionToLogicalStep(
          tachMotor.getCurrentPosition());

  resetDisplayStateAtPosition(currentLogicalPosition);

  digitalWrite(PIN_LED, LOW);
  gaugeState = GaugeState::Running;
}

// ============================================================
// v1 upper control
// ============================================================

void runV1UpperControl()
{
  unsigned long latestInterval = 0UL;
  unsigned long latestPulseTimestamp = 0UL;
  bool hasNewPulse = false;

  noInterrupts();

  if (newPulseAvailable) {
    latestInterval = latestPulseIntervalUs;
    latestPulseTimestamp = lastPulseUs;
    newPulseAvailable = false;
    hasNewPulse = true;
  }

  interrupts();

  if (hasNewPulse) {
    if (isValidTachInterval(latestInterval)) {
      lastAcceptedPulseUs = latestPulseTimestamp;

      updateRawValueFromPulseInterval(latestInterval);

      const bool runningNow =
          updateEngineRunConfirmation(latestInterval);

      if (runningNow) {
        if (!displayFilterInitialized) {
          initializeDisplayFilterFromRaw();
        }
        else {
          updateDisplayFilter();
        }

        updateTargetStepFromDisplay();
      }
    }
  }

  handleEngineStopZeroReturn();
  updateVirtualNeedleControl(micros());
}

bool updateEngineRunConfirmation(unsigned long intervalMicros)
{
  if (!isValidTachInterval(intervalMicros)) {
    engineRunConfirmCount = 0;
    engineRunningConfirmed = false;
    return false;
  }

  if (engineRunConfirmCount < ENGINE_RUN_CONFIRM_COUNT) {
    engineRunConfirmCount++;
  }

  if (engineRunConfirmCount >= ENGINE_RUN_CONFIRM_COUNT) {
    engineRunningConfirmed = true;
  }

  return engineRunningConfirmed;
}

bool isValidTachInterval(unsigned long intervalMicros)
{
  if (intervalMicros < MIN_VALID_INTERVAL_US) {
    return false;
  }

  if (intervalMicros > MAX_VALID_INTERVAL_US) {
    return false;
  }

  return true;
}

void initializeDisplayFilterFromRaw()
{
  filteredDisplayValue = rawTachValue;
  displayFilterInitialized = true;
}

void updateRawValueFromPulseInterval(unsigned long intervalMicros)
{
  if (intervalMicros == 0UL) {
    return;
  }

  rawTachValue =
      STEP_CONVERSION_NUMERATOR /
      (long)intervalMicros;

  if (rawTachValue < 0L) {
    rawTachValue = 0L;
  }

  if (rawTachValue > MOTOR_STEPS + STEP_OFFSET) {
    rawTachValue = MOTOR_STEPS + STEP_OFFSET;
  }
}

void updateDisplayFilter()
{
  filteredDisplayValue =
      ((long)DISPLAY_FILTER_OLD * filteredDisplayValue +
       (long)DISPLAY_FILTER_NEW * rawTachValue) /
      ((long)DISPLAY_FILTER_OLD +
       (long)DISPLAY_FILTER_NEW);
}

void updateTargetStepFromDisplay()
{
  long logicalStep =
      filteredDisplayValue - STEP_OFFSET;

  if (logicalStep < 0L) {
    logicalStep = 0L;
  }

  if (logicalStep > MOTOR_STEPS) {
    logicalStep = MOTOR_STEPS;
  }

  rawTargetStepFromDisplay = (float)logicalStep;
}

void updateVirtualNeedleControl(unsigned long nowMicros)
{
  if (lastControlUpdateUs == 0UL) {
    lastControlUpdateUs = nowMicros;
    return;
  }

  unsigned long elapsedUs =
      nowMicros - lastControlUpdateUs;

  if (elapsedUs < CONTROL_UPDATE_INTERVAL_US) {
    return;
  }

  lastControlUpdateUs = nowMicros;

  float dtSec =
      (float)elapsedUs / 1000000.0f;

  if (dtSec > 0.5f) {
    dtSec = 0.5f;
  }

  // v1 target-step slew-rate limiting
  const float targetDelta =
      rawTargetStepFromDisplay - stabilizedTargetStep;

  const float maxTargetDelta =
      (targetDelta >= 0.0f ?
       TARGET_STEP_MAX_UP_PER_SEC :
       TARGET_STEP_MAX_DOWN_PER_SEC) * dtSec;

  if (absFloat(targetDelta) <= maxTargetDelta) {
    stabilizedTargetStep = rawTargetStepFromDisplay;
  }
  else {
    stabilizedTargetStep +=
        targetDelta > 0.0f ?
        maxTargetDelta :
        -maxTargetDelta;
  }

  stabilizedTargetStep =
      constrainFloat(
          stabilizedTargetStep,
          0.0f,
          (float)MOTOR_STEPS);

  targetStepFromDisplay = stabilizedTargetStep;

  const float error =
      targetStepFromDisplay - virtualNeedleStep;

  if (absFloat(error) <= VIRTUAL_STOP_BAND_STEP) {
    virtualNeedleStep = targetStepFromDisplay;
    virtualNeedleVelocity = 0.0f;

    commandMotorPosition(virtualNeedleStep);
    return;
  }

  const float desiredDirection =
      error > 0.0f ? 1.0f : -1.0f;

  const float maxVelocity =
      desiredDirection > 0.0f ?
      VIRTUAL_MAX_VEL_UP_STEP_PER_SEC :
      VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC;

  const float acceleration =
      desiredDirection > 0.0f ?
      VIRTUAL_ACCEL_UP_STEP_PER_SEC2 :
      VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2;

  const float velocityAbs =
      absFloat(virtualNeedleVelocity);

  const float stoppingDistance =
      (velocityAbs * velocityAbs) /
      (2.0f * acceleration);

  if ((virtualNeedleVelocity > 0.0f &&
       desiredDirection < 0.0f) ||
      (virtualNeedleVelocity < 0.0f &&
       desiredDirection > 0.0f)) {

    if (virtualNeedleVelocity > 0.0f) {
      virtualNeedleVelocity -= acceleration * dtSec;

      if (virtualNeedleVelocity < 0.0f) {
        virtualNeedleVelocity = 0.0f;
      }
    }
    else {
      virtualNeedleVelocity += acceleration * dtSec;

      if (virtualNeedleVelocity > 0.0f) {
        virtualNeedleVelocity = 0.0f;
      }
    }
  }
  else if (absFloat(error) <=
           stoppingDistance + VIRTUAL_STOP_BAND_STEP) {

    if (virtualNeedleVelocity > 0.0f) {
      virtualNeedleVelocity -= acceleration * dtSec;

      if (virtualNeedleVelocity < 0.0f) {
        virtualNeedleVelocity = 0.0f;
      }
    }
    else if (virtualNeedleVelocity < 0.0f) {
      virtualNeedleVelocity += acceleration * dtSec;

      if (virtualNeedleVelocity > 0.0f) {
        virtualNeedleVelocity = 0.0f;
      }
    }
  }
  else {
    virtualNeedleVelocity +=
        desiredDirection * acceleration * dtSec;

    if (virtualNeedleVelocity > maxVelocity) {
      virtualNeedleVelocity = maxVelocity;
    }

    if (virtualNeedleVelocity < -maxVelocity) {
      virtualNeedleVelocity = -maxVelocity;
    }
  }

  virtualNeedleStep +=
      virtualNeedleVelocity * dtSec;

  if (virtualNeedleStep < 0.0f) {
    virtualNeedleStep = 0.0f;
    virtualNeedleVelocity = 0.0f;
  }

  if (virtualNeedleStep > (float)MOTOR_STEPS) {
    virtualNeedleStep = (float)MOTOR_STEPS;
    virtualNeedleVelocity = 0.0f;
  }

  commandMotorPosition(virtualNeedleStep);
}

void commandMotorPosition(float logicalStep)
{
  tachMotor.setPosition(
      logicalStepToMotorPosition(logicalStep));
}

void handleEngineStopZeroReturn()
{
  if (!engineRunningConfirmed) {
    return;
  }

  if (lastAcceptedPulseUs == 0UL) {
    return;
  }

  const unsigned long nowMicros = micros();

  if ((unsigned long)(nowMicros - lastAcceptedPulseUs) <
      NO_PULSE_STOP_TIMEOUT_US) {

    return;
  }

  engineRunConfirmCount = 0;
  engineRunningConfirmed = false;
  displayFilterInitialized = false;

  rawTachValue = 0L;
  filteredDisplayValue = 0L;
  rawTargetStepFromDisplay = 0.0f;
  stabilizedTargetStep = virtualNeedleStep;
  targetStepFromDisplay = virtualNeedleStep;

  // Keep current gauge position when engine pulses stop.
  // Key OFF is handled separately by GaugeState.
}

// ============================================================
// Startup helpers
// ============================================================

void forceZeroToStop()
{
  tachMotor.zero();
}

void openingDemo()
{
  commandMotorPosition((float)MOTOR_STEPS);
  tachMotor.updateBlocking();

  delay(200);

  commandMotorPosition(0.0f);
  tachMotor.updateBlocking();

  delay(200);
}

void resetDisplayState()
{
  resetDisplayStateAtPosition(0.0f);
}

void resetDisplayStateAtPosition(float logicalPosition)
{
  logicalPosition =
      constrainFloat(
          logicalPosition,
          0.0f,
          (float)MOTOR_STEPS);

  rawTachValue = 0L;
  filteredDisplayValue = 0L;

  rawTargetStepFromDisplay = logicalPosition;
  stabilizedTargetStep = logicalPosition;
  targetStepFromDisplay = logicalPosition;

  virtualNeedleStep = logicalPosition;
  virtualNeedleVelocity = 0.0f;

  lastControlUpdateUs = micros();

  engineRunConfirmCount = 0;
  engineRunningConfirmed = false;
  displayFilterInitialized = false;

  noInterrupts();
  latestPulseIntervalUs = 0UL;
  lastPulseUs = 0UL;
  newPulseAvailable = false;
  interrupts();

  lastAcceptedPulseUs = 0UL;

  commandMotorPosition(logicalPosition);
}

// ============================================================
// Tach pulse interrupt
// ============================================================

void onTachPulse()
{
  const unsigned long nowMicros = micros();

  if (lastPulseUs != 0UL) {
    latestPulseIntervalUs =
        nowMicros - lastPulseUs;

    newPulseAvailable = true;
  }

  lastPulseUs = nowMicros;
}

// ============================================================
// Small float helpers
// ============================================================

float constrainFloat(float value, float minValue, float maxValue)
{
  if (value < minValue) {
    return minValue;
  }

  if (value > maxValue) {
    return maxValue;
  }

  return value;
}

float minFloat(float a, float b)
{
  return a < b ? a : b;
}

float absFloat(float value)
{
  return value < 0.0f ? -value : value;
}
