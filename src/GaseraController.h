#pragma once
#include <Arduino.h>
#include <Client.h>
#include "AsyncTimer.h"

#define NAME_OF_SSID                "Sitecom05D1AE"
#define PASSWORD_OF_SSID            "UE7KSDBUWHU4"

#define GASERA_DEVICE_IP            "192.168.0.100"
#define GASERA_DEVICE_PORT          8888

#define GASERA_TASK_ID_DEFAULT 0

#define RC_TASK_IDLE 0x00
#define RC_TASK_TRIGGERED 0x04
#define RC_TASK_ABORTED 0x08

enum class TaskState {
    Idle = 0,
    QueryStatus,
    MoveToMark,
    StartMeasurement,
    WaitForMeasurement,
    StopMeasurement,
    MoveToHome
};

class GaseraController {
public:
    static GaseraController& getInstance();

    static void begin();
    static void MeasurementTask();
    void setTaskId(const int taskId) { currentTaskId = taskId; }
    int getTaskId() const { return currentTaskId; }
    TaskState getTaskState() const { return state; }
    void setTaskState(TaskState newState) { state = newState; }
    void setRCTaskState(uint8_t taskState);
    uint8_t getRCTaskState() const { return RCTaskState; }
    bool isConnectionEstablished() const { return connectionEstablished; }
    void setConnectionEstablished(bool established);

private:
    GaseraController() = default;

    bool connectionEstablished = false;
    uint8_t RCTaskState = RC_TASK_IDLE;
    int currentTaskId = GASERA_TASK_ID_DEFAULT;
    TaskState state = TaskState::Idle;
    int waitSeconds = 0;

    void transitionTo(TaskState newState, TimerID timer, uint32_t delayMs);
    void logState(TaskState s);
};
