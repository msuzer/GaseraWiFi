/*
 * GASERA Remote Controller v0.1
 * March 17, 2023
 * msuzer1@gmail.com
 *
 */

#ifndef GASERA_REMOTE_CONTROLLER
#define GASERA_REMOTE_CONTROLLER

#include <Arduino.h>
#include <stdint.h>

enum {
  no_error = 0,
  device_error
}; // DeviceError_t

enum {
  device_initializing = 0,
  initialization_error,
  device_idle_state,
  device_self_test_in_progress,
  malfunction,
  measurement_in_progress,
  calibration_in_progress,
  canceling_measurement,
  laserscan_in_progress
}; // DeviceStatus_t

// GASERA Queries
extern const char* str_get_device_status;
extern const char* str_get_measurement_status;
extern const char* str_set_online_measurement_mode;
extern const char* str_start_new_measurement;
extern const char* str_stop_current_measurement;

extern const char* str_device_error;
extern const char* str_device_initializing;
extern const char* str_initialization_error;
extern const char* str_device_idle_state;
extern const char* str_device_self_test_in_progress;
extern const char* str_malfunction;
extern const char* str_measurement_in_progress;
extern const char* str_calibration_in_progress;
extern const char* str_canceling_measurement;
extern const char* str_laserscan_in_progress;

void log_println(const char* message);
void log_printf(const char* format, ...);

bool queryGasera(const char* query, char* response);
bool getDeviceStatus(char* response);
bool startNewMeasurement(char* response);
bool stopCurrentMeasurement(char* response);
bool setOnlineMeasurementMode(char* response);

// Write into a C string (a char array), a la sprintf().
// Warning: there is no check for buffer overflow.
class StringPrinter : public Print {
public:
  StringPrinter(char* buffer)
    : buf(buffer), pos(0) {}
  virtual size_t write(uint8_t c) {
    buf[pos++] = c;  // add the character to the string
    buf[pos] = 0;    // null terminator
    return 1;        // one character written
  }
private:
  char* buf;
  size_t pos;
};

#endif  // GASERA_REMOTE_CONTROLLER