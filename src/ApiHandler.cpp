// ApiHandler.cpp
#include "ApiHandler.h"
#include <ArduinoJson.h>
#include "GaseraProtocol.h"

namespace ApiHandler {

String handleStatus() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::askCurrentStatus();
    doc["success"] = res.success;
    doc["status"] = GaseraProtocol::deviceStatusToString(res.status);
    String json;
    serializeJson(doc, json);
    return json;
}

String handleErrors() {
    DynamicJsonDocument doc(512);
    auto res = GaseraProtocol::askActiveErrors();
    doc["success"] = res.success;
    JsonArray arr = doc.createNestedArray("errors");
    for (const auto& e : res.errorCodes) arr.add(e);
    String json;
    serializeJson(doc, json);
    return json;
}

String handleResults() {
    DynamicJsonDocument doc(1024);
    auto res = GaseraProtocol::getLastResults();
    doc["success"] = res.success;
    JsonArray arr = doc.createNestedArray("results");
    for (const auto& r : res.results) {
        JsonObject obj = arr.createNestedObject();
        obj["timestamp"] = r.timestamp;
        obj["cas"] = r.cas;
        obj["concentration"] = r.concentration;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

String handleDevice() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::getDeviceName();
    doc["success"] = res.success;
    doc["name"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleNetwork() {
    DynamicJsonDocument doc(512);
    auto res = GaseraProtocol::getNetworkSettings();
    doc["success"] = res.success;
    if (res.success) {
        int start = 0;
        int part = 0;
        while (start < res.data.length()) {
            int end = res.data.indexOf(' ', start);
            if (end == -1) end = res.data.length();
            String token = res.data.substring(start, end);

            switch (part++) {
                case 0: doc["dhcp"] = token.toInt(); break;
                case 1: doc["ip"] = token; break;
                case 2: doc["netmask"] = token; break;
                case 3: doc["gateway"] = token; break;
            }
            start = end + 1;
        }
    }
    String json;
    serializeJson(doc, json);
    return json;
}

String handleDateTime() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::getDateTime();
    doc["success"] = res.success;
    doc["datetime"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleSystem() {
    DynamicJsonDocument doc(512);
    auto res = GaseraProtocol::getSystemParameters();
    doc["success"] = res.success;
    doc["parameters"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleComponentOrder(const String& casList) {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::setComponentOrder(casList);
    doc["success"] = res.success;
    doc["result"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleIterationNumber() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::getIterationNumber();
    doc["success"] = res.success;
    doc["value"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleSamplerParameters() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::getSamplerParameters();
    doc["success"] = res.success;
    doc["value"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleDeviceInfo() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::getDeviceInfo();
    doc["success"] = res.success;
    doc["value"] = res.data;
    String json;
    serializeJson(doc, json);
    return json;
}

String handleStartMeasurement(const String& taskIdStr) {
    int taskId = taskIdStr.toInt();
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::startMeasurement(taskId);
    doc["success"] = res.success;
    doc["status"] = GaseraProtocol::deviceStatusToString(res.status);
    String json;
    serializeJson(doc, json);
    return json;
}

String handleStopMeasurement() {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::stopMeasurement();
    doc["success"] = res.success;
    doc["status"] = GaseraProtocol::deviceStatusToString(res.status);
    String json;
    serializeJson(doc, json);
    return json;
}

String handleSetOnlineMode(bool enable) {
    DynamicJsonDocument doc(256);
    auto res = GaseraProtocol::setOnlineMode(enable);
    doc["success"] = res.success;
    doc["status"] = GaseraProtocol::deviceStatusToString(res.status);
    String json;
    serializeJson(doc, json);
    return json;
}

String extractQueryParam(const String& route, const String& key) {
    int qMark = route.indexOf('?');
    if (qMark == -1) return "";

    String query = route.substring(qMark + 1);
    int keyStart = query.indexOf(key + "=");
    if (keyStart == -1) return "";

    int valueStart = keyStart + key.length() + 1;
    int valueEnd = query.indexOf('&', valueStart);
    if (valueEnd == -1) valueEnd = query.length();

    return query.substring(valueStart, valueEnd);
}

String handleRequest(const String& route) {
    if (route.startsWith("GET /status")) return handleStatus();
    if (route.startsWith("GET /errors")) return handleErrors();
    if (route.startsWith("GET /results")) return handleResults();
    if (route.startsWith("GET /device")) return handleDevice();
    if (route.startsWith("GET /network")) return handleNetwork();
    if (route.startsWith("GET /datetime")) return handleDateTime();
    if (route.startsWith("GET /system")) return handleSystem();
    if (route.startsWith("GET /iteration")) return handleIterationNumber();
    if (route.startsWith("GET /sampler")) return handleSamplerParameters();
    if (route.startsWith("GET /info")) return handleDeviceInfo();
    if (route.startsWith("POST /componentOrder")) {
        String casList = extractQueryParam(route, "cas");
        return handleComponentOrder(casList);
    }
    if (route.startsWith("POST /startMeasurement")) {
        String taskIdStr = extractQueryParam(route, "taskId");
        return handleStartMeasurement(taskIdStr);
    }
    if (route.startsWith("POST /stopMeasurement")) return handleStopMeasurement();
    if (route.startsWith("POST /onlineOn")) return handleSetOnlineMode(true);
    if (route.startsWith("POST /onlineOff")) return handleSetOnlineMode(false);

    DynamicJsonDocument doc(128);
    doc["success"] = false;
    doc["error"] = "Unknown command";
    String json;
    serializeJson(doc, json);
    return json;
}

} // namespace ApiHandler
