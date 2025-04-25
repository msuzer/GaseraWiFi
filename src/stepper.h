/*
 * Stepper Motor Controller v0.1
 * March 17, 2023
 * msuzer1@gmail.com
 *
 */
#ifndef STEPPER_MOTOR_CONTROLLER
#define STEPPER_MOTOR_CONTROLLER

#include <stdint.h>

class Stepper {
public:
  Stepper(const int en_p, const int en_n, const int dir_p, const int dir_n, const int pul_p, const int pul_n);
  void step(void);
  void move(int32_t delta);
  void gotoPosition(int32_t targetPosition);
  void gotoHomePosition(void);
  void gotoMarkPosition(void);
  int32_t getCurrentPosition(void) { return currentPosition; }
  int32_t getMarkPosition(void) { return markPosition; }
  void setMotorSpeed(int speed);
  void resetHomePosition(void) { currentPosition = 0; }
  void setMarkPosition(int32_t position) { markPosition = position; }
  void markCurrentPosition(void) { markPosition = currentPosition; }
  bool isDriverEnabled(void);
  void setDriverEnable(bool enable);
  void setDirection(bool direction);
  bool getDirection(void);
private:
  int32_t currentPosition;
  int32_t markPosition;
  int motorSpeed;
  int enPin_p;
  int enPin_n;
  int dirPin_p;
  int dirPin_n;
  int pulsePin_p;
  int pulsePin_n;
};

#endif // STEPPER_MOTOR_CONTROLLER
