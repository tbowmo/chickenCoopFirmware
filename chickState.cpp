#include <Arduino.h>
#include "chickState.h"

static chickenSM _chickenSM;

void (*_sendAlarmCallback)(bool);

bool doorOpen();
bool doorClosed();



void SwitchSM(StateDefinition& newState) {
  Serial.print("old state ");
  Serial.print(_chickenSM.currentState->name);
  Serial.print(" time in state ");
  Serial.println(TimeInState());
  Serial.print("New state ");
  Serial.println(newState.name);

  // Change state if needed
  if (_chickenSM.currentState != &newState) _chickenSM.currentState = &newState;
  // Transition event
  if (_chickenSM.currentState->Transition) _chickenSM.currentState->Transition();
  // save time
  _chickenSM.stateEnter = millis();
}

bool CurrentStateIs(StateDefinition& state) {
  return _chickenSM.currentState ==  &state;
}

uint32_t TimeInState() {
  return millis() - _chickenSM.stateEnter;
}

void UpdateSM() {
  if (_chickenSM.currentState->Update) _chickenSM.currentState->Update();
}

/**********************************/
static StateDefinition sdOpen        = { StateOpenDoorTransition, StateOpenDoor, "Opening door"};
static StateDefinition sdClose       = { StateCloseDoorTransition, StateCloseDoor, "Close door"};
static StateDefinition sdStandby     = { StateStandbyTransition, StateStandby, "Standby"};
static StateDefinition sdError       = { StateErrorTransition, StateError, "Error"};
/**********************************/

/********* States *************/
void StateOpenDoorTransition() {
  if (doorOpen()) {
    SwitchSM(sdStandby);
  } else {
    setMotorState(UP);
  }
}

void StateOpenDoor() {
  if (doorOpen()) {
    SwitchSM(sdStandby);
  }

  if (TimeInState() > TimeBeforeFailure) {
    SwitchSM(sdError);
  }
}

void StateCloseDoorTransition() {
  if (doorClosed()) {
    delay(200);
    SwitchSM(sdStandby);
  } else {
    setMotorState(DOWN);
  }
}

void StateCloseDoor() {
  if (doorClosed()) {
    delay(200);
    SwitchSM(sdStandby);
  }
  if (TimeInState() > TimeBeforeFailure) {
    SwitchSM(sdError);
  }
}

void StateErrorTransition() {
  setMotorState(OFF);
}

void StateError() {
  if (doorOpen() && doorClosed()) {
    SwitchSM(sdStandby);
  }
  _sendAlarmCallback(true);
}

void StateStandbyTransition() {
  _sendAlarmCallback(false);
  setMotorState(OFF);
}

void StateStandby() {
  // do NOTHING!
}


/** support functions **/

void setMotorState(doorActivity action) {
  switch (action) {
    case UP:
      digitalWrite(DOOR_DIRECTION, DIR_UP);
      digitalWrite(DOOR_ACTIVATE, HIGH);
      break;
    case DOWN:
      digitalWrite(DOOR_DIRECTION, DIR_DOWN);
      digitalWrite(DOOR_ACTIVATE, HIGH);
      break;
    case OFF:
      digitalWrite(DOOR_ACTIVATE, LOW);
      break;
  }
}

bool doorOpen() {
  return !digitalRead(DOOR_OPEN_DETECT) && digitalRead(DOOR_CLOSE_DETECT);
}

bool doorClosed() {
  return !digitalRead(DOOR_CLOSE_DETECT) && digitalRead(DOOR_OPEN_DETECT);
}

/**
 * "public" methods
 **/

void setDoor(bool state) { // true = open, false = closed
  if (state)
    SwitchSM(sdOpen);
  else
    SwitchSM(sdClose);
}


void initSM(void(*sendCallback)(bool)) {
  pinMode(DOOR_DIRECTION, OUTPUT);
  pinMode(DOOR_ACTIVATE, OUTPUT);
  pinMode(DOOR_OPEN_DETECT, INPUT);
  pinMode(DOOR_CLOSE_DETECT, INPUT);
  digitalWrite(DOOR_OPEN_DETECT, HIGH);
  digitalWrite(DOOR_CLOSE_DETECT, HIGH);
  _sendAlarmCallback = sendCallback;
   SwitchSM(sdOpen);
}

