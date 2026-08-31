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
  Honda Beat Tachometer v2.1 - analyzed parameter test
  ホンダビート タコメーター v2.1 - 解析パラメータ実車テスト

  Base / ベース:
    meter/tach/tach_v2.1/tach_v2_1/tach_v2_1.ino

  Purpose / 目的:
    Validate parameter candidates derived from real-vehicle log analysis
    and simulation without changing the v2.1 production baseline.
    実車ログ解析とシミュレーションから導出した候補パラメータを、
    v2.1本体を変更せず実車で検証するためのテストコードです。

  Changed parameters / 変更パラメータ:
    TARGET UP/DOWN      200/100 -> 400/180
    VIRTUAL VEL UP/DOWN 150/150 -> 400/200
    VIRTUAL ACC UP/DOWN 500/500 -> 2000/1500
    Control interval    100 ms  -> 100 ms (unchanged)
*/

const int MOTOR_STEPS = 240 * 3;

const uint8_t PIN_TACH_INPUT = 2;
const uint8_t PIN_KEY_ON = 3;
const uint8_t PIN_LED = 13;

const uint8_t PIN_A_NEG = 5;
const uint8_t PIN_A_POS = 6;
const uint8_t PIN_STBY  = 7;
const uint8_t PIN_B_POS = 9;
const uint8_t PIN_B_NEG = 10;

const uint8_t KEY_ON_ACTIVE_LEVEL = HIGH;
const int8_t SCALE_DIRECTION = -1;

const int STEP_OFFSET = 23;
const long STEP_CONVERSION_NUMERATOR = 2952000L;

const byte DISPLAY_FILTER_OLD = 7;
const byte DISPLAY_FILTER_NEW = 1;

const unsigned long CONTROL_UPDATE_INTERVAL_US = 100000UL;

const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC = 400.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC = 200.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2 = 2000.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2 = 1500.0f;
const float VIRTUAL_STOP_BAND_STEP = 5.0f;

const float TARGET_STEP_MAX_UP_PER_SEC = 400.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC = 180.0f;

const bool ENABLE_OPENING_DEMO = true;
const bool FORCE_ZERO_AFTER_OPENING = true;

const int STARTUP_KEY_CHECK_COUNT = 5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS = 20;
const unsigned int STARTUP_KEY_SETTLE_DELAY_MS = 300;

const unsigned long NO_PULSE_STOP_TIMEOUT_US = 1000000UL;

const byte ENGINE_RUN_CONFIRM_COUNT = 20;
const unsigned long MIN_VALID_INTERVAL_US = 3000UL;
const unsigned long MAX_VALID_INTERVAL_US = 300000UL;

const int32_t MOTOR_POSITION_RATIO_NUMERATOR = 32;
const int32_t MOTOR_POSITION_RATIO_DENOMINATOR = 3;

const int32_t MOTOR_MAX_SIXTEENTH_STEPS =
    ((int32_t)MOTOR_STEPS * MOTOR_POSITION_RATIO_NUMERATOR) /
    MOTOR_POSITION_RATIO_DENOMINATOR;

int32_t logicalStepToMotorPosition(float logicalStep)
{
  logicalStep = constrainFloat(logicalStep, 0.0f, (float)MOTOR_STEPS);
  return lroundf(logicalStep * (float)MOTOR_POSITION_RATIO_NUMERATOR /
                 (float)MOTOR_POSITION_RATIO_DENOMINATOR);
}

float motorPositionToLogicalStep(int32_t motorPosition)
{
  motorPosition = constrain(motorPosition, (int32_t)0, MOTOR_MAX_SIXTEENTH_STEPS);
  return (float)motorPosition * (float)MOTOR_POSITION_RATIO_DENOMINATOR /
         (float)MOTOR_POSITION_RATIO_NUMERATOR;
}

const uint8_t RUN_PWM_MAX = 255;
const uint8_t HOLD_PWM_MAX = 150;
const uint8_t ZERO_PWM_MAX = 200;

const float MOTOR_MAX_SPEED_SIXTEENTH_STEP_PER_SEC = 3600.0f;
const float MOTOR_ACCEL_SIXTEENTH_STEP_PER_SEC2 = 9600.0f;

const float BLOCKING_MAX_SPEED_SIXTEENTH_STEP_PER_SEC = 16800.0f;
const float BLOCKING_ACCEL_SIXTEENTH_STEP_PER_SEC2 = 90000.0f;

const float MOTOR_MAX_UPDATE_DT_SEC = 0.020f;
const uint8_t MOTOR_MAX_EMITTED_STEPS_PER_UPDATE = 32;

const int32_t ZERO_TRAVEL_MARGIN_SIXTEENTH_STEPS = 768;
const int32_t ZERO_TRAVEL_SIXTEENTH_STEPS =
    MOTOR_MAX_SIXTEENTH_STEPS + ZERO_TRAVEL_MARGIN_SIXTEENTH_STEPS;
const uint16_t ZERO_STEP_INTERVAL_US = 225;
const int32_t ZERO_TRAVEL_SIXTEENTH_STEPS_ALIGNED =
    ((ZERO_TRAVEL_SIXTEENTH_STEPS + 63L) / 64L) * 64L;
const unsigned long KEYOFF_ZERO_HOLD_MS = 200UL;

struct PhaseValue { int16_t phaseA; int16_t phaseB; };

const PhaseValue PHASE_TABLE[64] = {
  {0,255},{25,254},{50,250},{74,244},{98,236},{120,225},{142,212},{162,197},
  {180,180},{197,162},{212,142},{225,120},{236,98},{244,74},{250,50},{254,25},
  {255,0},{254,-25},{250,-50},{244,-74},{236,-98},{225,-120},{212,-142},{197,-162},
  {180,-180},{162,-197},{142,-212},{120,-225},{98,-236},{74,-244},{50,-250},{25,-254},
  {0,-255},{-25,-254},{-50,-250},{-74,-244},{-98,-236},{-120,-225},{-142,-212},{-162,-197},
  {-180,-180},{-197,-162},{-212,-142},{-225,-120},{-236,-98},{-244,-74},{-250,-50},{-254,-25},
  {-255,0},{-254,25},{-250,50},{-244,74},{-236,98},{-225,120},{-212,142},{-197,162},
  {-180,180},{-162,197},{-142,212},{-120,225},{-98,236},{-74,244},{-50,250},{-25,254}
};

class X27SixteenthStep {
public:
  void begin() {
    pinMode(PIN_A_NEG, OUTPUT); pinMode(PIN_A_POS, OUTPUT); pinMode(PIN_STBY, OUTPUT);
    pinMode(PIN_B_POS, OUTPUT); pinMode(PIN_B_NEG, OUTPUT);
    digitalWrite(PIN_STBY, LOW);
    analogWrite(PIN_A_NEG,0); analogWrite(PIN_A_POS,0); analogWrite(PIN_B_POS,0); analogWrite(PIN_B_NEG,0);
    enabled_=false; currentPosition_=0; targetPosition_=0; currentPhase_=0;
    motorVelocity_=0.0f; stepAccumulator_=0.0f; lastMotionUpdateUs_=micros(); wasMoving_=false;
  }
  void setPosition(int32_t position) { targetPosition_=constrain(position,(int32_t)0,MOTOR_MAX_SIXTEENTH_STEPS); }
  int32_t getCurrentPosition() const { return currentPosition_; }
  bool isAtTarget() const { return currentPosition_==targetPosition_ && fabsf(motorVelocity_)<0.5f && fabsf(stepAccumulator_)<1.0f; }
  void update() { updateMotion(MOTOR_MAX_SPEED_SIXTEENTH_STEP_PER_SEC,MOTOR_ACCEL_SIXTEENTH_STEP_PER_SEC2); }
  void updateBlocking() {
    unsigned long safetyStartMs=millis();
    while(!isAtTarget()) {
      updateMotion(BLOCKING_MAX_SPEED_SIXTEENTH_STEP_PER_SEC,BLOCKING_ACCEL_SIXTEENTH_STEP_PER_SEC2);
      delayMicroseconds(50);
      if((unsigned long)(millis()-safetyStartMs)>10000UL) break;
    }
    applyPhase(currentPhase_,HOLD_PWM_MAX);
  }
  void zero() {
    enable(); motorVelocity_=0.0f; stepAccumulator_=0.0f; lastMotionUpdateUs_=micros();
    for(int32_t i=0;i<ZERO_TRAVEL_SIXTEENTH_STEPS_ALIGNED;i++){ emitLogicalSixteenthStep(-1,ZERO_PWM_MAX); delayMicroseconds(ZERO_STEP_INTERVAL_US); }
    currentPosition_=0; targetPosition_=0; motorVelocity_=0.0f; stepAccumulator_=0.0f; lastMotionUpdateUs_=micros(); wasMoving_=false;
    applyPhase(currentPhase_,HOLD_PWM_MAX);
  }
  void enable(){ if(enabled_) return; digitalWrite(PIN_STBY,HIGH); enabled_=true; lastMotionUpdateUs_=micros(); applyPhase(currentPhase_,HOLD_PWM_MAX); }
  void disable(){ analogWrite(PIN_A_NEG,0); analogWrite(PIN_A_POS,0); analogWrite(PIN_B_POS,0); analogWrite(PIN_B_NEG,0); digitalWrite(PIN_STBY,LOW); enabled_=false; motorVelocity_=0.0f; stepAccumulator_=0.0f; lastMotionUpdateUs_=micros(); wasMoving_=false; }
  bool isEnabled() const { return enabled_; }
  bool isMoving() const { return !isAtTarget(); }
  void stopMotion(){ motorVelocity_=0.0f; stepAccumulator_=0.0f; lastMotionUpdateUs_=micros(); wasMoving_=false; if(enabled_) applyPhase(currentPhase_,HOLD_PWM_MAX); }
  void holdCurrentPosition(){ targetPosition_=currentPosition_; stopMotion(); }
private:
  int32_t currentPosition_=0,targetPosition_=0; int16_t currentPhase_=0; float motorVelocity_=0.0f,stepAccumulator_=0.0f; unsigned long lastMotionUpdateUs_=0UL; bool enabled_=false,wasMoving_=false;
  void updateMotion(float maximumSpeed,float acceleration){
    if(!enabled_) return; const unsigned long nowUs=micros(); unsigned long elapsedUs=nowUs-lastMotionUpdateUs_; if(elapsedUs==0UL) return; lastMotionUpdateUs_=nowUs;
    float dtSec=(float)elapsedUs/1000000.0f; if(dtSec>MOTOR_MAX_UPDATE_DT_SEC) dtSec=MOTOR_MAX_UPDATE_DT_SEC;
    const int32_t error=targetPosition_-currentPosition_;
    if(error==0){ motorVelocity_=0.0f; stepAccumulator_=0.0f; if(wasMoving_) applyPhase(currentPhase_,HOLD_PWM_MAX); wasMoving_=false; return; }
    const int8_t desiredDirection=error>0?1:-1; const int8_t velocityDirection=motorVelocity_>0.0f?1:motorVelocity_<0.0f?-1:0;
    if(desiredDirection!=0 && velocityDirection!=0 && desiredDirection!=velocityDirection){ motorVelocity_=0.0f; stepAccumulator_=0.0f; }
    const float speedAbs=fabsf(motorVelocity_); const float stoppingDistance=(speedAbs*speedAbs)/(2.0f*acceleration); float accelerationCommand=0.0f;
    if(velocityDirection!=0 && velocityDirection!=desiredDirection) accelerationCommand=-(float)velocityDirection*acceleration;
    else if((float)labs(error)<=stoppingDistance+1.0f) accelerationCommand=-(float)velocityDirection*acceleration;
    else accelerationCommand=(float)desiredDirection*acceleration;
    motorVelocity_+=accelerationCommand*dtSec; motorVelocity_=constrainFloat(motorVelocity_,-maximumSpeed,maximumSpeed);
    if(desiredDirection>0 && motorVelocity_<0.0f) motorVelocity_=0.0f; if(desiredDirection<0 && motorVelocity_>0.0f) motorVelocity_=0.0f;
    stepAccumulator_+=motorVelocity_*dtSec; uint8_t emittedSteps=0;
    while(fabsf(stepAccumulator_)>=1.0f && emittedSteps<MOTOR_MAX_EMITTED_STEPS_PER_UPDATE){ const int8_t stepDirection=stepAccumulator_>0.0f?1:-1; emitLogicalSixteenthStep(stepDirection,RUN_PWM_MAX); currentPosition_+=stepDirection; stepAccumulator_-=(float)stepDirection; wasMoving_=true; emittedSteps++; if(currentPosition_==targetPosition_){ motorVelocity_=0.0f; stepAccumulator_=0.0f; break; } }
    if(currentPosition_==targetPosition_ && fabsf(stepAccumulator_)<1.0f){ motorVelocity_=0.0f; stepAccumulator_=0.0f; applyPhase(currentPhase_,HOLD_PWM_MAX); wasMoving_=false; }
  }
  void emitLogicalSixteenthStep(int8_t logicalDirection,uint8_t pwmMax){ const int8_t physicalDirection=logicalDirection*SCALE_DIRECTION; currentPhase_+=physicalDirection; while(currentPhase_<0) currentPhase_+=64; while(currentPhase_>=64) currentPhase_-=64; applyPhase(currentPhase_,pwmMax); }
  void applyPhase(int16_t phaseIndex,uint8_t pwmMax){ phaseIndex%=64; if(phaseIndex<0) phaseIndex+=64; const PhaseValue phase=PHASE_TABLE[phaseIndex]; applySignedPhase(PIN_A_POS,PIN_A_NEG,phase.phaseA,pwmMax); applySignedPhase(PIN_B_POS,PIN_B_NEG,phase.phaseB,pwmMax); }
  void applySignedPhase(uint8_t pinPositive,uint8_t pinNegative,int16_t signedAmplitude,uint8_t pwmMax){ int16_t magnitude=abs(signedAmplitude); if(magnitude>255) magnitude=255; uint16_t scaled=((uint16_t)magnitude*(uint16_t)pwmMax)/255U; if(signedAmplitude>0){ analogWrite(pinPositive,(uint8_t)scaled); analogWrite(pinNegative,0); } else if(signedAmplitude<0){ analogWrite(pinPositive,0); analogWrite(pinNegative,(uint8_t)scaled); } else { analogWrite(pinPositive,0); analogWrite(pinNegative,0); } }
};

X27SixteenthStep tachMotor;

enum class GaugeState:uint8_t{DriverOff,Starting,Running,ReturningToZero,ZeroHold};
GaugeState gaugeState=GaugeState::DriverOff; unsigned long zeroHoldStartMs=0UL; bool lastConfirmedKeyOn=false,keyOnDebounced=false,lastRawKeyOn=false; unsigned long keyStateChangedMs=0UL; const unsigned long KEY_DEBOUNCE_MS=50UL;
volatile unsigned long latestPulseIntervalUs=0UL,lastPulseUs=0UL; volatile bool newPulseAvailable=false; unsigned long lastAcceptedPulseUs=0UL;
long rawTachValue=0L,filteredDisplayValue=0L; float rawTargetStepFromDisplay=0.0f,stabilizedTargetStep=0.0f,targetStepFromDisplay=0.0f,virtualNeedleStep=0.0f,virtualNeedleVelocity=0.0f; unsigned long lastControlUpdateUs=0UL; byte engineRunConfirmCount=0; bool engineRunningConfirmed=false,displayFilterInitialized=false;

void setup(){ pinMode(PIN_KEY_ON,INPUT_PULLUP); pinMode(PIN_TACH_INPUT,INPUT); pinMode(PIN_LED,OUTPUT); digitalWrite(PIN_LED,HIGH); tachMotor.begin(); lastRawKeyOn=readKeyOnRaw(); keyOnDebounced=lastRawKeyOn; lastConfirmedKeyOn=keyOnDebounced; keyStateChangedMs=millis(); attachInterrupt(digitalPinToInterrupt(PIN_TACH_INPUT),onTachPulse,RISING); if(isKeyOnConfirmedAtStartup()) startGaugeFromDriverOff(); else { gaugeState=GaugeState::DriverOff; tachMotor.disable(); digitalWrite(PIN_LED,HIGH); } }
void loop(){ const bool keyOn=updateAndReadKeyOn(); switch(gaugeState){ case GaugeState::DriverOff: if(keyOn) startGaugeFromDriverOff(); break; case GaugeState::Starting: break; case GaugeState::Running: if(!keyOn) beginKeyOffReturn(); else { tachMotor.update(); runV1UpperControl(); } break; case GaugeState::ReturningToZero: if(keyOn) resumeRunningDuringKeyOffReturn(); else { tachMotor.update(); if(tachMotor.isAtTarget()){ zeroHoldStartMs=millis(); gaugeState=GaugeState::ZeroHold; } } break; case GaugeState::ZeroHold: if(keyOn) resumeRunningDuringKeyOffReturn(); else if((unsigned long)(millis()-zeroHoldStartMs)>=KEYOFF_ZERO_HOLD_MS){ tachMotor.disable(); digitalWrite(PIN_LED,HIGH); gaugeState=GaugeState::DriverOff; } break; } lastConfirmedKeyOn=keyOn; }

bool readKeyOnRaw(){ return digitalRead(PIN_KEY_ON)==KEY_ON_ACTIVE_LEVEL; }
bool updateAndReadKeyOn(){ const bool rawKeyOn=readKeyOnRaw(); const unsigned long nowMs=millis(); if(rawKeyOn!=lastRawKeyOn){ lastRawKeyOn=rawKeyOn; keyStateChangedMs=nowMs; } if((unsigned long)(nowMs-keyStateChangedMs)>=KEY_DEBOUNCE_MS) keyOnDebounced=rawKeyOn; return keyOnDebounced; }
bool isKeyOnConfirmedAtStartup(){ for(int i=0;i<STARTUP_KEY_CHECK_COUNT;i++){ if(!readKeyOnRaw()) return false; delay(STARTUP_KEY_CHECK_INTERVAL_MS);} return true; }
void startGaugeFromDriverOff(){ gaugeState=GaugeState::Starting; digitalWrite(PIN_LED,LOW); tachMotor.enable(); delay(STARTUP_KEY_SETTLE_DELAY_MS); forceZeroToStop(); if(ENABLE_OPENING_DEMO) openingDemo(); if(FORCE_ZERO_AFTER_OPENING) forceZeroToStop(); resetDisplayState(); gaugeState=GaugeState::Running; }
void beginKeyOffReturn(){ const float currentLogicalPosition=motorPositionToLogicalStep(tachMotor.getCurrentPosition()); resetDisplayStateAtPosition(currentLogicalPosition); tachMotor.stopMotion(); commandMotorPosition(0.0f); digitalWrite(PIN_LED,HIGH); gaugeState=GaugeState::ReturningToZero; }
void resumeRunningDuringKeyOffReturn(){ tachMotor.holdCurrentPosition(); const float currentLogicalPosition=motorPositionToLogicalStep(tachMotor.getCurrentPosition()); resetDisplayStateAtPosition(currentLogicalPosition); digitalWrite(PIN_LED,LOW); gaugeState=GaugeState::Running; }
void forceZeroToStop(){ tachMotor.zero(); }
void openingDemo(){ commandMotorPosition((float)MOTOR_STEPS); tachMotor.updateBlocking(); delay(200); commandMotorPosition(0.0f); tachMotor.updateBlocking(); delay(200); }
void resetDisplayState(){ resetDisplayStateAtPosition(0.0f); }
void resetDisplayStateAtPosition(float logicalPosition){ logicalPosition=constrainFloat(logicalPosition,0.0f,(float)MOTOR_STEPS); rawTachValue=0L; filteredDisplayValue=0L; rawTargetStepFromDisplay=logicalPosition; stabilizedTargetStep=logicalPosition; targetStepFromDisplay=logicalPosition; virtualNeedleStep=logicalPosition; virtualNeedleVelocity=0.0f; lastControlUpdateUs=micros(); engineRunConfirmCount=0; engineRunningConfirmed=false; displayFilterInitialized=false; noInterrupts(); latestPulseIntervalUs=0UL; lastPulseUs=0UL; newPulseAvailable=false; interrupts(); lastAcceptedPulseUs=0UL; commandMotorPosition(logicalPosition); }
void onTachPulse(){ const unsigned long nowUs=micros(); if(lastPulseUs!=0UL){ latestPulseIntervalUs=nowUs-lastPulseUs; newPulseAvailable=true; } lastPulseUs=nowUs; }

void runV1UpperControl(){ unsigned long pulseIntervalUs=0UL; bool pulseAvailable=false; unsigned long pulseTimestampUs=0UL; noInterrupts(); if(newPulseAvailable){ pulseIntervalUs=latestPulseIntervalUs; pulseTimestampUs=lastPulseUs; newPulseAvailable=false; pulseAvailable=true; } interrupts(); if(pulseAvailable && isValidTachInterval(pulseIntervalUs)){ lastAcceptedPulseUs=pulseTimestampUs; updateRawValueFromPulseInterval(pulseIntervalUs); const bool wasConfirmed=engineRunningConfirmed; if(updateEngineRunConfirmation(pulseIntervalUs)){ if(!wasConfirmed) initializeDisplayFilterFromRaw(); updateDisplayFilter(); updateTargetStepFromDisplay(); } } handleEngineStopZeroReturn(); updateVirtualNeedleControl(micros()); }
bool isValidTachInterval(unsigned long intervalMicros){ return intervalMicros>=MIN_VALID_INTERVAL_US && intervalMicros<=MAX_VALID_INTERVAL_US; }
bool updateEngineRunConfirmation(unsigned long intervalMicros){ if(!isValidTachInterval(intervalMicros)){ engineRunConfirmCount=0; engineRunningConfirmed=false; return false; } if(engineRunConfirmCount<ENGINE_RUN_CONFIRM_COUNT) engineRunConfirmCount++; if(engineRunConfirmCount>=ENGINE_RUN_CONFIRM_COUNT) engineRunningConfirmed=true; return engineRunningConfirmed; }
void updateRawValueFromPulseInterval(unsigned long intervalMicros){ if(intervalMicros==0UL) return; rawTachValue=STEP_CONVERSION_NUMERATOR/(long)intervalMicros; }
void initializeDisplayFilterFromRaw(){ filteredDisplayValue=rawTachValue; displayFilterInitialized=true; }
void updateDisplayFilter(){ if(!displayFilterInitialized){ initializeDisplayFilterFromRaw(); return; } filteredDisplayValue=((long)DISPLAY_FILTER_OLD*filteredDisplayValue+(long)DISPLAY_FILTER_NEW*rawTachValue)/((long)DISPLAY_FILTER_OLD+(long)DISPLAY_FILTER_NEW); }
void updateTargetStepFromDisplay(){ long stepValue=filteredDisplayValue-STEP_OFFSET; if(stepValue<0L) stepValue=0L; if(stepValue>MOTOR_STEPS) stepValue=MOTOR_STEPS; rawTargetStepFromDisplay=(float)stepValue; }
void updateVirtualNeedleControl(unsigned long nowMicros){ if(lastControlUpdateUs==0UL){ lastControlUpdateUs=nowMicros; return; } const unsigned long elapsedUs=nowMicros-lastControlUpdateUs; if(elapsedUs<CONTROL_UPDATE_INTERVAL_US) return; lastControlUpdateUs=nowMicros; float dtSec=(float)elapsedUs/1000000.0f; if(dtSec<=0.0f) return; if(dtSec>0.5f) dtSec=0.5f; const float targetDifference=rawTargetStepFromDisplay-stabilizedTargetStep; float maximumTargetChange=(targetDifference>0.0f?TARGET_STEP_MAX_UP_PER_SEC:TARGET_STEP_MAX_DOWN_PER_SEC)*dtSec; if(fabsf(targetDifference)<=maximumTargetChange) stabilizedTargetStep=rawTargetStepFromDisplay; else stabilizedTargetStep+=targetDifference>0.0f?maximumTargetChange:-maximumTargetChange; stabilizedTargetStep=constrainFloat(stabilizedTargetStep,0.0f,(float)MOTOR_STEPS); targetStepFromDisplay=stabilizedTargetStep; const float positionError=targetStepFromDisplay-virtualNeedleStep; if(fabsf(positionError)<=VIRTUAL_STOP_BAND_STEP){ virtualNeedleStep=targetStepFromDisplay; virtualNeedleVelocity=0.0f; commandMotorPosition(virtualNeedleStep); return; } const float desiredVelocity=constrainFloat(positionError/dtSec,-VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC,VIRTUAL_MAX_VEL_UP_STEP_PER_SEC); if(desiredVelocity>virtualNeedleVelocity){ virtualNeedleVelocity+=VIRTUAL_ACCEL_UP_STEP_PER_SEC2*dtSec; if(virtualNeedleVelocity>desiredVelocity) virtualNeedleVelocity=desiredVelocity; } else if(desiredVelocity<virtualNeedleVelocity){ virtualNeedleVelocity-=VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2*dtSec; if(virtualNeedleVelocity<desiredVelocity) virtualNeedleVelocity=desiredVelocity; } virtualNeedleVelocity=constrainFloat(virtualNeedleVelocity,-VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC,VIRTUAL_MAX_VEL_UP_STEP_PER_SEC); virtualNeedleStep+=virtualNeedleVelocity*dtSec; if(positionError>0.0f && virtualNeedleStep>targetStepFromDisplay){ virtualNeedleStep=targetStepFromDisplay; virtualNeedleVelocity=0.0f; } if(positionError<0.0f && virtualNeedleStep<targetStepFromDisplay){ virtualNeedleStep=targetStepFromDisplay; virtualNeedleVelocity=0.0f; } virtualNeedleStep=constrainFloat(virtualNeedleStep,0.0f,(float)MOTOR_STEPS); commandMotorPosition(virtualNeedleStep); }
void commandMotorPosition(float logicalStep){ logicalStep=constrainFloat(logicalStep,0.0f,(float)MOTOR_STEPS); tachMotor.setPosition(logicalStepToMotorPosition(logicalStep)); }
void handleEngineStopZeroReturn(){ if(lastAcceptedPulseUs==0UL) return; const unsigned long nowUs=micros(); if((unsigned long)(nowUs-lastAcceptedPulseUs)<NO_PULSE_STOP_TIMEOUT_US) return; lastAcceptedPulseUs=0UL; engineRunConfirmCount=0; engineRunningConfirmed=false; displayFilterInitialized=false; rawTachValue=0L; filteredDisplayValue=0L; rawTargetStepFromDisplay=0.0f; }
float constrainFloat(float value,float minValue,float maxValue){ if(value<minValue) return minValue; if(value>maxValue) return maxValue; return value; }
float minFloat(float a,float b){ return a<b?a:b; }
float absFloat(float value){ return value<0.0f?-value:value; }
