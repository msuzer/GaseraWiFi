// ============================================
// File: LogUtils.cpp
// Purpose: Implementation of centralized logging helpers
// Part of: Core system utilities
// License: Proprietary License
// Author: 
// Date: 
// ============================================

#include <Arduino.h>
#include <stdarg.h>
#include <GyverOLED.h>
#include "LogUtils.h"
#include "IOConfig.h"  // For UserLEDPin

extern GyverOLED<SSH1106_128x64> oled;
LogLevel LogUtils::currentLogLevel = LogLevel::Info;

void LogUtils::setLogLevel(LogLevel level) {
    // Ensure the log level is within the defined range
    if (level < LogLevel::Silent)
        level = LogLevel::Silent;
    else if (level > LogLevel::Verbose)
        level = LogLevel::Verbose;

    currentLogLevel = level;
    info("Current Log Level: %s\n", logLevelToString(currentLogLevel));
}

LogLevel LogUtils::getLogLevel() {
    return currentLogLevel;
}

const char* LogUtils::logLevelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Silent:  return "Silent";
        case LogLevel::Error:   return "Error";
        case LogLevel::Warn:    return "Warn";
        case LogLevel::Info:    return "Info";
        case LogLevel::Verbose: return "Verbose";
        default:                return "Unknown";
    }
}

void LogUtils::die(const char* format, ...) {
    oled.clear();
    oled.home();
    printf("[DIE] ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    oled.printf(format, args);
    va_end(args);

    while (1) {
        digitalWrite(UserLEDPin, HIGH);
        delay(100);
        digitalWrite(UserLEDPin, LOW);
        delay(1000);
    }
}

void LogUtils::warn(const char* format, ...) {
    if (currentLogLevel < LogLevel::Warn)
        return;

    oled.clear();
    oled.home();
    printf("[WARN] ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    oled.printf(format, args);
    va_end(args);
}

void LogUtils::info(const char* format, ...) {
    if (currentLogLevel < LogLevel::Info)
        return;

    oled.clear();
    oled.home();
    printf("[INFO] ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    oled.printf(format, args);
    va_end(args);
}

void LogUtils::verbose(const char* format, ...) {
    if (currentLogLevel < LogLevel::Verbose)
        return;

    printf("[VERBOSE] ");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
