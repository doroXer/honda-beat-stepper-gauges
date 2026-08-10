    case GaugeState::ReturningToZero:
      if (keyOn) {
        resumeRunningDuringKeyOffReturn();
        return;
      }

      if (speedMotor.isAtTarget()) {
        zeroHoldStartMs = millis();
        gaugeState = GaugeState::HoldingAtZero;
      }
      return;

    case GaugeState::HoldingAtZero:
      if (keyOn) {
        resumeRunningDuringKeyOffReturn();
        return;
      }

      if ((unsigned long)(millis() - zeroHoldStartMs) >=
          KEYOFF_ZERO_HOLD_MS) {
        speedMotor.disable();
        digitalWrite(PIN_LED, HIGH);
        gaugeState = GaugeState::DriverOff;
      }
      return;
  }
}

bool readKeyOnRaw()
{
  return digitalRead(PIN_KEY_ON) == KEY_ON_ACTIVE_LEVEL;
}

bool updateAndReadKeyOn()
{
  const bool raw = readKeyOnRaw();
  const unsigned long nowMs = millis();

  if (raw != keyRawState) {
    keyRawState = raw;
    keyLastChangeMs = nowMs;
  }

  if ((unsigned long)(nowMs - keyLastChangeMs) >= KEY_DEBOUNCE_MS) {
    keyStableState = keyRawState;
  }

  return keyStableState;
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

void startGaugeFromDriverOff()
{
  speedMotor.enable();
  digitalWrite(PIN_LED, LOW);

  delay(STARTUP_KEY_SETTLE_DELAY_MS);

  forceZeroToStop();
  resetDisplayState();

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
          speedMotor.getCurrentPosition());

  resetDisplayStateAtPosition(currentLogicalPosition);

  speedMotor.stopMotion();
  commandMotorPosition(0.0f);

  digitalWrite(PIN_LED, HIGH);
  gaugeState = GaugeState::ReturningToZero;
}

void resumeRunningDuringKeyOffReturn()
{

  speedMotor.holdCurrentPosition();

  const float currentLogicalPosition =
      motorPositionToLogicalStep(
          speedMotor.getCurrentPosition());

  resetDisplayStateAtPosition(currentLogicalPosition);

  digitalWrite(PIN_LED, LOW);
  gaugeState = GaugeState::Running;
}

void runV1UpperControl()
{
  updateRawSpeedValueFromPulseInterval();

  const unsigned long nowMicros = micros();

  if ((unsigned long)(nowMicros - lastControlUpdateMicros) >=
      CONTROL_UPDATE_INTERVAL_US) {

    updateDisplayFilter();
    updateTargetStepFromDisplay();
    updateVirtualNeedleControl(nowMicros);
    lastControlUpdateMicros = nowMicros;
  }
}

void updateRawSpeedValueFromPulseInterval()
{
  const unsigned long nowMicros = micros();

  unsigned long intervalCopy = 0;
  unsigned long lastSeenCopy = 0;
  bool hasUpdatedInterval = false;

  noInterrupts();
  intervalCopy = latestPulseIntervalMicros;
  lastSeenCopy = lastPulseSeenMicros;
  hasUpdatedInterval = pulseIntervalUpdated;
  pulseIntervalUpdated = false;
  interrupts();

  unsigned long ageMicros = 0;

  if (lastSeenCopy == 0) {
    ageMicros = 0;
  }
  else if (nowMicros >= lastSeenCopy) {
    ageMicros = nowMicros - lastSeenCopy;
  }
  else {

    ageMicros = 0;
  }

  const bool timeoutNow =
      (lastSeenCopy == 0 || ageMicros > SPEED_PULSE_TIMEOUT_US);

  if (timeoutNow) {
    rawSpeedValue = 0;
    speedTimeoutZeroActive = true;
    return;
  }

  if (hasUpdatedInterval && intervalCopy >= MIN_VALID_INTERVAL_US) {
    const unsigned long value =
        SPEED_STEP_NUMERATOR / intervalCopy;

    rawSpeedValue = constrain(
        value,
        0UL,
        (unsigned long)(MOTOR_STEPS + SPEED_STEP_OFFSET));

    speedTimeoutZeroActive = false;
  }
}

void updateDisplayFilter()
{

  if (filteredDisplayValue <= 0.0f) {
    filteredDisplayValue = (float)rawSpeedValue;
    return;
  }

  filteredDisplayValue =
      (filteredDisplayValue * DISPLAY_FILTER_OLD +
       (float)rawSpeedValue * DISPLAY_FILTER_NEW) /
      (DISPLAY_FILTER_OLD + DISPLAY_FILTER_NEW);

  if (speedTimeoutZeroActive && filteredDisplayValue < 0.5f) {
    filteredDisplayValue = 0.0f;
  }
}

void updateTargetStepFromDisplay()
{
  const float calcStep =
      filteredDisplayValue - (float)SPEED_STEP_OFFSET;

  rawTargetStepFromDisplay =
      constrainFloat(
          calcStep,
          0.0f,
          (float)MOTOR_STEPS);

  if (!targetSlewInitialized) {
    stabilizedTargetStep = rawTargetStepFromDisplay;
    targetSlewInitialized = true;
  }
  else {
