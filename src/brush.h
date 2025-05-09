#ifndef BRUSH_MOTOR_H
#define BRUSH_MOTOR_H

#include <Arduino.h>
#include <stdint.h> // for int32_t

class MotorDriver {
  private:
    uint8_t in1Pin;
    uint8_t in2Pin;
    uint8_t enPin;

  public:
    MotorDriver(uint8_t in1, uint8_t in2, uint8_t en);
    void begin();
    void forward(uint8_t speed = 255);
    void reverse(uint8_t speed = 255);
    void stop();
};

#endif // BRUSH_MOTOR_H