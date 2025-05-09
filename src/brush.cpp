#include "brush.h"

MotorDriver::MotorDriver(uint8_t in1, uint8_t in2, uint8_t en)
  : in1Pin(in1), in2Pin(in2), enPin(en) {}

void MotorDriver::begin() {
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enPin, OUTPUT);
  stop();
}

void MotorDriver::forward(uint8_t speed) {
  digitalWrite(in1Pin, HIGH);
  digitalWrite(in2Pin, LOW);
  analogWrite(enPin, speed);
}

void MotorDriver::reverse(uint8_t speed) {
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, HIGH);
  analogWrite(enPin, speed);
}

void MotorDriver::stop() {
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
  analogWrite(enPin, 0);
}
