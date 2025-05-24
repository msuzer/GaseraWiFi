
#ifndef GASERA_REMOTE_CONTROLLER_H
#define GASERA_REMOTE_CONTROLLER_H

#include <Arduino.h>
#include <Client.h>
#include "sys_config.h"

void log_println(const char* message);
void log_printf_alt(const char* format, ...);

class GaseraController {
public:
    static GaseraController& getInstance();

    bool getDeviceStatus(Client& client, char* response);
    bool startNewMeasurement(Client& client, char* response);
    bool stopCurrentMeasurement(Client& client, char* response);
    bool setOnlineMeasurementMode(Client& client, char* response);

    int parseResponse(const char* response);
    void printStatus(int status);

    enum DeviceStatus {
        DeviceInitializing = 0,
        InitializationError,
        DeviceIdle,
        SelfTestInProgress,
        Malfunction,
        Measuring,
        Calibrating,
        CancelingMeasurement,
        LaserScanning
    };

    private:
    GaseraController() = default;
    ~GaseraController() = default;
    GaseraController(const GaseraController&) = delete;
    GaseraController& operator=(const GaseraController&) = delete;

    bool queryDevice(Client& client, const char* command, char* response);

    static constexpr const char* DEVICE_IP = GASERA_DEVICE_IP;
    static constexpr uint16_t DEVICE_PORT = GASERA_DEVICE_PORT;

    static constexpr const char* CMD_GET_STATUS = "ASTS K0";
    static constexpr const char* CMD_SET_ONLINE = "SONL K0 0";
    static constexpr const char* CMD_START_MEASUREMENT = "STAM K0 11";
    static constexpr const char* CMD_STOP_MEASUREMENT = "STPM K0";
};

#endif // GASERA_REMOTE_CONTROLLER_H
