        -maximumSpeed,
         maximumSpeed);

    if (velocityDirection > 0 &&
        motorVelocity_ < 0.0f &&
        desiredDirection >= 0) {
      motorVelocity_ = 0.0f;
    }

    if (velocityDirection < 0 &&
        motorVelocity_ > 0.0f &&
        desiredDirection <= 0) {
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
        labs(error) <= 1) {
      motorVelocity_ = 0.0f;
      stepAccumulator_ = 0.0f;
    }
  }

  void drivePhasePair(
      uint8_t positivePin,
      uint8_t negativePin,
      int16_t command,
      uint8_t pwmMaximum)
  {
    const uint8_t pwmValue =
        (uint8_t)(
            ((long)abs(command) *
             (long)pwmMaximum) /
            255L);

    if (command > 0) {
      analogWrite(positivePin, pwmValue);
      analogWrite(negativePin, 0);
    }
    else if (command < 0) {
      analogWrite(positivePin, 0);
      analogWrite(negativePin, pwmValue);
    }
    else {
      analogWrite(positivePin, 0);
      analogWrite(negativePin, 0);
    }
  }

  void applyPhase(int8_t phaseIndex, uint8_t pwmMaximum)
  {
    phaseIndex %= 16;

    if (phaseIndex < 0) {
      phaseIndex += 16;
    }

    const PhaseValue &phase = PHASE_TABLE[phaseIndex];

    drivePhasePair(
        PIN_A_POS,
        PIN_A_NEG,
        phase.phaseA,
        pwmMaximum);

    drivePhasePair(
        PIN_B_POS,
        PIN_B_NEG,
        phase.phaseB,
        pwmMaximum);
  }

  void emitLogicalQuarterStep(
      int8_t logicalDirection,
      uint8_t pwmMaximum)
  {
    currentPhase_ += logicalDirection * SCALE_DIRECTION;

    if (currentPhase_ >= 16) {
      currentPhase_ = 0;
    }
    else if (currentPhase_ < 0) {
      currentPhase_ = 15;
    }

    applyPhase(currentPhase_, pwmMaximum);
  }
};

X27QuarterStep speedMotor;

volatile unsigned long lastPulseMicros = 0;
volatile unsigned long lastPulseSeenMicros = 0;
volatile unsigned long latestPulseIntervalMicros = 0;
volatile unsigned long latestRawPulseIntervalMicros = 0;
volatile bool pulseIntervalUpdated = false;

volatile unsigned long speedPulseCountTotal = 0;
volatile unsigned long speedPulseCountValid = 0;
volatile unsigned long speedPulseCountRejectedShort = 0;

unsigned long rawSpeedValue = 0;
float filteredDisplayValue = 0.0f;
float rawTargetStepFromDisplay = 0.0f;
float stabilizedTargetStep = 0.0f;
float targetStepFromDisplay = 0.0f;
bool targetSlewInitialized = false;

float virtualNeedleStep = 0.0f;
float virtualNeedleVelocity = 0.0f;

int32_t lastCommandedMotorPosition = -1;
unsigned long lastControlUpdateMicros = 0;
bool speedTimeoutZeroActive = false;

enum class GaugeState : uint8_t {
  DriverOff,
  Running,
  ReturningToZero,
  HoldingAtZero
};

GaugeState gaugeState = GaugeState::DriverOff;
unsigned long zeroHoldStartMs = 0;

const unsigned long KEY_DEBOUNCE_MS = 20UL;
bool keyRawState = false;
bool keyStableState = false;
unsigned long keyLastChangeMs = 0;

void setup()
{

  pinMode(PIN_KEY_ON, INPUT);
  pinMode(PIN_SPEED_INPUT, INPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, HIGH);

  speedMotor.begin();

  keyRawState = readKeyOnRaw();
  keyStableState = keyRawState;
  keyLastChangeMs = millis();

  if (isKeyOnConfirmedAtStartup()) {
    startGaugeFromDriverOff();
  }

  attachInterrupt(
      digitalPinToInterrupt(PIN_SPEED_INPUT),
      onSpeedPulse,
      RISING);
}

void loop()
{

  speedMotor.update();

  const bool keyOn = updateAndReadKeyOn();

  switch (gaugeState) {
    case GaugeState::DriverOff:
      if (keyOn) {
        startGaugeFromDriverOff();
      }
      return;

    case GaugeState::Running:
      if (!keyOn) {
        beginKeyOffReturn();
        return;
      }

      runV1UpperControl();
      return;

