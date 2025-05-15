#include "relay.h"

RelayMotorDriver::RelayMotorDriver(uint8_t forwardRelayPin, uint8_t reverseRelayPin)
  : forwardPin(forwardRelayPin), reversePin(reverseRelayPin) {}

void RelayMotorDriver::begin() {
  pinMode(forwardPin, OUTPUT);
  pinMode(reversePin, OUTPUT);
  stop();
}

void RelayMotorDriver::forward() {
  digitalWrite(forwardPin, LOW);   // Active-Low ON
  digitalWrite(reversePin, HIGH);  // Active-Low OFF
}

void RelayMotorDriver::reverse() {
  digitalWrite(forwardPin, HIGH);  // Active-Low OFF
  digitalWrite(reversePin, LOW);   // Active-Low ON
}

void RelayMotorDriver::stop() {
  digitalWrite(forwardPin, HIGH);  // Both OFF
  digitalWrite(reversePin, HIGH);
}
