#include "GaseraProtocol.h"

Client* GaseraProtocol::client = nullptr;

void GaseraProtocol::setClient(Client& c) {
    client = &c;
}

Client& GaseraProtocol::getClient() {
    return *client;
}

const char* const GaseraProtocol::CommandStrings[COMMAND_COUNT] = {
    "ASTS", "AERR", "ATSK", "STAM", "STPM", "ACON",
    "SCOR", "SCON", "AMST", "ANAM", "STAT", "AITR",
    "ANET", "SNET", "APAR", "SONL", "ACLK", "STUN",
    "ATSP", "ASYP", "AMPS", "ADEV", "STST", "ASTR", "RDEV"
};

GaseraProtocol::Command GaseraProtocol::commandFromString(const String& cmd) {
    for (int i = 0; i < COMMAND_COUNT; ++i) {
        if (cmd.equalsIgnoreCase(CommandStrings[i])) {
            return static_cast<Command>(i);
        }
    }
    return COMMAND_COUNT;
}

const char* GaseraProtocol::deviceStatusToString(GaseraStatus status) {
    switch (status) {
        case GaseraStatus::DeviceInitializing:     return "Device Initializing";
        case GaseraStatus::InitializationError:    return "Initialization Error";
        case GaseraStatus::DeviceIdle:             return "Device Idle";
        case GaseraStatus::SelfTestInProgress:     return "Self-Test In Progress";
        case GaseraStatus::Malfunction:            return "Malfunction";
        case GaseraStatus::Measuring:              return "Measuring";
        case GaseraStatus::Calibrating:            return "Calibrating";
        case GaseraStatus::CancelingMeasurement:   return "Canceling Measurement";
        case GaseraStatus::LaserScanning:          return "Laser Scanning";
        default:                                   return "Unknown";
    }
}

String GaseraProtocol::buildRequest(Command cmd, const String& data) {
    String msg;
    const char* cmdStr = CommandStrings[cmd];
    msg += STX; msg += BLANK; msg += cmdStr; msg += BLANK;
    msg += 'K'; msg += CHANNEL;
    if (!data.isEmpty()) {
        msg += BLANK; msg += data;
    }
    msg += ETX;
    return msg;
}

// Parsers for specific commands
static void parseASTSResponse(GaseraProtocol::Response& res) {
    uint8_t v = res.data.toInt();
    res.status = (v <= uint8_t(GaseraStatus::LaserScanning))
        ? static_cast<GaseraStatus>(v)
        : GaseraStatus::Unknown;
}

static void parseAERRResponse(GaseraProtocol::Response& res) {
    res.errorCodes.clear();
    int i = 0;
    while (i < res.data.length()) {
        int sp = res.data.indexOf(' ', i);
        if (sp == -1) sp = res.data.length();
        String c = res.data.substring(i, sp);
        if (!c.isEmpty()) res.errorCodes.push_back(c);
        i = sp + 1;
    }
}

static void parseATSKResponse(GaseraProtocol::Response& res) {
    res.taskList.clear();
    int i = 0;
    while (i < res.data.length()) {
        int sp = res.data.indexOf(' ', i);
        if (sp == -1) break;
        String id = res.data.substring(i, sp);
        i = sp + 1;
        sp = res.data.indexOf(' ', i);
        if (sp == -1) sp = res.data.length();
        String nm = res.data.substring(i, sp);
        res.taskList.emplace_back(id, nm);
        i = sp + 1;
    }
}

static void parseACONResponse(GaseraProtocol::Response& res) {
    res.results.clear();
    int i = 0;
    while (i < res.data.length()) {
        int sp = res.data.indexOf(' ', i);
        if (sp == -1) break;
        String ts = res.data.substring(i, sp);
        i = sp + 1;
        sp = res.data.indexOf(' ', i);
        if (sp == -1) break;
        String cas = res.data.substring(i, sp);
        i = sp + 1;
        sp = res.data.indexOf(' ', i);
        if (sp == -1) sp = res.data.length();
        String conc = res.data.substring(i, sp);
        res.results.push_back({ts, cas, conc});
        i = sp + 1;
    }
}

const GaseraProtocol::ResponseParser GaseraProtocol::responseParsers[COMMAND_COUNT] = {
    parseASTSResponse, parseAERRResponse, parseATSKResponse,
    nullptr, nullptr, parseACONResponse,
    nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
};

GaseraProtocol::Response GaseraProtocol::parseResponse(const String& raw) {
    Response r;
    if (raw.length()<10 || raw[0]!=STX || raw[raw.length()-1]!=ETX) {
        r.success = false; return r;
    }
    r.command = raw.substring(2,6);
    int sp = raw.indexOf(BLANK,6)+1;
    if (sp<=6 || sp>=raw.length()) { r.success=false; return r;}
    r.success = raw[sp]=='0';
    int ds = raw.indexOf(BLANK,sp);
    if (ds!=-1 && ds+1<raw.length()-1)
        r.data = raw.substring(ds+1, raw.length()-1);

    auto cmd = commandFromString(r.command);
    if (cmd < COMMAND_COUNT && responseParsers[cmd])
        responseParsers[cmd](r);
    return r;
}

void GaseraProtocol::printResponse(const Response& res) {
    Serial.println("Command: " + res.command);
    Serial.println("Status: " + String(res.success ? "OK" : "ERROR"));
    if (!res.success) return;

    Command cmd = commandFromString(res.command);
    switch (cmd) {
        case ASTS:
            Serial.println("Device Status: " + String(deviceStatusToString(res.status)));
            break;

        case AERR:
            Serial.println("Active Errors:");
            for (const auto& code : res.errorCodes)
                Serial.println("  - " + code);
            break;

        case ATSK:
            Serial.println("Task List:");
            for (const auto& task : res.taskList)
                Serial.println("  [" + task.first + "] " + task.second);
            break;

        case ACON:
            Serial.println("Measurement Results:");
            for (const auto& r : res.results)
                Serial.println("  [" + r.timestamp + "] " + r.cas + " = " + r.concentration + " ppm");
            break;

        default:
            Serial.println("Raw Data: " + res.data);
            break;
    }
}

GaseraProtocol::Response GaseraProtocol::query(Command cmd, const String& data, uint32_t timeoutMs) {
    // Send the request
    String request = buildRequest(cmd, data);
    client->print(request);

    // Wait for response with timeout
    String response;
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        while (client->available()) {
            char c = client->read();
            response += c;
            if (c == ETX) {
                // Finished reading a full response
                return parseResponse(response);
            }
        }
        delay(10); // small delay to avoid busy waiting
    }

    // Timeout occurred
    Response res;
    res.success = false;
    return res;
}

GaseraProtocol::Response GaseraProtocol::setConcentrationFormat(uint8_t showTime, uint8_t showCAS, uint8_t showConc, int8_t showInlet) {
    String data = String(showTime) + " " + String(showCAS) + " " + String(showConc);
    if (showInlet >= 0) data += " " + String(showInlet);
    return query(SCON, data, 1000);
}

GaseraProtocol::Response GaseraProtocol::setNetworkSettings(uint8_t useDHCP, const String& ip, const String& netmask, const String& gw) {
    String data = String(useDHCP) + " " + ip + " " + netmask + " " + gw;
    return query(SNET, data, 1000);
}
