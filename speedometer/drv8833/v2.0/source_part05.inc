    const float dtSec =
        CONTROL_UPDATE_INTERVAL_US / 1000000.0f;

    const float diff =
        rawTargetStepFromDisplay - stabilizedTargetStep;

    if (diff > 0.0f) {
      const float maxUp =
          TARGET_STEP_MAX_UP_PER_SEC * dtSec;

      stabilizedTargetStep += minFloat(diff, maxUp);
    }
    else if (diff < 0.0f) {
      const float maxDown =
          TARGET_STEP_MAX_DOWN_PER_SEC * dtSec;

      stabilizedTargetStep -= minFloat(-diff, maxDown);
    }
  }

  stabilizedTargetStep =
      constrainFloat(
          stabilizedTargetStep,
          0.0f,
          (float)MOTOR_STEPS);

  targetStepFromDisplay = stabilizedTargetStep;
}

void updateVirtualNeedleControl(unsigned long nowMicros)
{
  float dtSec =
      (nowMicros - lastControlUpdateMicros) /
      1000000.0f;

  if (dtSec <= 0.0f || dtSec > 0.5f) {
    dtSec = CONTROL_UPDATE_INTERVAL_US / 1000000.0f;
  }

  const float diff =
      targetStepFromDisplay - virtualNeedleStep;

  if (absFloat(diff) <= VIRTUAL_STOP_BAND_STEP &&
      absFloat(virtualNeedleVelocity) < 1.0f) {

    virtualNeedleStep = targetStepFromDisplay;
    virtualNeedleVelocity = 0.0f;
    commandMotorPosition(virtualNeedleStep);
    return;
  }

  float desiredVelocity = diff / dtSec;

  desiredVelocity = constrainFloat(
      desiredVelocity,
      -VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC,
       VIRTUAL_MAX_VEL_UP_STEP_PER_SEC);

  float maxVelocityChange;

  if (desiredVelocity > virtualNeedleVelocity) {
    maxVelocityChange =
        VIRTUAL_ACCEL_UP_STEP_PER_SEC2 * dtSec;

    virtualNeedleVelocity +=
        minFloat(
            maxVelocityChange,
            desiredVelocity - virtualNeedleVelocity);
  }
  else {
    maxVelocityChange =
        VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 * dtSec;

    virtualNeedleVelocity -=
        minFloat(
            maxVelocityChange,
            virtualNeedleVelocity - desiredVelocity);
  }

  float nextStep =
      virtualNeedleStep +
      virtualNeedleVelocity * dtSec;

  if ((diff > 0.0f && nextStep > targetStepFromDisplay) ||
      (diff < 0.0f && nextStep < targetStepFromDisplay)) {

    nextStep = targetStepFromDisplay;
    virtualNeedleVelocity = 0.0f;
  }

  virtualNeedleStep =
      constrainFloat(
          nextStep,
          0.0f,
          (float)MOTOR_STEPS);

  commandMotorPosition(virtualNeedleStep);
}

void commandMotorPosition(float logicalStep)
{

  const int32_t motorPosition =
      logicalStepToMotorPosition(logicalStep);

  if (motorPosition != lastCommandedMotorPosition) {
    speedMotor.setPosition(motorPosition);
    lastCommandedMotorPosition = motorPosition;
  }
}

void forceZeroToStop()
{
  speedMotor.zero();
  speedMotor.setPosition(0);
  lastCommandedMotorPosition = 0;
}

void openingDemo()
{
  commandMotorPosition((float)MOTOR_STEPS);
  speedMotor.updateBlocking();
  delay(OPENING_HOLD_MS);

  commandMotorPosition(0.0f);
  speedMotor.updateBlocking();
  delay(OPENING_HOLD_MS);
}

void resetDisplayState()
{
  rawSpeedValue = 0;
  filteredDisplayValue = 0.0f;
  rawTargetStepFromDisplay = 0.0f;
  stabilizedTargetStep = 0.0f;
  targetStepFromDisplay = 0.0f;
  targetSlewInitialized = false;

  virtualNeedleStep = 0.0f;
  virtualNeedleVelocity = 0.0f;
  lastCommandedMotorPosition = 0;

  lastControlUpdateMicros = micros();
  speedTimeoutZeroActive = false;

  noInterrupts();
  lastPulseMicros = 0;
  lastPulseSeenMicros = 0;
  latestPulseIntervalMicros = 0;
  latestRawPulseIntervalMicros = 0;
  pulseIntervalUpdated = false;
  interrupts();
}

void resetDisplayStateAtPosition(float logicalPosition)
{
  logicalPosition =
      constrainFloat(
          logicalPosition,
          0.0f,
          (float)MOTOR_STEPS);

  rawSpeedValue = 0;
  filteredDisplayValue = 0.0f;

  rawTargetStepFromDisplay = logicalPosition;
  stabilizedTargetStep = logicalPosition;
  targetStepFromDisplay = logicalPosition;
  targetSlewInitialized = true;

  virtualNeedleStep = logicalPosition;
  virtualNeedleVelocity = 0.0f;

  lastCommandedMotorPosition =
      logicalStepToMotorPosition(logicalPosition);

  lastControlUpdateMicros = micros();
  speedTimeoutZeroActive = false;

  noInterrupts();
  lastPulseMicros = 0;
  lastPulseSeenMicros = 0;
  latestPulseIntervalMicros = 0;
  latestRawPulseIntervalMicros = 0;
  pulseIntervalUpdated = false;
  interrupts();
}

void onSpeedPulse()
{
  const unsigned long nowMicros = micros();
  speedPulseCountTotal++;

  if (lastPulseMicros != 0) {
    const unsigned long interval =
