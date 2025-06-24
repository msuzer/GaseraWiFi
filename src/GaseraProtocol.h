#pragma once

#include <Arduino.h>
#include <vector>

enum class GaseraStatus : uint8_t {
    DeviceInitializing = 0,
    InitializationError,
    DeviceIdle,
    SelfTestInProgress,
    Malfunction,
    Measuring,
    Calibrating,
    CancelingMeasurement,
    LaserScanning,
    Unknown = 255
};

class GaseraProtocol {
public:
    static constexpr char STX = 0x02;
    static constexpr char ETX = 0x03;
    static constexpr char BLANK = ' ';
    static constexpr char CHANNEL = '0';

    enum Command : uint8_t {
        ASTS, AERR, ATSK, STAM, STPM, ACON,
        SCOR, SCON, AMST, ANAM, STAT, AITR,
        ANET, SNET, APAR, SONL, ACLK, STUN,
        ATSP, ASYP, AMPS, ADEV, STST, ASTR, RDEV,
        COMMAND_COUNT
    };

    static const char* const CommandStrings[COMMAND_COUNT];
    static const char* deviceStatusToString(GaseraStatus status);

    struct Response {
        String command;
        bool success;
        String data;

        GaseraStatus status = GaseraStatus::Unknown;
        std::vector<String> errorCodes;
        std::vector<std::pair<String, String>> taskList;
        struct AconResult {
            String timestamp;
            String cas;
            String concentration;
        };
        std::vector<AconResult> results;

        explicit operator bool() const { return success; }
    };

    static void setClient(Client& c);
    static Client& getClient();

    using ResponseParser = void (*)(Response&);
    static Command commandFromString(const String& cmd);
    static const ResponseParser responseParsers[COMMAND_COUNT];

    static String buildRequest(Command cmd, const String& data = "");
    static Response parseResponse(const String& raw);
    static void printResponse(const Response& res);
    static Response query(Command cmd, const String& data = "", uint32_t timeoutMs = 1000);

    static Response askCurrentStatus() { return query(ASTS, "", 1000); }
    static Response askActiveErrors() { return query(AERR, "", 1000); }
    static Response askTaskList() { return query(ATSK, "", 1000); }
    static Response startMeasurement(uint8_t taskId) { return query(STAM, String(taskId), 1000); }
    static Response stopMeasurement() { return query(STPM, "", 1000); }
    static Response getLastResults() { return query(ACON, "", 1000); }
    static Response setComponentOrder(const String& casList) { return query(SCOR, casList, 1000); }
    static Response setConcentrationFormat(uint8_t showTime, uint8_t showCAS, uint8_t showConc, int8_t showInlet);
    static Response getMeasurementPhase() { return query(AMST, "", 1000); }
    static Response getDeviceName() { return query(ANAM, "", 1000); }
    static Response startMeasurementByName(const String& taskName) { return query(STAT, taskName, 1000); }
    static Response getIterationNumber() { return query(AITR, "", 1000); }
    static Response getNetworkSettings() { return query(ANET, "", 1000); }
    static Response setNetworkSettings(uint8_t useDHCP, const String& ip, const String& netmask, const String& gw);
    static Response getParameter(const String& name) { return query(APAR, name, 1000); }
    static Response setOnlineMode(bool enable) { return query(SONL, String(enable), 1000); }
    static Response getDateTime() { return query(ACLK, "", 1000); }
    static Response setLaserTuningInterval(uint16_t interval) { return query(STUN, String(interval), 1000); }
    static Response getTaskParameters(uint8_t taskId) { return query(ATSP, String(taskId), 1000); }
    static Response getSystemParameters() { return query(ASYP, "", 1000); }
    static Response getSamplerParameters() { return query(AMPS, "", 1000); }
    static Response getDeviceInfo() { return query(ADEV, "", 1000); }
    static Response startSelfTest() { return query(STST, "", 1000); }
    static Response getSelfTestResult() { return query(ASTR, "", 1000); }
    static Response rebootDevice() { return query(RDEV, "", 1000); }
private:
    static Client* client;
};
