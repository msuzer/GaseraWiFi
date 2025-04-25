#include "gasera.h"
#include "sys_config.h"
#include <WiFiEspAT.h>

// GASERA Queries
const char* str_get_device_status = "ASTS K0";
const char* str_set_online_measurement_mode = "SONL K0 0";  // Enable data saving to GASERA
const char* str_start_new_measurement = "STAM K0 11";
const char* str_stop_current_measurement = "STPM K0";

const char* str_device_error = "GASERA Error: %d";
const char* str_device_initializing = "GASERA Init";
const char* str_initialization_error = "GASERA Init Error";
const char* str_device_idle_state = "GASERA Idle";
const char* str_device_self_test_in_progress = "GASERA Self Test";
const char* str_malfunction = "GASERA Mulfunction";
const char* str_measurement_in_progress = "GASERA Measuring";
const char* str_calibration_in_progress = "GASERA Calibrating";
const char* str_canceling_measurement = "GASERA Canceling Measurement";
const char* str_laserscan_in_progress = "GASERA Laser Scanning";

bool getDeviceStatus(char* response) {
  return queryGasera(str_get_device_status, response);
}

bool startNewMeasurement(char* response) {
  return queryGasera(str_start_new_measurement, response);
}

bool stopCurrentMeasurement(char* response) {
  return queryGasera(str_stop_current_measurement, response);
}

bool setOnlineMeasurementMode(char* response) {
  return queryGasera(str_set_online_measurement_mode, response);
}

bool queryGasera(const char* query, char* response) {
  WiFiClient client;

  if (client.connect(GASERA_DEVICE_IP, GASERA_DEVICE_PORT)) {
    client.write(0x02);
    client.write(' ');
    client.println(query);
    client.write(0x03);

    int maxloops = 0;
    while (!client.available() && maxloops < 1000) {
      maxloops++;
      delay(1);  //delay 1 msec
    }

    if (client.available() > 0) {
      if (client.read() == 0x02) {
        int read = client.readBytesUntil(0x03, response, 64);
        response[read] = '\0';
      }
    } else {
      log_println("GASERA Response Timeout!");
      return false;
    }

    client.stop();
  } else {
    log_println("GASERA NOT Connected");
    return false;
  }

  return true;
}
