#include "GaseraController.h"
#include "sys_config.h"

GaseraController& GaseraController::getInstance() {
    static GaseraController instance;
    return instance;
}

bool GaseraController::getDeviceStatus(Client& client, char* response) {
    return queryDevice(client, CMD_GET_STATUS, response);
}

bool GaseraController::startNewMeasurement(Client& client, char* response) {
    return queryDevice(client, CMD_START_MEASUREMENT, response);
}

bool GaseraController::stopCurrentMeasurement(Client& client, char* response) {
    return queryDevice(client, CMD_STOP_MEASUREMENT, response);
}

bool GaseraController::setOnlineMeasurementMode(Client& client, char* response) {
    return queryDevice(client, CMD_SET_ONLINE, response);
}

bool GaseraController::queryDevice(Client& client, const char* command, char* response) {
    if (!client.connect(DEVICE_IP, DEVICE_PORT)) {
        log_println("GASERA: Connection failed");
        return false;
    }

    client.write(0x02);
    client.write(' ');
    client.println(command);
    client.write(0x03);

    int timeout = 1000;
    while (!client.available() && timeout--) {
        delay(1);
    }

    if (client.available() && client.read() == 0x02) {
        int len = client.readBytesUntil(0x03, response, 64);
        response[len] = '\0';
    } else {
        log_println("GASERA: No response");
        client.stop();
        return false;
    }

    client.stop();
    return true;
}

int GaseraController::parseResponse(const char* response) {
    if (!response || strlen(response) < 6) return -1;

    while (*response == ' ') response++;

    int error = 0;
    int status = DeviceIdle;

    if (strncmp(response, "ASTS", 4) == 0) {
        error = response[5] & 1;
        if (!error) status = response[7] & 0x0F;
    } else if (strncmp(response, "STAM", 4) == 0) {
        error = response[5] & 1;
        if (!error) log_println("Measurement started");
    } else if (strncmp(response, "STPM", 4) == 0) {
        error = response[5] & 1;
        if (!error) log_println("Measurement stopped");
    } else if (strncmp(response, "SONL", 4) == 0) {
        error = response[5] & 1;
        if (!error) log_println("Online mode enabled");
    } else {
        log_println("GASERA: Unknown response");
        return -1;
    }

    return (error == 0) ? status : -1;
}

void GaseraController::printStatus(int status) {
    switch (status) {
        case DeviceInitializing: log_println("GASERA: Initializing"); break;
        case InitializationError: log_println("GASERA: Init Error"); break;
        case DeviceIdle: log_println("GASERA: Idle"); break;
        case SelfTestInProgress: log_println("GASERA: Self-Test"); break;
        case Malfunction: log_println("GASERA: Malfunction"); break;
        case Measuring: log_println("GASERA: Measuring"); break;
        case Calibrating: log_println("GASERA: Calibrating"); break;
        case CancelingMeasurement: log_println("GASERA: Canceling"); break;
        case LaserScanning: log_println("GASERA: Laser Scanning"); break;
        default: log_printf_alt("GASERA: Unknown status %d", status); break;
    }

}