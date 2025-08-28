#include "GaseraController.h"
#include "GaseraProtocol.h"
#include "LogUtils.h"
#include "relay.h"

extern RelayMotorDriver motorA;
extern RelayMotorDriver motorB;

#define GASERA_MEASUREMENT_TIME         600 // secs
#define MOTOR_FULL_MOVEMENT_TIME_SEC    60 // secs

GaseraController& GaseraController::getInstance() {
    static GaseraController instance;
    return instance;
}

void GaseraController::begin() {
    // Initialization code here
}

void GaseraController::transitionTo(TaskState newState, TimerID timer, uint32_t delayMs) {
    state = newState;
    Timer::start(timer, delayMs);
    logState(state);
}

void GaseraController::logState(TaskState s) {
    static const char* names[] = {
        "Idle", "QueryStatus", "MoveToMark", "StartMeasurement",
        "WaitForMeasurement", "StopMeasurement", "MoveToHome"
    };
    LogUtils::info("[GaseraTask] State => %s\n", names[static_cast<int>(s)]);
}

void GaseraController::MeasurementTask() {
    auto& gasera = getInstance();
    static bool connectionStatusLast = true;

    bool connectionStatus = gasera.isConnectionEstablished();
    if (connectionStatusLast != connectionStatus && !connectionStatus) {
        connectionStatusLast = connectionStatus;
        LogUtils::warn("GASERA connection not established.\n");
        gasera.state = TaskState::Idle;
        gasera.RCTaskState = RC_TASK_IDLE;
        return;
    }
    connectionStatusLast = connectionStatus;

    switch (gasera.state) {
        case TaskState::Idle:
            if (gasera.RCTaskState == RC_TASK_TRIGGERED) {
                LogUtils::info("Measurement Triggered\n");
                gasera.transitionTo(TaskState::QueryStatus, TMR_DEVICE_STATUS, 100);
            }
            break;

        case TaskState::QueryStatus:
            if (Timer::expired(TMR_DEVICE_STATUS)) {
                auto resp = GaseraProtocol::askCurrentStatus();
                if (resp && resp.status == GaseraStatus::DeviceIdle) {
                    motorA.moveForwardWithTimeout(MOTOR_FULL_MOVEMENT_TIME_SEC * 1000);
                    motorB.moveForwardWithTimeout(MOTOR_FULL_MOVEMENT_TIME_SEC * 1000);
                    gasera.state = TaskState::MoveToMark;
                    gasera.logState(gasera.state);
                } else {
                    LogUtils::info("Device status: %s\n", GaseraProtocol::deviceStatusToString(resp.status));
                    LogUtils::info("Waiting for device to become idle...\n");
                    Timer::restart(TMR_DEVICE_STATUS, 1000);
                }
            }
            break;

        case TaskState::MoveToMark:
            if (motorA.isMotorDone() && motorB.isMotorDone()) {
                gasera.transitionTo(TaskState::StartMeasurement, TMR_DEVICE_STATUS, 1000);
            }
            break;
        case TaskState::StartMeasurement:
            if (Timer::expired(TMR_DEVICE_STATUS)) {
                auto resp = GaseraProtocol::startMeasurement(gasera.currentTaskId);
                if (resp && resp.success) {
                    gasera.waitSeconds = GASERA_MEASUREMENT_TIME;
                    gasera.transitionTo(TaskState::WaitForMeasurement, TMR_MEASUREMENT, 10000);
                } else {
                    LogUtils::info("Device status: %s\n", GaseraProtocol::deviceStatusToString(resp.status));
                    LogUtils::info("Measurement start failed or device busy.\n");
                    gasera.transitionTo(TaskState::StopMeasurement, TMR_ABORT_WAIT, 2000);
                }
            }
            break;

        case TaskState::WaitForMeasurement:
            if (gasera.RCTaskState == RC_TASK_ABORTED) {
                LogUtils::warn("Measurement aborted by user.\n");
                gasera.transitionTo(TaskState::StopMeasurement, TMR_ABORT_WAIT, 1000);
            } else if (Timer::expired(TMR_MEASUREMENT)) {
                gasera.waitSeconds -= 10;
                if (gasera.waitSeconds > 0) {
                    LogUtils::info("Measuring... remaining: %d\n", gasera.waitSeconds);
                    Timer::restart(TMR_MEASUREMENT, 10000);
                } else {
                    LogUtils::info("Measurement time reached.\n");
                    gasera.transitionTo(TaskState::StopMeasurement, TMR_ABORT_WAIT, 1000);
                }
            }
            break;

        case TaskState::StopMeasurement:
            if (Timer::expired(TMR_ABORT_WAIT)) {
                auto resp = GaseraProtocol::stopMeasurement();
                if (resp && resp.success) {
                    motorA.moveBackwardWithTimeout(MOTOR_FULL_MOVEMENT_TIME_SEC * 1000);
                    motorB.moveBackwardWithTimeout(MOTOR_FULL_MOVEMENT_TIME_SEC * 1000);
                    gasera.state = TaskState::MoveToHome;
                    gasera.logState(gasera.state);
                } else {
                    LogUtils::info("Device status: %s\n", GaseraProtocol::deviceStatusToString(resp.status));
                    LogUtils::info("Retry stopping measurement...\n");
                    Timer::restart(TMR_ABORT_WAIT, 2000);
                }
            }
            break;

        case TaskState::MoveToHome:
            if (motorA.isMotorDone() && motorB.isMotorDone()) {
                LogUtils::info("Measurement Task Completed\n");
                gasera.state = TaskState::Idle;
                gasera.RCTaskState = RC_TASK_IDLE;
            }
            break;
    }
}

void GaseraController::setRCTaskState(uint8_t taskState) {
    switch (RCTaskState) {
        case RC_TASK_IDLE:
            if (taskState == RC_TASK_TRIGGERED) {
                LogUtils::info("RC Task Triggered!");
                RCTaskState = RC_TASK_TRIGGERED;
            }
        case RC_TASK_TRIGGERED:
            if (taskState == RC_TASK_TRIGGERED) {
                LogUtils::warn("RC Task Aborted!");
                RCTaskState = RC_TASK_ABORTED;
            }
            break;
        case RC_TASK_ABORTED:
            if (taskState == RC_TASK_TRIGGERED) {
                LogUtils::info("RC Task Triggered Again!");
                RCTaskState = RC_TASK_TRIGGERED;
            }
            break;
        default:
            break;
    }
}

void GaseraController::setConnectionEstablished(bool established) {
    if (connectionEstablished != established) {
        connectionEstablished = established;
        if (established) {
            LogUtils::info("GASERA connection established.\n");
            auto resp = GaseraProtocol::setOnlineMode(true);
            if (resp) {
                LogUtils::info("Device status: %s\n", GaseraProtocol::deviceStatusToString(resp.status));
                if (resp.status == GaseraStatus::DeviceIdle) {
                    LogUtils::info("GASERA Ready for measurements.\n");
                } else {
                    LogUtils::info("Device status: %s\n", GaseraProtocol::deviceStatusToString(resp.status));
                }
            }
        } else {
            LogUtils::warn("GASERA connection lost.\n");
        }
    }
}
