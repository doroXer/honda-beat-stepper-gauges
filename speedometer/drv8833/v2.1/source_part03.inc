        motorVelocity_ > 0.0f ? 1 :
        motorVelocity_ < 0.0f ? -1 : 0;

    if (desiredDirection != 0 &&
        velocityDirection != 0 &&
        desiredDirection != velocityDirection) {

      motorVelocity_ = 0.0f;
      stepAccumulator_ = 0.0f;
    }

    const float speedAbs = fabsf(motorVelocity_);
    const float stoppingDistance =
        (speedAbs * speedAbs) /
        (2.0f * acceleration);

    float accelerationCommand = 0.0f;

    if (desiredDirection == 0) {
      accelerationCommand =
          -(float)velocityDirection * acceleration;
    }
    else if (velocityDirection != 0 &&
             velocityDirection != desiredDirection) {
      accelerationCommand =
          (float)desiredDirection * acceleration;
    }
    else if ((float)labs(error) <= stoppingDistance + 1.0f) {
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

      emitLogicalMicrostep(stepDirection, RUN_PWM_MAX);
      currentPosition_ += stepDirection;
      stepAccumulator_ -= (float)stepDirection;

      wasMoving_ = true;
      emittedSteps++;

      if (currentPosition_ == targetPosition_) {
        motorVelocity_ = 0.0f;
        stepAccumulator_ = 0.0f;
        break;
      }
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
    phaseIndex %= (int8_t)PHASE_COUNT;

    if (phaseIndex < 0) {
      phaseIndex += (int8_t)PHASE_COUNT;
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

  void emitLogicalMicrostep(
      int8_t logicalDirection,
      uint8_t pwmMaximum)
  {
    currentPhase_ += logicalDirection * SCALE_DIRECTION;

    if (currentPhase_ >= (int8_t)PHASE_COUNT) {
      currentPhase_ = 0;
    }
    else if (currentPhase_ < 0) {
      currentPhase_ = (int8_t)PHASE_COUNT - 1;
    }

    applyPhase(currentPhase_, pwmMaximum);
  }
};

X27SixteenthStep speedMotor;

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

