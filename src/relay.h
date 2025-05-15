#ifndef RELAY_MOTOR_DRIVER_H
#define RELAY_MOTOR_DRIVER_H

#include <Arduino.h>

class RelayMotorDriver {
  private:
    uint8_t forwardPin;
    uint8_t reversePin;

  public:
    RelayMotorDriver(uint8_t forwardRelayPin, uint8_t reverseRelayPin);
    void begin();
    void forward();
    void reverse();
    void stop();

    bool isForward() const;
    bool isReverse() const;
    bool isStopped() const;
};

#endif // RELAY_MOTOR_DRIVER_H
