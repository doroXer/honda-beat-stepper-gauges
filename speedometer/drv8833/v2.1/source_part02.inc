  {-212, -142},
  {-225, -120},
  {-236,  -98},
  {-244,  -74},
  {-250,  -50},
  {-254,  -25},
  {-255,    0},
  {-254,   25},
  {-250,   50},
  {-244,   74},
  {-236,   98},
  {-225,  120},
  {-212,  142},
  {-197,  162},
  {-181,  181},
  {-162,  197},
  {-142,  212},
  {-120,  225},
  { -98,  236},
  { -74,  244},
  { -50,  250},
  { -25,  254}
};

class X27SixteenthStep
{
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

    configureTimer1Pwm();

    currentPosition_ = 0;
    targetPosition_ = 0;
    currentPhase_ = 0;

    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    lastMotionUpdateUs_ = micros();

    enabled_ = false;
    wasMoving_ = false;
  }

  void setPosition(int32_t position)
  {
    targetPosition_ = constrain(
        position,
        (int32_t)0,
        MOTOR_MAX_MICROSTEPS);
  }

  void update()
  {
    updateMotion(
        MOTOR_MAX_SPEED_MICROSTEP_PER_SEC,
        MOTOR_ACCEL_MICROSTEP_PER_SEC2);
  }

  void updateBlocking()
  {
    if (!enabled_) {
      enable();
    }

    while (!isAtTarget()) {
      updateMotion(
          BLOCKING_MAX_SPEED_MICROSTEP_PER_SEC,
          BLOCKING_ACCEL_MICROSTEP_PER_SEC2);

      delayMicroseconds(50);
    }

    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    applyPhase(currentPhase_, HOLD_PWM_MAX);
  }

  void zero()
  {
    if (!enabled_) {
      enable();
    }

    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    wasMoving_ = false;

    for (int32_t i = 0; i < ZERO_TRAVEL_MICROSTEPS_ALIGNED; i++) {
      emitLogicalMicrostep(-1, ZERO_PWM_MAX);
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

    motorVelocity_ = 0.0f;
    stepAccumulator_ = 0.0f;
    enabled_ = false;
    wasMoving_ = false;
  }

  bool isEnabled() const
  {
    return enabled_;
  }

  bool isAtTarget() const
  {
    return currentPosition_ == targetPosition_ &&
           fabsf(motorVelocity_) < 0.5f;
  }

  int32_t getCurrentPosition() const
  {
    return currentPosition_;
  }

  int32_t getTargetPosition() const
  {
    return targetPosition_;
  }

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

  void holdCurrentPosition()
  {
    targetPosition_ = currentPosition_;
    stopMotion();
  }

private:
  int32_t currentPosition_ = 0;
  int32_t targetPosition_ = 0;
  int8_t currentPhase_ = 0;

  float motorVelocity_ = 0.0f;
  float stepAccumulator_ = 0.0f;
  uint32_t lastMotionUpdateUs_ = 0;

  bool enabled_ = false;
  bool wasMoving_ = false;

  void configureTimer1Pwm()
  {

    TCCR1A = _BV(WGM10);
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  }

  void updateMotion(float maximumSpeed, float acceleration)
  {
    if (!enabled_) {
      return;
    }

    const uint32_t nowUs = micros();

    float dtSec =
        (float)(uint32_t)(nowUs - lastMotionUpdateUs_) /
        1000000.0f;

    lastMotionUpdateUs_ = nowUs;

    if (dtSec <= 0.0f) {
      return;
    }

    if (dtSec > MOTOR_MAX_UPDATE_DT_SEC) {
      dtSec = MOTOR_MAX_UPDATE_DT_SEC;
    }

    const int32_t error = targetPosition_ - currentPosition_;

    if (error == 0 && fabsf(motorVelocity_) < 0.5f) {
      motorVelocity_ = 0.0f;
      stepAccumulator_ = 0.0f;

      if (wasMoving_) {
        applyPhase(currentPhase_, HOLD_PWM_MAX);
        wasMoving_ = false;
      }

      return;
    }

    const int8_t desiredDirection =
        error > 0 ? 1 :
        error < 0 ? -1 : 0;

    const int8_t velocityDirection =
