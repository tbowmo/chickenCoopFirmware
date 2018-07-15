#ifndef stateMachine_h
#define stateMachine_h
#include "portdefs.h"

       
#define DIR_UP HIGH
#define DIR_DOWN LOW

struct StateDefinition {
  void(*Transition)();
  void(*Update)();
  const char* name;
};

enum doorActivity {
  UP,
  DOWN,
  OFF
};

// definition of the heat state machine : state & properties
typedef struct {
  StateDefinition* currentState;
  uint32_t stateEnter;
} chickenSM;

void initSM(void(*alarmCallback)(bool));

void SwitchSM(StateDefinition& newState);      // Change the state in the machine
void UpdateSM();                         // Update the state machine (transition once, then update) etc.
uint32_t TimeInState();                  // Time elapsed in state (in seconds!)
bool CurrentStateIs(StateDefinition& state);
void setDoor(bool);
void setMotorState(doorActivity action);


/********************************************************************
 * states / state transitions below here, should not be called outside
 * the statemachine.
 */
void StateOpenDoorTransition();
void StateOpenDoor();
void StateCloseDoorTransition();
void StateCloseDoor();
void StateStandbyTransition();
void StateStandby();
void StateErrorTransition();
void StateError();

static const uint32_t TimeBeforeFailure = 10000;
#endif
