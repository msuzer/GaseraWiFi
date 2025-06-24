#include "relay.h"

RelayMotorDriver::RelayMotorDriver(uint8_t forwardRelayPin, uint8_t reverseRelayPin, uint8_t limitSwitchPin)
    : forwardPin(forwardRelayPin), reversePin(reverseRelayPin), limitSwitchPin(limitSwitchPin) {}

void RelayMotorDriver::begin() {
  pinMode(forwardPin, OUTPUT);
  pinMode(reversePin, OUTPUT);
  pinMode(limitSwitchPin, INPUT_PULLUP); // Assuming limit switch is active-low
  stop();
}

void RelayMotorDriver::forward() {
  digitalWrite(forwardPin, HIGH);   // Active-High ON
  digitalWrite(reversePin, LOW);  // Active-High OFF
  moving = true;
}

void RelayMotorDriver::reverse() {
  digitalWrite(forwardPin, LOW);  // Active-High OFF
  digitalWrite(reversePin, HIGH);   // Active-High ON
  moving = true;
}

void RelayMotorDriver::stop() {
  digitalWrite(forwardPin, HIGH);  // Both OFF
  digitalWrite(reversePin, HIGH);
  moving = false;
  lastStopReason = STOPPED_BY_USER; // Reset stop reason
}

void RelayMotorDriver::moveForwardWithTimeout(uint32_t timeoutMs) {
  forward();
  startTime = millis();
  timeoutDuration = timeoutMs;
  debounceCount = 0;
}

void RelayMotorDriver::moveBackwardWithTimeout(uint32_t timeoutMs) {
  reverse();
  startTime = millis();
  timeoutDuration = timeoutMs;
  debounceCount = 0;
}

void RelayMotorDriver::update() {
  if (!moving) return;

  // Debounce limit switch (active-low)
  bool rawState = digitalRead(limitSwitchPin) == LOW;
  if (rawState == limitSwitchStableState) {
    debounceCount = 0;
  } else {
    if (++debounceCount >= DEBOUNCE_MAX) {
      limitSwitchStableState = rawState;
      debounceCount = 0;
    }
  }

  // Check stop conditions
  bool timeoutReached = (millis() - startTime >= timeoutDuration);
  bool limitTriggered = limitSwitchStableState;

  if (timeoutReached || limitTriggered) {
    stop();
    lastStopReason = limitTriggered ? STOPPED_BY_LIMIT_SWITCH : STOPPED_BY_TIMEOUT;
  }
}

bool RelayMotorDriver::isForward() const {
  return digitalRead(forwardPin) == HIGH && digitalRead(reversePin) == LOW;
}

bool RelayMotorDriver::isReverse() const {
  return digitalRead(forwardPin) == LOW && digitalRead(reversePin) == HIGH;
}

bool RelayMotorDriver::isStopped() const {
  return digitalRead(forwardPin) == digitalRead(reversePin);
}