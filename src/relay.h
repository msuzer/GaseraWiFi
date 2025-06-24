#ifndef RELAY_MOTOR_DRIVER_H
#define RELAY_MOTOR_DRIVER_H

#include <Arduino.h>

enum MotorStopReason {
  STOPPED_BY_USER = 0,
  STOPPED_BY_TIMEOUT,
  STOPPED_BY_LIMIT_SWITCH
};

class RelayMotorDriver {
  private:
    uint8_t forwardPin;
    uint8_t reversePin;
    uint8_t limitSwitchPin;

    // Movement tracking
    bool moving = false;
    uint32_t startTime = 0;
    uint32_t timeoutDuration = 0;

    // Debouncer
    static constexpr uint8_t DEBOUNCE_MAX = 5;
    uint8_t debounceCount = 0;
    bool limitSwitchStableState = false;

    MotorStopReason lastStopReason = STOPPED_BY_USER;

  public:
    RelayMotorDriver(uint8_t forwardRelayPin = -1, uint8_t reverseRelayPin = -1, uint8_t limitSwitchPin = -1 );
    void begin();
    void forward();
    void reverse();
    void stop();

    void moveForwardWithTimeout(uint32_t timeoutMs);
    void moveBackwardWithTimeout(uint32_t timeoutMs);

    bool isMotorDone() const { return !moving; }
    MotorStopReason getStopReason() const { return lastStopReason; }

    void update();  // Call this at 50 Hz

    bool isForward() const;
    bool isReverse() const;
    bool isStopped() const;
};

#endif // RELAY_MOTOR_DRIVER_H
