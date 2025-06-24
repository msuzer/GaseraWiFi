#pragma once
#include <Arduino.h>

namespace ApiHandler {
    // GET Handlers
    String handleStatus();
    String handleErrors();
    String handleResults();
    String handleDevice();
    String handleNetwork();
    String handleDateTime();
    String handleSystem();
    String handleIterationNumber();
    String handleSamplerParameters();
    String handleDeviceInfo();

    // POST Handlers
    String handleStartMeasurement(const String& taskIdStr);
    String handleStopMeasurement();
    String handleSetOnlineMode(bool enable);
    String handleComponentOrder(const String& casList);

    // Dispatcher
    String handleRequest(const String& route);
    String extractQueryParam(const String& route, const String& key);
}
