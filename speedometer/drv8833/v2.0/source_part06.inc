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
