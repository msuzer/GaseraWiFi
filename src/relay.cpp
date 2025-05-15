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

bool RelayMotorDriver::isForward() const {
  return digitalRead(forwardPin) == LOW && digitalRead(reversePin) == HIGH;
}

bool RelayMotorDriver::isReverse() const {
  return digitalRead(forwardPin) == HIGH && digitalRead(reversePin) == LOW;
}

bool RelayMotorDriver::isStopped() const {
  return digitalRead(forwardPin) == HIGH && digitalRead(reversePin) == HIGH;
}