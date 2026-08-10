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
        nowMicros - lastPulseMicros;

    latestRawPulseIntervalMicros = interval;

    if (interval >= MIN_VALID_INTERVAL_US) {
      latestPulseIntervalMicros = interval;
      pulseIntervalUpdated = true;
      speedPulseCountValid++;
    }
    else {
      speedPulseCountRejectedShort++;
    }
  }

  lastPulseMicros = nowMicros;
  lastPulseSeenMicros = nowMicros;
}

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
  return (a < b) ? a : b;
}

float absFloat(float value)
{
  return (value < 0.0f) ? -value : value;
}
