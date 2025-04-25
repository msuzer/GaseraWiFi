#include "stepper.h"
#include "sys_config.h"
#include <Arduino.h>

Stepper::Stepper(const int en_p, const int en_n, const int dir_p, const int dir_n, const int pul_p, const int pul_n) {
  this->enPin_p = en_p;
  this->enPin_n = en_n;
  this->dirPin_p = dir_p;
  this->dirPin_n = dir_n;
  this->pulsePin_p = pul_p;
  this->pulsePin_n = pul_n;
  currentPosition = 0;
  markPosition = STEPPER_MARK_POSITION;
  motorSpeed = STEPPER_DEFAULT_SPEED;

  pinMode(this->enPin_p, OUTPUT);
  pinMode(this->enPin_n, OUTPUT);
  pinMode(this->dirPin_p, OUTPUT);
  pinMode(this->dirPin_n, OUTPUT);
  pinMode(this->pulsePin_p, OUTPUT);
  pinMode(this->pulsePin_n, OUTPUT);

  digitalWrite(this->enPin_p, HIGH);
  digitalWrite(this->enPin_n, LOW);
  digitalWrite(this->dirPin_p, LOW);
  digitalWrite(this->dirPin_n, HIGH);
  digitalWrite(this->pulsePin_p, LOW);
  digitalWrite(this->pulsePin_n, HIGH);
}

void Stepper::setMotorSpeed(int speed) {
  if (speed > 100 && speed < 10000) {
    this->motorSpeed = speed;
  }
}

void Stepper::gotoHomePosition(void) {
  gotoPosition(0);
}

void Stepper::gotoMarkPosition(void) {
  gotoPosition(markPosition);
}

void Stepper::gotoPosition(int32_t targetPosition) {
  move(targetPosition - currentPosition);
}

void Stepper::step(void) {
  digitalWrite(this->pulsePin_p, HIGH);
  digitalWrite(this->pulsePin_n, LOW);
  delayMicroseconds(motorSpeed);
  digitalWrite(this->pulsePin_p, LOW);
  digitalWrite(this->pulsePin_n, HIGH);
  delayMicroseconds(motorSpeed);
  currentPosition += getDirection() ? -1 : 1;
}

void Stepper::move(int32_t delta) {
  bool driverEnabled;

  currentPosition += delta;

  if (delta == 0) {
    return;
  } else if (delta < 0) {
    delta = -delta;
    setDirection(true);
  } else {
    setDirection(false);
  }

  driverEnabled = isDriverEnabled();
  setDriverEnable(true);
  int i = 0;
  while (delta > 0) {
    digitalWrite(this->pulsePin_p, HIGH);
    digitalWrite(this->pulsePin_n, LOW);
    delayMicroseconds(motorSpeed);
    digitalWrite(this->pulsePin_p, LOW);
    digitalWrite(this->pulsePin_n, HIGH);
    delayMicroseconds(motorSpeed);
    ++i;
    --delta;    
    if (delta < 1000) {
      motorSpeed = STEPPER_DEFAULT_SPEED;
    } else if (i > 1000) {
      motorSpeed = STEPPER_FAST_SPEED;
    }
  }
  setDriverEnable(driverEnabled);
}

bool Stepper::isDriverEnabled(void) {
  return !(digitalRead(this->enPin_p) == HIGH && digitalRead(this->enPin_n) == LOW);
}

bool Stepper::getDirection(void) {
  return (digitalRead(this->dirPin_p) == HIGH && digitalRead(this->dirPin_n) == LOW);
}

void Stepper::setDriverEnable(bool enable) {
  if (enable) {
    digitalWrite(this->enPin_p, LOW);   //ENA+(+5V) low=enabled
    digitalWrite(this->enPin_n, HIGH);  //ENA-(ENA)
  } else {
    digitalWrite(this->enPin_p, HIGH);  //ENA+(+5V) high=disabled
    digitalWrite(this->enPin_n, LOW);   //ENA-(ENA)
  }
}

void Stepper::setDirection(bool dir) {
  if (dir) {
    digitalWrite(this->dirPin_p, HIGH);
    digitalWrite(this->dirPin_n, LOW);
  } else {
    digitalWrite(this->dirPin_p, LOW);
    digitalWrite(this->dirPin_n, HIGH);
  }
}
