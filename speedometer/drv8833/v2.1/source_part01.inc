#include <Arduino.h>
#include <math.h>

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
void updateRawSpeedValueFromPulseInterval();
void updateDisplayFilter();
void updateTargetStepFromDisplay();
void updateVirtualNeedleControl(unsigned long nowMicros);
void commandMotorPosition(float logicalStep);

void forceZeroToStop();
void openingDemo();
void resetDisplayState();
void resetDisplayStateAtPosition(float logicalPosition);
void onSpeedPulse();

const int MOTOR_STEPS = 240 * 3;

const uint8_t PIN_SPEED_INPUT = 2;
const uint8_t PIN_KEY_ON = 3;
const uint8_t PIN_LED = 13;

const uint8_t PIN_A_NEG = 5;
const uint8_t PIN_A_POS = 6;
const uint8_t PIN_STBY  = 7;
const uint8_t PIN_B_POS = 9;
const uint8_t PIN_B_NEG = 10;

const uint8_t KEY_ON_ACTIVE_LEVEL = HIGH;

const int8_t SCALE_DIRECTION = -1;

const long SPEED_STEP_NUMERATOR = 7480000L;
const int SPEED_STEP_OFFSET = 36;

const byte DISPLAY_FILTER_OLD = 7;
const byte DISPLAY_FILTER_NEW = 1;

const unsigned long CONTROL_UPDATE_INTERVAL_US = 50000UL;

const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC = 500.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC = 500.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2 = 6000.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 = 6000.0f;
const float VIRTUAL_STOP_BAND_STEP = 3.0f;

const float TARGET_STEP_MAX_UP_PER_SEC = 20.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC = 45.0f;

const unsigned long SPEED_PULSE_TIMEOUT_US = 500000UL;
const unsigned long MIN_VALID_INTERVAL_US = 1000UL;

const bool ENABLE_OPENING_DEMO = true;
const bool FORCE_ZERO_AFTER_OPENING = true;

const int STARTUP_KEY_CHECK_COUNT = 5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS = 20;
const unsigned int STARTUP_KEY_SETTLE_DELAY_MS = 100;
const unsigned int OPENING_HOLD_MS = 200;

const int32_t MOTOR_POSITION_RATIO_NUMERATOR = 32;
const int32_t MOTOR_POSITION_RATIO_DENOMINATOR = 3;

const int32_t MOTOR_MAX_MICROSTEPS =
    ((int32_t)MOTOR_STEPS * MOTOR_POSITION_RATIO_NUMERATOR) /
    MOTOR_POSITION_RATIO_DENOMINATOR;

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
      MOTOR_MAX_MICROSTEPS);

  return
      (float)motorPosition *
      (float)MOTOR_POSITION_RATIO_DENOMINATOR /
      (float)MOTOR_POSITION_RATIO_NUMERATOR;
}

const uint8_t RUN_PWM_MAX = 255;
const uint8_t HOLD_PWM_MAX = 150;
const uint8_t ZERO_PWM_MAX = 200;

const float MOTOR_MAX_SPEED_MICROSTEP_PER_SEC = 3600.0f;
const float MOTOR_ACCEL_MICROSTEP_PER_SEC2 = 9600.0f;

const float BLOCKING_MAX_SPEED_MICROSTEP_PER_SEC = 11200.0f;
const float BLOCKING_ACCEL_MICROSTEP_PER_SEC2 = 40000.0f;

const float MOTOR_MAX_UPDATE_DT_SEC = 0.020f;
const uint8_t MOTOR_MAX_EMITTED_STEPS_PER_UPDATE = 32;

const int32_t ZERO_TRAVEL_MARGIN_MICROSTEPS = 768;
const int32_t ZERO_TRAVEL_MICROSTEPS =
    MOTOR_MAX_MICROSTEPS + ZERO_TRAVEL_MARGIN_MICROSTEPS;
const uint16_t ZERO_STEP_INTERVAL_US = 225;
const uint8_t PHASE_COUNT = 64;

const int32_t ZERO_TRAVEL_MICROSTEPS_ALIGNED =
    ((ZERO_TRAVEL_MICROSTEPS + (int32_t)PHASE_COUNT - 1L) /
     (int32_t)PHASE_COUNT) * (int32_t)PHASE_COUNT;

const unsigned long KEYOFF_ZERO_HOLD_MS = 200UL;

struct PhaseValue {
  int16_t phaseA;
  int16_t phaseB;
};

const PhaseValue PHASE_TABLE[PHASE_COUNT] = {
  {   0,  255},
  {  25,  254},
  {  50,  250},
  {  74,  244},
  {  98,  236},
  { 120,  225},
  { 142,  212},
  { 162,  197},
  { 181,  181},
  { 197,  162},
  { 212,  142},
  { 225,  120},
  { 236,   98},
  { 244,   74},
  { 250,   50},
  { 254,   25},
  { 255,    0},
  { 254,  -25},
  { 250,  -50},
  { 244,  -74},
  { 236,  -98},
  { 225, -120},
  { 212, -142},
  { 197, -162},
  { 181, -181},
  { 162, -197},
  { 142, -212},
  { 120, -225},
  {  98, -236},
  {  74, -244},
  {  50, -250},
  {  25, -254},
  {   0, -255},
  { -25, -254},
  { -50, -250},
  { -74, -244},
  { -98, -236},
  {-120, -225},
  {-142, -212},
  {-162, -197},
  {-181, -181},
  {-197, -162},
