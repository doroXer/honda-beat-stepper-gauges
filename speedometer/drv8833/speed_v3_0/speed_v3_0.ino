#include <Arduino.h>
#include <math.h>

float constrainFloat(float value,float minValue,float maxValue);
float minFloat(float a,float b);
float absFloat(float value);
bool readKeyOnRaw(); bool updateAndReadKeyOn(); bool isKeyOnConfirmedAtStartup();
void startGaugeFromDriverOff(); void beginKeyOffReturn(); void resumeRunningDuringKeyOffReturn();
void runV1UpperControl(); void updateRawSpeedValueFromPulseInterval(); void updateDisplayFilter(); void clearMovingAverageFilter(); void updateTargetStepFromDisplay(); void updateVirtualNeedleControl(unsigned long nowMicros); void commandMotorPosition(float logicalStep);
void forceZeroToStop(); void openingDemo(); void resetDisplayState(); void resetDisplayStateAtPosition(float logicalPosition); void onSpeedPulse();

/*
  Honda Beat Speedometer v3.0
  ホンダビート スピードメーター v3.0

  Public v3.0 basis / 公開版v3.0のベース:
    Private v3.0 release candidate, validated in the vehicle.

  Main changes from maintained v2.1 tuned / v2.1 Tunedからの主変更:
    Display filter        3:1 IIR @ 50 ms -> 4-pulse moving average per pulse
    Control interval      50 ms (retained)
    TARGET UP/DOWN        45/60 -> 1600/1600 logical step/s
    VIRTUAL VEL UP/DOWN   500/500 -> 425/160 logical step/s
    VIRTUAL ACC UP/DOWN   6000/6000 -> 2600/1500 logical step/s^2
    STOP BAND             3 -> 0 logical step
    MOTOR MAX SPEED       900 motor-position step/s (retained)
    MOTOR ACCEL           8000 motor-position step/s^2 (retained)

  Notes / 注記:
    - The 4-pulse moving average is updated when each valid speed pulse interval
      is consumed, not only at the 50 ms control tick.
    - The latest moving-average result is sampled into rawTarget at each 50 ms
      control update.
    - TARGET 1600/1600 keeps the TARGET layer effectively non-dominant.
    - Virtual values are aligned with tachometer v3.0 based on raw-log
      simulation and real-vehicle validation.
    - STOP BAND processing remains in code but is disabled with value 0.
    - Missing-pulse correction is intentionally NOT included.
*/

const int MOTOR_STEPS=240*3;
const uint8_t PIN_SPEED_INPUT=2,PIN_KEY_ON=3,PIN_LED=13;
const uint8_t PIN_A_NEG=5,PIN_A_POS=6,PIN_STBY=7,PIN_B_POS=9,PIN_B_NEG=10;
const uint8_t KEY_ON_ACTIVE_LEVEL=HIGH;
const int8_t SCALE_DIRECTION=-1;
const long SPEED_STEP_NUMERATOR=7480000L;
const int SPEED_STEP_OFFSET=36;
const byte DISPLAY_MOVING_AVERAGE_SAMPLES=4;
const unsigned long CONTROL_UPDATE_INTERVAL_US=50000UL;

const float VIRTUAL_MAX_VEL_UP_STEP_PER_SEC=425.0f;
const float VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC=160.0f;
const float VIRTUAL_ACCEL_UP_STEP_PER_SEC2=2600.0f;
const float VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2=1500.0f;
const float VIRTUAL_STOP_BAND_STEP=0.0f;
const float TARGET_STEP_MAX_UP_PER_SEC=1600.0f;
const float TARGET_STEP_MAX_DOWN_PER_SEC=1600.0f;

const unsigned long SPEED_PULSE_TIMEOUT_US=500000UL,MIN_VALID_INTERVAL_US=1000UL;
const bool ENABLE_OPENING_DEMO=true,FORCE_ZERO_AFTER_OPENING=true;
const int STARTUP_KEY_CHECK_COUNT=5;
const unsigned int STARTUP_KEY_CHECK_INTERVAL_MS=20,STARTUP_KEY_SETTLE_DELAY_MS=100,OPENING_HOLD_MS=200;
const int32_t MOTOR_POSITION_RATIO_NUMERATOR=32,MOTOR_POSITION_RATIO_DENOMINATOR=3;
const int32_t MOTOR_MAX_MICROSTEPS=((int32_t)MOTOR_STEPS*MOTOR_POSITION_RATIO_NUMERATOR)/MOTOR_POSITION_RATIO_DENOMINATOR;

int32_t logicalStepToMotorPosition(float logicalStep){ logicalStep=constrainFloat(logicalStep,0.0f,(float)MOTOR_STEPS); return lroundf(logicalStep*(float)MOTOR_POSITION_RATIO_NUMERATOR/(float)MOTOR_POSITION_RATIO_DENOMINATOR); }
float motorPositionToLogicalStep(int32_t motorPosition){ motorPosition=constrain(motorPosition,(int32_t)0,MOTOR_MAX_MICROSTEPS); return (float)motorPosition*(float)MOTOR_POSITION_RATIO_DENOMINATOR/(float)MOTOR_POSITION_RATIO_NUMERATOR; }

const uint8_t RUN_PWM_MAX=255,HOLD_PWM_MAX=150,ZERO_PWM_MAX=200;
const float MOTOR_MAX_SPEED_MICROSTEP_PER_SEC=900.0f;
const float MOTOR_ACCEL_MICROSTEP_PER_SEC2=8000.0f;
const float BLOCKING_MAX_SPEED_MICROSTEP_PER_SEC=11200.0f;
const float BLOCKING_ACCEL_MICROSTEP_PER_SEC2=40000.0f;
const float MOTOR_MAX_UPDATE_DT_SEC=0.020f;
const uint8_t MOTOR_MAX_EMITTED_STEPS_PER_UPDATE=32;
const int32_t ZERO_TRAVEL_MARGIN_MICROSTEPS=768,ZERO_TRAVEL_MICROSTEPS=MOTOR_MAX_MICROSTEPS+ZERO_TRAVEL_MARGIN_MICROSTEPS;
const uint16_t ZERO_STEP_INTERVAL_US=225;
const uint8_t PHASE_COUNT=64;
const int32_t ZERO_TRAVEL_MICROSTEPS_ALIGNED=((ZERO_TRAVEL_MICROSTEPS+(int32_t)PHASE_COUNT-1L)/(int32_t)PHASE_COUNT)*(int32_t)PHASE_COUNT;
const unsigned long KEYOFF_ZERO_HOLD_MS=200UL;

struct PhaseValue{int16_t phaseA;int16_t phaseB;};
const PhaseValue PHASE_TABLE[PHASE_COUNT]={
{0,255},{25,254},{50,250},{74,244},{98,236},{120,225},{142,212},{162,197},{181,181},{197,162},{212,142},{225,120},{236,98},{244,74},{250,50},{254,25},
{255,0},{254,-25},{250,-50},{244,-74},{236,-98},{225,-120},{212,-142},{197,-162},{181,-181},{162,-197},{142,-212},{120,-225},{98,-236},{74,-244},{50,-250},{25,-254},
{0,-255},{-25,-254},{-50,-250},{-74,-244},{-98,-236},{-120,-225},{-142,-212},{-162,-197},{-181,-181},{-197,-162},{-212,-142},{-225,-120},{-236,-98},{-244,-74},{-250,-50},{-254,-25},
{-255,0},{-254,25},{-250,50},{-244,74},{-236,98},{-225,120},{-212,142},{-197,162},{-181,181},{-162,197},{-142,212},{-120,225},{-98,236},{-74,244},{-50,250},{-25,254}};

class X27SixteenthStep{
public:
 void begin(){ pinMode(PIN_A_NEG,OUTPUT);pinMode(PIN_A_POS,OUTPUT);pinMode(PIN_STBY,OUTPUT);pinMode(PIN_B_POS,OUTPUT);pinMode(PIN_B_NEG,OUTPUT);digitalWrite(PIN_STBY,LOW);analogWrite(PIN_A_NEG,0);analogWrite(PIN_A_POS,0);analogWrite(PIN_B_POS,0);analogWrite(PIN_B_NEG,0);configureTimer1Pwm();currentPosition_=targetPosition_=0;currentPhase_=0;motorVelocity_=stepAccumulator_=0.0f;lastMotionUpdateUs_=micros();enabled_=wasMoving_=false;}
 void setPosition(int32_t position){targetPosition_=constrain(position,(int32_t)0,MOTOR_MAX_MICROSTEPS);} 
 void update(){updateMotion(MOTOR_MAX_SPEED_MICROSTEP_PER_SEC,MOTOR_ACCEL_MICROSTEP_PER_SEC2);} 
 void updateBlocking(){if(!enabled_)enable();while(!isAtTarget()){updateMotion(BLOCKING_MAX_SPEED_MICROSTEP_PER_SEC,BLOCKING_ACCEL_MICROSTEP_PER_SEC2);delayMicroseconds(50);}motorVelocity_=stepAccumulator_=0.0f;applyPhase(currentPhase_,HOLD_PWM_MAX);} 
 void zero(){if(!enabled_)enable();motorVelocity_=stepAccumulator_=0.0f;wasMoving_=false;for(int32_t i=0;i<ZERO_TRAVEL_MICROSTEPS_ALIGNED;i++){emitLogicalMicrostep(-1,ZERO_PWM_MAX);delayMicroseconds(ZERO_STEP_INTERVAL_US);}currentPosition_=targetPosition_=0;motorVelocity_=stepAccumulator_=0.0f;lastMotionUpdateUs_=micros();wasMoving_=false;applyPhase(currentPhase_,HOLD_PWM_MAX);} 
 void enable(){if(enabled_)return;digitalWrite(PIN_STBY,HIGH);enabled_=true;lastMotionUpdateUs_=micros();applyPhase(currentPhase_,HOLD_PWM_MAX);} 
 void disable(){analogWrite(PIN_A_NEG,0);analogWrite(PIN_A_POS,0);analogWrite(PIN_B_POS,0);analogWrite(PIN_B_NEG,0);digitalWrite(PIN_STBY,LOW);motorVelocity_=stepAccumulator_=0.0f;enabled_=wasMoving_=false;} 
 bool isEnabled()const{return enabled_;} bool isAtTarget()const{return currentPosition_==targetPosition_&&fabsf(motorVelocity_)<0.5f;} int32_t getCurrentPosition()const{return currentPosition_;} int32_t getTargetPosition()const{return targetPosition_;}
 void stopMotion(){motorVelocity_=stepAccumulator_=0.0f;lastMotionUpdateUs_=micros();wasMoving_=false;if(enabled_)applyPhase(currentPhase_,HOLD_PWM_MAX);} void holdCurrentPosition(){targetPosition_=currentPosition_;stopMotion();}
private:
 int32_t currentPosition_=0,targetPosition_=0; int8_t currentPhase_=0; float motorVelocity_=0.0f,stepAccumulator_=0.0f; uint32_t lastMotionUpdateUs_=0; bool enabled_=false,wasMoving_=false;
 void configureTimer1Pwm(){TCCR1A=_BV(WGM10);TCCR1B=_BV(WGM12)|_BV(CS11)|_BV(CS10);} 
 void updateMotion(float maximumSpeed,float acceleration){if(!enabled_)return;const uint32_t nowUs=micros();float dtSec=(float)(uint32_t)(nowUs-lastMotionUpdateUs_)/1000000.0f;lastMotionUpdateUs_=nowUs;if(dtSec<=0.0f)return;if(dtSec>MOTOR_MAX_UPDATE_DT_SEC)dtSec=MOTOR_MAX_UPDATE_DT_SEC;const int32_t error=targetPosition_-currentPosition_;if(error==0&&fabsf(motorVelocity_)<0.5f){motorVelocity_=stepAccumulator_=0.0f;if(wasMoving_){applyPhase(currentPhase_,HOLD_PWM_MAX);wasMoving_=false;}return;}const int8_t desiredDirection=error>0?1:error<0?-1:0;const int8_t velocityDirection=motorVelocity_>0.0f?1:motorVelocity_<0.0f?-1:0;if(desiredDirection!=0&&velocityDirection!=0&&desiredDirection!=velocityDirection){motorVelocity_=stepAccumulator_=0.0f;}const float speedAbs=fabsf(motorVelocity_);const float stoppingDistance=(speedAbs*speedAbs)/(2.0f*acceleration);float accelerationCommand=0.0f;if(desiredDirection==0)accelerationCommand=-(float)velocityDirection*acceleration;else if(velocityDirection!=0&&velocityDirection!=desiredDirection)accelerationCommand=(float)desiredDirection*acceleration;else if((float)labs(error)<=stoppingDistance+1.0f)accelerationCommand=-(float)velocityDirection*acceleration;else accelerationCommand=(float)desiredDirection*acceleration;motorVelocity_+=accelerationCommand*dtSec;motorVelocity_=constrainFloat(motorVelocity_,-maximumSpeed,maximumSpeed);if(velocityDirection>0&&motorVelocity_<0.0f&&desiredDirection>=0)motorVelocity_=0.0f;if(velocityDirection<0&&motorVelocity_>0.0f&&desiredDirection<=0)motorVelocity_=0.0f;stepAccumulator_+=motorVelocity_*dtSec;uint8_t emittedSteps=0;while(fabsf(stepAccumulator_)>=1.0f&&emittedSteps<MOTOR_MAX_EMITTED_STEPS_PER_UPDATE){const int8_t stepDirection=stepAccumulator_>0.0f?1:-1;emitLogicalMicrostep(stepDirection,RUN_PWM_MAX);currentPosition_+=stepDirection;stepAccumulator_-=(float)stepDirection;wasMoving_=true;emittedSteps++;if(currentPosition_==targetPosition_){motorVelocity_=stepAccumulator_=0.0f;break;}}if(currentPosition_==targetPosition_&&labs(error)<=1){motorVelocity_=stepAccumulator_=0.0f;}}
 void drivePhasePair(uint8_t positivePin,uint8_t negativePin,int16_t command,uint8_t pwmMaximum){const uint8_t pwmValue=(uint8_t)(((long)abs(command)*(long)pwmMaximum)/255L);if(command>0){analogWrite(positivePin,pwmValue);analogWrite(negativePin,0);}else if(command<0){analogWrite(positivePin,0);analogWrite(negativePin,pwmValue);}else{analogWrite(positivePin,0);analogWrite(negativePin,0);}}
 void applyPhase(int8_t phaseIndex,uint8_t pwmMaximum){phaseIndex%=(int8_t)PHASE_COUNT;if(phaseIndex<0)phaseIndex+=(int8_t)PHASE_COUNT;const PhaseValue &phase=PHASE_TABLE[phaseIndex];drivePhasePair(PIN_A_POS,PIN_A_NEG,phase.phaseA,pwmMaximum);drivePhasePair(PIN_B_POS,PIN_B_NEG,phase.phaseB,pwmMaximum);} 
 void emitLogicalMicrostep(int8_t logicalDirection,uint8_t pwmMaximum){currentPhase_+=logicalDirection*SCALE_DIRECTION;if(currentPhase_>=(int8_t)PHASE_COUNT)currentPhase_=0;else if(currentPhase_<0)currentPhase_=(int8_t)PHASE_COUNT-1;applyPhase(currentPhase_,pwmMaximum);} };

X27SixteenthStep speedMotor;
volatile unsigned long lastPulseMicros=0,lastPulseSeenMicros=0,latestPulseIntervalMicros=0,latestRawPulseIntervalMicros=0;volatile bool pulseIntervalUpdated=false;volatile unsigned long speedPulseCountTotal=0,speedPulseCountValid=0,speedPulseCountRejectedShort=0;
unsigned long rawSpeedValue=0;
float filteredDisplayValue=0.0f;
unsigned long displayMovingAverageBuffer[DISPLAY_MOVING_AVERAGE_SAMPLES]={0UL,0UL,0UL,0UL};
unsigned long displayMovingAverageSum=0UL;
byte displayMovingAverageIndex=0;
byte displayMovingAverageCount=0;
float rawTargetStepFromDisplay=0.0f,stabilizedTargetStep=0.0f,targetStepFromDisplay=0.0f;
bool targetSlewInitialized=false;
float virtualNeedleStep=0.0f,virtualNeedleVelocity=0.0f;
int32_t lastCommandedMotorPosition=-1;
unsigned long lastControlUpdateMicros=0;
bool speedTimeoutZeroActive=false;
enum class GaugeState:uint8_t{DriverOff,Running,ReturningToZero,HoldingAtZero};GaugeState gaugeState=GaugeState::DriverOff;unsigned long zeroHoldStartMs=0;const unsigned long KEY_DEBOUNCE_MS=20UL;bool keyRawState=false,keyStableState=false;unsigned long keyLastChangeMs=0;

void setup(){pinMode(PIN_KEY_ON,INPUT);pinMode(PIN_SPEED_INPUT,INPUT);pinMode(PIN_LED,OUTPUT);digitalWrite(PIN_LED,HIGH);speedMotor.begin();keyRawState=readKeyOnRaw();keyStableState=keyRawState;keyLastChangeMs=millis();if(isKeyOnConfirmedAtStartup())startGaugeFromDriverOff();attachInterrupt(digitalPinToInterrupt(PIN_SPEED_INPUT),onSpeedPulse,RISING);} 
void loop(){speedMotor.update();const bool keyOn=updateAndReadKeyOn();switch(gaugeState){case GaugeState::DriverOff:if(keyOn)startGaugeFromDriverOff();return;case GaugeState::Running:if(!keyOn){beginKeyOffReturn();return;}runV1UpperControl();return;case GaugeState::ReturningToZero:if(keyOn){resumeRunningDuringKeyOffReturn();return;}if(speedMotor.isAtTarget()){zeroHoldStartMs=millis();gaugeState=GaugeState::HoldingAtZero;}return;case GaugeState::HoldingAtZero:if(keyOn){resumeRunningDuringKeyOffReturn();return;}if((unsigned long)(millis()-zeroHoldStartMs)>=KEYOFF_ZERO_HOLD_MS){speedMotor.disable();digitalWrite(PIN_LED,HIGH);gaugeState=GaugeState::DriverOff;}return;}}
bool readKeyOnRaw(){return digitalRead(PIN_KEY_ON)==KEY_ON_ACTIVE_LEVEL;}bool updateAndReadKeyOn(){const bool raw=readKeyOnRaw();const unsigned long nowMs=millis();if(raw!=keyRawState){keyRawState=raw;keyLastChangeMs=nowMs;}if((unsigned long)(nowMs-keyLastChangeMs)>=KEY_DEBOUNCE_MS)keyStableState=keyRawState;return keyStableState;}bool isKeyOnConfirmedAtStartup(){for(int i=0;i<STARTUP_KEY_CHECK_COUNT;i++){if(!readKeyOnRaw())return false;delay(STARTUP_KEY_CHECK_INTERVAL_MS);}return true;}
void startGaugeFromDriverOff(){speedMotor.enable();digitalWrite(PIN_LED,LOW);delay(STARTUP_KEY_SETTLE_DELAY_MS);forceZeroToStop();resetDisplayState();if(ENABLE_OPENING_DEMO)openingDemo();if(FORCE_ZERO_AFTER_OPENING)forceZeroToStop();resetDisplayState();gaugeState=GaugeState::Running;}void beginKeyOffReturn(){const float currentLogicalPosition=motorPositionToLogicalStep(speedMotor.getCurrentPosition());resetDisplayStateAtPosition(currentLogicalPosition);speedMotor.stopMotion();commandMotorPosition(0.0f);digitalWrite(PIN_LED,HIGH);gaugeState=GaugeState::ReturningToZero;}void resumeRunningDuringKeyOffReturn(){speedMotor.holdCurrentPosition();const float currentLogicalPosition=motorPositionToLogicalStep(speedMotor.getCurrentPosition());resetDisplayStateAtPosition(currentLogicalPosition);digitalWrite(PIN_LED,LOW);gaugeState=GaugeState::Running;}
void runV1UpperControl(){updateRawSpeedValueFromPulseInterval();const unsigned long nowMicros=micros();if((unsigned long)(nowMicros-lastControlUpdateMicros)>=CONTROL_UPDATE_INTERVAL_US){updateTargetStepFromDisplay();updateVirtualNeedleControl(nowMicros);lastControlUpdateMicros=nowMicros;}}
void updateRawSpeedValueFromPulseInterval(){const unsigned long nowMicros=micros();unsigned long intervalCopy=0,lastSeenCopy=0;bool hasUpdatedInterval=false;noInterrupts();intervalCopy=latestPulseIntervalMicros;lastSeenCopy=lastPulseSeenMicros;hasUpdatedInterval=pulseIntervalUpdated;pulseIntervalUpdated=false;interrupts();unsigned long ageMicros=0;if(lastSeenCopy==0)ageMicros=0;else if(nowMicros>=lastSeenCopy)ageMicros=nowMicros-lastSeenCopy;else ageMicros=0;const bool timeoutNow=(lastSeenCopy==0||ageMicros>SPEED_PULSE_TIMEOUT_US);if(timeoutNow){rawSpeedValue=0;filteredDisplayValue=0.0f;clearMovingAverageFilter();speedTimeoutZeroActive=true;return;}if(hasUpdatedInterval&&intervalCopy>=MIN_VALID_INTERVAL_US){const unsigned long value=SPEED_STEP_NUMERATOR/intervalCopy;rawSpeedValue=constrain(value,0UL,(unsigned long)(MOTOR_STEPS+SPEED_STEP_OFFSET));speedTimeoutZeroActive=false;updateDisplayFilter();}}
void clearMovingAverageFilter(){for(byte i=0;i<DISPLAY_MOVING_AVERAGE_SAMPLES;i++)displayMovingAverageBuffer[i]=0UL;displayMovingAverageSum=0UL;displayMovingAverageIndex=0;displayMovingAverageCount=0;}
void updateDisplayFilter(){if(displayMovingAverageCount<DISPLAY_MOVING_AVERAGE_SAMPLES){displayMovingAverageBuffer[displayMovingAverageIndex]=rawSpeedValue;displayMovingAverageSum+=rawSpeedValue;displayMovingAverageCount++;displayMovingAverageIndex++;if(displayMovingAverageIndex>=DISPLAY_MOVING_AVERAGE_SAMPLES)displayMovingAverageIndex=0;filteredDisplayValue=(float)displayMovingAverageSum/(float)displayMovingAverageCount;return;}displayMovingAverageSum-=displayMovingAverageBuffer[displayMovingAverageIndex];displayMovingAverageBuffer[displayMovingAverageIndex]=rawSpeedValue;displayMovingAverageSum+=rawSpeedValue;displayMovingAverageIndex++;if(displayMovingAverageIndex>=DISPLAY_MOVING_AVERAGE_SAMPLES)displayMovingAverageIndex=0;filteredDisplayValue=(float)displayMovingAverageSum/(float)DISPLAY_MOVING_AVERAGE_SAMPLES;}
void updateTargetStepFromDisplay(){const float calcStep=filteredDisplayValue-(float)SPEED_STEP_OFFSET;rawTargetStepFromDisplay=constrainFloat(calcStep,0.0f,(float)MOTOR_STEPS);if(!targetSlewInitialized){stabilizedTargetStep=rawTargetStepFromDisplay;targetSlewInitialized=true;}else{const float dtSec=CONTROL_UPDATE_INTERVAL_US/1000000.0f;const float diff=rawTargetStepFromDisplay-stabilizedTargetStep;if(diff>0.0f){const float maxUp=TARGET_STEP_MAX_UP_PER_SEC*dtSec;stabilizedTargetStep+=minFloat(diff,maxUp);}else if(diff<0.0f){const float maxDown=TARGET_STEP_MAX_DOWN_PER_SEC*dtSec;stabilizedTargetStep-=minFloat(-diff,maxDown);}}stabilizedTargetStep=constrainFloat(stabilizedTargetStep,0.0f,(float)MOTOR_STEPS);targetStepFromDisplay=stabilizedTargetStep;}
void updateVirtualNeedleControl(unsigned long nowMicros){float dtSec=(nowMicros-lastControlUpdateMicros)/1000000.0f;if(dtSec<=0.0f||dtSec>0.5f)dtSec=CONTROL_UPDATE_INTERVAL_US/1000000.0f;const float diff=targetStepFromDisplay-virtualNeedleStep;if(absFloat(diff)<=VIRTUAL_STOP_BAND_STEP&&absFloat(virtualNeedleVelocity)<1.0f){virtualNeedleStep=targetStepFromDisplay;virtualNeedleVelocity=0.0f;commandMotorPosition(virtualNeedleStep);return;}float desiredVelocity=diff/dtSec;desiredVelocity=constrainFloat(desiredVelocity,-VIRTUAL_MAX_VEL_DOWN_STEP_PER_SEC,VIRTUAL_MAX_VEL_UP_STEP_PER_SEC);float maxVelocityChange;if(desiredVelocity>virtualNeedleVelocity){maxVelocityChange=VIRTUAL_ACCEL_UP_STEP_PER_SEC2*dtSec;virtualNeedleVelocity+=minFloat(maxVelocityChange,desiredVelocity-virtualNeedleVelocity);}else{maxVelocityChange=VIRTUAL_ACCEL_DOWN_STEP_PER_SEC2*dtSec;virtualNeedleVelocity-=minFloat(maxVelocityChange,virtualNeedleVelocity-desiredVelocity);}float nextStep=virtualNeedleStep+virtualNeedleVelocity*dtSec;if((diff>0.0f&&nextStep>targetStepFromDisplay)||(diff<0.0f&&nextStep<targetStepFromDisplay)){nextStep=targetStepFromDisplay;virtualNeedleVelocity=0.0f;}virtualNeedleStep=constrainFloat(nextStep,0.0f,(float)MOTOR_STEPS);commandMotorPosition(virtualNeedleStep);}
void commandMotorPosition(float logicalStep){const int32_t motorPosition=logicalStepToMotorPosition(logicalStep);if(motorPosition!=lastCommandedMotorPosition){speedMotor.setPosition(motorPosition);lastCommandedMotorPosition=motorPosition;}}
void forceZeroToStop(){speedMotor.zero();speedMotor.setPosition(0);lastCommandedMotorPosition=0;}void openingDemo(){commandMotorPosition((float)MOTOR_STEPS);speedMotor.updateBlocking();delay(OPENING_HOLD_MS);commandMotorPosition(0.0f);speedMotor.updateBlocking();delay(OPENING_HOLD_MS);}void resetDisplayState(){rawSpeedValue=0;filteredDisplayValue=0.0f;clearMovingAverageFilter();rawTargetStepFromDisplay=0.0f;stabilizedTargetStep=0.0f;targetStepFromDisplay=0.0f;targetSlewInitialized=false;virtualNeedleStep=0.0f;virtualNeedleVelocity=0.0f;lastCommandedMotorPosition=0;lastControlUpdateMicros=micros();speedTimeoutZeroActive=false;noInterrupts();lastPulseMicros=0;lastPulseSeenMicros=0;latestPulseIntervalMicros=0;latestRawPulseIntervalMicros=0;pulseIntervalUpdated=false;interrupts();}
void resetDisplayStateAtPosition(float logicalPosition){logicalPosition=constrainFloat(logicalPosition,0.0f,(float)MOTOR_STEPS);rawSpeedValue=0;filteredDisplayValue=0.0f;clearMovingAverageFilter();rawTargetStepFromDisplay=logicalPosition;stabilizedTargetStep=logicalPosition;targetStepFromDisplay=logicalPosition;targetSlewInitialized=true;virtualNeedleStep=logicalPosition;virtualNeedleVelocity=0.0f;lastCommandedMotorPosition=logicalStepToMotorPosition(logicalPosition);lastControlUpdateMicros=micros();speedTimeoutZeroActive=false;noInterrupts();lastPulseMicros=0;lastPulseSeenMicros=0;latestPulseIntervalMicros=0;latestRawPulseIntervalMicros=0;pulseIntervalUpdated=false;interrupts();}
void onSpeedPulse(){const unsigned long nowMicros=micros();speedPulseCountTotal++;if(lastPulseMicros!=0){const unsigned long interval=nowMicros-lastPulseMicros;latestRawPulseIntervalMicros=interval;if(interval>=MIN_VALID_INTERVAL_US){latestPulseIntervalMicros=interval;pulseIntervalUpdated=true;speedPulseCountValid++;}else speedPulseCountRejectedShort++;}lastPulseMicros=nowMicros;lastPulseSeenMicros=nowMicros;}
float constrainFloat(float value,float minValue,float maxValue){if(value<minValue)return minValue;if(value>maxValue)return maxValue;return value;}float minFloat(float a,float b){return a<b?a:b;}float absFloat(float value){return value<0.0f?-value:value;}
