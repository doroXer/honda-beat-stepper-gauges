#include <Arduino_FreeRTOS.h>
#include <math.h>
#include "SwitecX25.h"

/*
  Honda Beat fuel / coolant temperature meter driver
  v1.0 - FreeRTOS single gauge-drive task version / FreeRTOS・単一GaugeDriveタスク版

  Base / ベース:
    - fuel_temp_v0_1.ino

  FreeRTOS library / FreeRTOSライブラリ:
    - Arduino_FreeRTOS_Library
    - Richard Barry / Phillip Stevens / feilipu

  Hardware / ハードウェア:
    - Arduino Pro Mini 5V / 16MHz
    - X27.168 / X25 compatible stepper motors
    - SwitecX25 library

  Meter assignment / メーター割り当て:
    - motorTemp : coolant temperature meter, pins D4-D7, sensor input A0
    - motorFuel : fuel meter, pins D8-D11, sensor input A1

  FreeRTOS task assignment / FreeRTOSタスク割り当て:
    - TaskGaugeDrive : owns both SwitecX25 motor objects
        * startup opening sweep for both meters at the same time
        * zero calibration one meter at a time
        * continuous motorTemp.update() and motorFuel.update()
        * applies target positions requested by the logic tasks

    - TaskTempLogic : coolant ADC / filter / target calculation only
    - TaskFuelLogic : fuel ADC / filter / target calculation only

  Purpose / 目的:
    - Avoid uneven motor.update() timing caused by splitting the two meters into
      separate motor-driving tasks.
    - Keep both motor.update() calls adjacent and in a fixed order.
    - Keep all direct SwitecX25 access inside TaskGaugeDrive.
    - Apply the previously used fuel / coolant acceleration table to both motors.

  Notes / 注意:
    - No key-off monitoring is included.
    - This fuel / coolant temperature meter power supply is not held after key-off.
    - loop() is not used after vTaskStartScheduler().
*/

// ----- Pin assignment -----
const byte TEMP_SENSOR_PIN = A0;
const byte FUEL_SENSOR_PIN = A1;

// ----- Meter configuration -----
const unsigned int STEPS_PER_DEG = 3;
const unsigned int METER_RANGE_DEG = 120;
const unsigned int METER_STEPS = METER_RANGE_DEG * STEPS_PER_DEG;

const unsigned int OPENING_DEG = 100;
const unsigned int OPENING_STEPS = OPENING_DEG * STEPS_PER_DEG;

const unsigned long OPENING_SETTLE_MS = 500;
const unsigned long ZERO_SETTLE_MS = 100;
const unsigned long SAMPLE_INTERVAL_MS = 1000;

// Strong filtering is intentional for fuel and coolant temperature gauges.
const float FILTER_NEW = 1.0 / 60.0;
const float FILTER_OLD = 59.0 / 60.0;

// ----- FreeRTOS configuration -----
// Stack size is in words, not bytes.
// Arduino Pro Mini / ATmega328P SRAM is small, so keep task stacks modest.
const uint16_t TASK_STACK_WORDS_DRIVE = 128;
const uint16_t TASK_STACK_WORDS_LOGIC = 128;

// All tasks use the same priority intentionally.
// TaskGaugeDrive uses taskYIELD() instead of vTaskDelay(1), because AVR FreeRTOS
// tick timing may be too coarse for smooth SwitecX25 updates.
// Same-priority tasks allow the 1-second logic tasks to run when they wake.
const UBaseType_t TASK_PRIORITY_GAUGE = 1;
const UBaseType_t TASK_PRIORITY_LOGIC = 1;

// ----- Motor objects -----
// Only TaskGaugeDrive should call methods on these objects.
SwitecX25 motorTemp(METER_STEPS, 4, 5, 6, 7);
SwitecX25 motorFuel(METER_STEPS, 8, 9, 10, 11);

// Slower acceleration table selected during fuel / coolant meter testing.
// The commented values are earlier reference values retained for comparison.
// 燃料計・水温計の実機テストで選定した低速側の加速テーブルです。
// コメント値は比較用に残した以前の参照値です。
unsigned short meterAccelTable[][2] = {
  { 20, 8000 },  // former reference: 4500
  { 50, 4400 },  // former reference: 2600
  {100, 3200 },  // former reference: 1800
  {150, 2400 },  // former reference: 1400
  {300, 1900 }   // former reference: 1100
};

const byte meterAccelTableSize = sizeof(meterAccelTable) / sizeof(meterAccelTable[0]);

// ----- Shared state between logic tasks and drive task -----
// Logic tasks write these target values.
// TaskGaugeDrive reads them and applies them to the motors.
volatile unsigned int requestedTempStep = 0;
volatile unsigned int requestedFuelStep = 0;
volatile bool gaugeInitDone = false;

float tempFiltered = 0.0;
float fuelFiltered = 0.0;

// ----- Utility functions -----
float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

unsigned int angleToSteps(float angleDeg) {
  return (unsigned int)(angleDeg * STEPS_PER_DEG + 0.5);
}

void applyMotorSettings() {
  motorTemp.accelTable = meterAccelTable;
  motorTemp.maxVel = meterAccelTable[meterAccelTableSize - 1][0];

  motorFuel.accelTable = meterAccelTable;
  motorFuel.maxVel = meterAccelTable[meterAccelTableSize - 1][0];
}

float readFilteredAdc(byte pin, float previousValue) {
  float rawValue = analogRead(pin);
  return rawValue * FILTER_NEW + previousValue * FILTER_OLD;
}

unsigned int coolantTempSteps(float tempAdc) {
  // Installation-specific calibrated ADC-to-angle relationship.
  // This is not a universal coolant-sensor transfer function; recalibrate if
  // the sensor, input circuit, ADC reference, supply, wiring, or gauge geometry changes.
  // 本プロジェクトの実装条件で校正したADC→角度換算です。一般的な水温センサーの
  // 普遍的な特性式ではないため、センサー、入力回路、ADC基準、電源、配線、
  // メーター形状を変更した場合は再校正してください。

  // Protect log() from zero or negative values caused by wiring faults or ADC noise.
  if (tempAdc < 1.0) tempAdc = 1.0;

  float angleDeg = -76.732 * log(tempAdc) + 520.0;
  angleDeg = clampFloat(angleDeg, 0.0, 115.0);

  return angleToSteps(angleDeg);
}

unsigned int fuelSteps(float fuelAdc) {
  // Installation-specific calibrated ADC-to-angle relationship.
  // This is not a universal fuel-sender transfer function; recalibrate if
  // the sender, input circuit, ADC reference, supply, wiring, or gauge geometry changes.
  // 本プロジェクトの実装条件で校正したADC→角度換算です。一般的な燃料センダーの
  // 普遍的な特性式ではないため、センダー、入力回路、ADC基準、電源、配線、
  // メーター形状を変更した場合は再校正してください。

  float angleDeg = -0.1744 * fuelAdc + 115.87;
  angleDeg = clampFloat(angleDeg, 0.0, 115.0);

  return angleToSteps(angleDeg);
}

void setRequestedTempStep(unsigned int stepValue) {
  taskENTER_CRITICAL();
  requestedTempStep = stepValue;
  taskEXIT_CRITICAL();
}

void setRequestedFuelStep(unsigned int stepValue) {
  taskENTER_CRITICAL();
  requestedFuelStep = stepValue;
  taskEXIT_CRITICAL();
}

void getRequestedSteps(unsigned int *tempStep, unsigned int *fuelStep) {
  taskENTER_CRITICAL();
  *tempStep = requestedTempStep;
  *fuelStep = requestedFuelStep;
  taskEXIT_CRITICAL();
}

bool isGaugeInitDone() {
  bool result;

  taskENTER_CRITICAL();
  result = gaugeInitDone;
  taskEXIT_CRITICAL();

  return result;
}

void setGaugeInitDone() {
  taskENTER_CRITICAL();
  gaugeInitDone = true;
  taskEXIT_CRITICAL();
}

void updateBothMotorsOnce() {
  motorTemp.update();
  motorFuel.update();
}

void runOpeningBothMotors() {
  motorTemp.setPosition(OPENING_STEPS);
  motorFuel.setPosition(OPENING_STEPS);

  while (motorTemp.currentStep != motorTemp.targetStep ||
         motorFuel.currentStep != motorFuel.targetStep) {
    updateBothMotorsOnce();
    taskYIELD();
  }

  unsigned long startMs = millis();
  while (millis() - startMs < OPENING_SETTLE_MS) {
    updateBothMotorsOnce();
    taskYIELD();
  }
}

void zeroMotorsSequentially() {
  // zero() is kept sequential.
  // Opening is the visible simultaneous motion; zero calibration does not need
  // to be simultaneous, and sequential zero reduces timing disturbance.
  motorTemp.zero();
  vTaskDelay(pdMS_TO_TICKS(ZERO_SETTLE_MS));

  motorFuel.zero();
  vTaskDelay(pdMS_TO_TICKS(ZERO_SETTLE_MS));
}

// ----- FreeRTOS tasks -----
void TaskGaugeDrive(void *pvParameters) {
  (void) pvParameters;

  applyMotorSettings();

  // Startup sequence owned by this single motor-drive task.
  // Both motor updates are adjacent and ordered, so opening motion should be
  // smoother than the previous two-motor-task version.
  runOpeningBothMotors();
  zeroMotorsSequentially();

  // After zero(), both meters are at 0 until the logic tasks request positions.
  setRequestedTempStep(0);
  setRequestedFuelStep(0);
  setGaugeInitDone();

  unsigned int appliedTempStep = 0;
  unsigned int appliedFuelStep = 0;

  for (;;) {
    unsigned int newTempStep;
    unsigned int newFuelStep;
    getRequestedSteps(&newTempStep, &newFuelStep);

    if (newTempStep != appliedTempStep) {
      appliedTempStep = newTempStep;
      motorTemp.setPosition(appliedTempStep);
    }

    if (newFuelStep != appliedFuelStep) {
      appliedFuelStep = newFuelStep;
      motorFuel.setPosition(appliedFuelStep);
    }

    updateBothMotorsOnce();

    // Do not use vTaskDelay(1) here unless testing shows CPU starvation.
    // On AVR FreeRTOS, a 1-tick delay can be much too coarse for smooth meter motion.
    taskYIELD();
  }
}

void TaskTempLogic(void *pvParameters) {
  (void) pvParameters;

  while (!isGaugeInitDone()) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  tempFiltered = analogRead(TEMP_SENSOR_PIN);
  setRequestedTempStep(coolantTempSteps(tempFiltered));

  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));

    tempFiltered = readFilteredAdc(TEMP_SENSOR_PIN, tempFiltered);
    setRequestedTempStep(coolantTempSteps(tempFiltered));
  }
}

void TaskFuelLogic(void *pvParameters) {
  (void) pvParameters;

  while (!isGaugeInitDone()) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  fuelFiltered = analogRead(FUEL_SENSOR_PIN);
  setRequestedFuelStep(fuelSteps(fuelFiltered));

  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));

    fuelFiltered = readFilteredAdc(FUEL_SENSOR_PIN, fuelFiltered);
    setRequestedFuelStep(fuelSteps(fuelFiltered));
  }
}

void setup() {
  xTaskCreate(
    TaskGaugeDrive,
    "Gauge",
    TASK_STACK_WORDS_DRIVE,
    NULL,
    TASK_PRIORITY_GAUGE,
    NULL
  );

  xTaskCreate(
    TaskTempLogic,
    "Temp",
    TASK_STACK_WORDS_LOGIC,
    NULL,
    TASK_PRIORITY_LOGIC,
    NULL
  );

  xTaskCreate(
    TaskFuelLogic,
    "Fuel",
    TASK_STACK_WORDS_LOGIC,
    NULL,
    TASK_PRIORITY_LOGIC,
    NULL
  );

  vTaskStartScheduler();

  // Normally not reached.
  // If reached, task creation probably failed because of insufficient SRAM.
  for (;;) {
  }
}

void loop() {
  // Not used. FreeRTOS tasks run after vTaskStartScheduler().
}