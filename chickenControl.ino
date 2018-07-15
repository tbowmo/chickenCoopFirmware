/********************************************************
 *
 * Chicken coop control software
 *
 *
 *  This file contains the main initialization, and mysensors communications
 *   
 *  For the state machine, look at chickState.cpp / chickState.h 
 *  
 *  The state machine handles the opening / closing of the door, and run more or less
 *  autonomously.
 */

#define MY_RADIO_NRF24
#define MY_DEBUG
#define MY_NODE_ID 21
#define MY_REPEATER_FEATURE
#include <MySensors.h>
#include "chickState.h"
#include "portdefs.h"

// Sensor numbers
#define DOOR       1
#define PIR        2
#define GATE1      3

#define ALARM      7


// Mysensor Message objects
MyMessage msgStatus(DOOR, V_TRIPPED);

struct PortSensorMap {
  const uint8_t port;
  const uint8_t sensor;
  uint8_t last;
};

PortSensorMap pir =   { PIN_PIR,   PIR,   false };
PortSensorMap gate1 = { PIN_GATE1, GATE1, false };

bool oldDoorState = false;
bool oldAlarmState = false;

unsigned long heartbeatTimer = 0;
unsigned long lastAlarmSent = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("chickenController 1.2");

  // Input pins
  pinMode(PIN_GATE1, INPUT);
  pinMode(PIN_PIR, INPUT);
  digitalWrite(PIN_GATE1, HIGH);
  digitalWrite(PIN_PIR, HIGH);
  initSM(alarmCallback);
}

void presentation() {
  sendSketchInfo("ChickenController", "1.2");
  present(DOOR, S_DOOR, "Side door");
  present(PIR, S_MOTION, "PIR");
  present(GATE1, S_DOOR, "Front door");
  present(ALARM, S_DOOR, "ALARM");
}

void receive(const MyMessage &message) {
  Serial.print(F("Remote command : "));
  Serial.println(message.sensor);
  if (message.sensor == DOOR) {
    setDoor(message.getBool());
  }
}

void loop() {
  UpdateSM();
  bool force = false;
  if (millis() > heartbeatTimer) {
    heartbeatTimer = millis() + 3600000;
    force = true;
  }
  checkAndReportPin(&pir, force);
  checkAndReportPin(&gate1, force);
  checkDoorPosition(force);
}

// Report back open / closed state of the door.
// Only send the value, if the door is either open OR closed, if
// door is in transition, then do not report back (ie, if both detection inputs is high)
void checkDoorPosition(bool force) {
  bool doorOpen = !digitalRead(DOOR_OPEN_DETECT);
  bool doorClose = !digitalRead(DOOR_CLOSE_DETECT);
  bool doorXOR = (( doorOpen  &&  !doorClose ) || ( !doorOpen && doorClose ));
  bool doorState = false;

  if (doorOpen && !doorClose) {
     doorState = true;
  }
  
  if (force || ((doorState != oldDoorState) && doorXOR)) {
    Serial.print("door state : ");
    if (doorState) {
      Serial.println("open");
    }
    else {
      Serial.println("closed");
    }
    send(msgStatus.setSensor(DOOR).set(doorState));
    oldDoorState = doorState;
  }
}

/*
 * Helper function to send port state as sensor messages
 */
void checkAndReportPin(PortSensorMap* pin, bool force) {
  if ((digitalRead(pin->port) != pin->last) || force) {
    pin->last = digitalRead(pin->port);
    send(msgStatus.setSensor(pin->sensor).set(pin->last));
  }
}

/*
 * Callback for statemachine, indicating error state (timeout on door open / close action)
 */
void alarmCallback(bool state) {
  if (lastAlarmSent + 30000 < millis() || state != oldAlarmState) {
    if (state) {
      Serial.println("Error sent");
    } else {
      Serial.println("Error cleared");
    }
    send(msgStatus.setSensor(ALARM).set(state));
    lastAlarmSent = millis();
    oldAlarmState = state;
  }
}

