#include <Arduino.h>
#include <stdarg.h>
#include <GyverOLED.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "relay.h"
#include "sys_config.h"
#include "sys_timer.h"
#include "SerialHandler.h"
#include "GaseraController.h"

#define MOTOR_FULL_MOVEMENT_TIME_SEC 60

#define MOTOR_A_FORWARD_PIN   22
#define MOTOR_A_REVERSE_PIN   21

// Motor B Relay Pins
#define MOTOR_B_FORWARD_PIN   19
#define MOTOR_B_REVERSE_PIN   18

#define RCTriggerPin          13

#define Motor1UpButtonPin     34
#define Motor1DownButtonPin   35
#define Motor2UpButtonPin     36
#define Motor2DownButtonPin   39

#define LimitSwitch1Pin       26
#define LimitSwitch2Pin       27

#define BUTTON_NO_PRESS 0x00
#define BUTTON_SHORT_PRESS 0x01
#define BUTTON_LONG_PRESS 0x02

#define RC_TASK_IDLE 0x00
#define RC_TASK_TRIGGERED 0x04
#define RC_TASK_ABORTED 0x08

#define TIMER_OVF_OCCURRED 0x20

static void checkRCTrigger(void);

static void SetupSysTickTimer(void);
static void OnTimerOverFlowEvent(uint8_t);

static void HandleAsyncEvents(void);
static void HandleUserInstruction(const char*);
static void ConnectionTask(uint8_t event, uint8_t idx);
static void MeasurementTask(uint8_t event, uint8_t idx);

GyverOLED<SSH1106_128x64> oled;
WiFiClient wifiClient;

// Motor A and B Instances
RelayMotorDriver motorA(MOTOR_A_FORWARD_PIN, MOTOR_A_REVERSE_PIN);
RelayMotorDriver motorB(MOTOR_B_FORWARD_PIN, MOTOR_B_REVERSE_PIN);

static uint8_t RCTaskState = RC_TASK_IDLE;

// Button IDs
enum ButtonId { BTN1_UP = 0, BTN1_DOWN, BTN2_UP, BTN2_DOWN, RC_TRIG, BTN_COUNT };
const uint8_t buttonPins[BTN_COUNT] = {Motor1UpButtonPin, Motor1DownButtonPin, Motor2UpButtonPin, Motor2DownButtonPin, RCTriggerPin};
volatile uint8_t buttonStates[BTN_COUNT] = {BUTTON_NO_PRESS, BUTTON_NO_PRESS, BUTTON_NO_PRESS, BUTTON_NO_PRESS, RC_TASK_IDLE};

void checkButton(ButtonId id);

static bool GASERAReady = false;

static char gaseraRxBuffer[64];

constexpr size_t bufferSize = 32;
char bufferA[bufferSize];
char bufferB[bufferSize];

SerialHandler serialHandler(bufferA, bufferB, bufferSize);

void onSerialMessage(const char* message, size_t length) {
    Serial.print("Received: ");
    Serial.write(message, length);
    Serial.println();
    
    if (serialHandler.isMessageTruncated()) {
        Serial.println("Warning: Message was truncated!");
    }

    HandleUserInstruction(message);
}

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(Motor1UpButtonPin, INPUT_PULLUP);
  pinMode(Motor1DownButtonPin, INPUT_PULLUP);
  pinMode(Motor2UpButtonPin, INPUT_PULLUP);
  pinMode(Motor2DownButtonPin, INPUT_PULLUP);

  pinMode(LimitSwitch1Pin, INPUT_PULLUP);
  pinMode(LimitSwitch2Pin, INPUT_PULLUP);

  pinMode(RCTriggerPin, INPUT_PULLUP);

  serialHandler.setCallback(onSerialMessage);

  motorA.begin();
  motorB.begin();

  oled.init();
  oled.autoPrintln(true);
  oled.setScale(2);

  SetupSysTickTimer();
  SYS_TIMER_Initialize();
  SYS_TIMER_SetOnTimerOverFlowEventHandler(OnTimerOverFlowEvent);

  log_println("GASERA Remote Control Project");
  HandleUserInstruction("print");

  WiFi.begin(NAME_OF_SSID, PASSWORD_OF_SSID);
  log_println("Connecting to WiFi...");
  SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(1000));  // Set Timer to Fire ConnectionTask()
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    serialHandler.onReceiveChar(c);
  }

  serialHandler.process();
  SYS_TIMER_Main_Tasks();
  HandleAsyncEvents();
}

void IRAM_ATTR onSysTick() {
  SYS_TIMER_Periodic_Tasks();
  checkRCTrigger();
  checkButton(BTN1_UP);
  checkButton(BTN1_DOWN);
  checkButton(BTN2_UP);
  checkButton(BTN2_DOWN);
  checkButton(RC_TRIG);
}

static void OnTimerOverFlowEvent(uint8_t idx) {
  ConnectionTask(TIMER_OVF_OCCURRED, idx);
  MeasurementTask(TIMER_OVF_OCCURRED, idx);
}

static void ConnectionTask(uint8_t event, uint8_t idx) {
  static int state = 0;
  int dev_status;

  if (event != TIMER_OVF_OCCURRED || idx != SYS_TMR1) {
    return;
  }

  auto& gasera = GaseraController::getInstance();

  switch (state) {
    case 0:
      if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();
        log_printf_alt("IP: %u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        state = 1;
      }
      SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(2000));
      break;
    case 1:
      log_println("Connecting to GASERA");
      SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(2000));
      state = 2;
      break;
    case 2:
      if (gasera.getDeviceStatus(wifiClient, gaseraRxBuffer)) {
        dev_status = gasera.parseResponse(gaseraRxBuffer);
        gasera.printStatus(dev_status);
        if (dev_status == GaseraController::DeviceIdle) {
          state = 3;
        } else if (dev_status == GaseraController::Measuring) {
          gasera.stopCurrentMeasurement(wifiClient, gaseraRxBuffer);
        }
      }
      SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(2000));
      break;
    case 3:
      if (gasera.setOnlineMeasurementMode(wifiClient, gaseraRxBuffer)) {
        dev_status = gasera.parseResponse(gaseraRxBuffer);
        if (dev_status == GaseraController::DeviceIdle) {
          log_println("GASERA Ready!");
          GASERAReady = true;
          state = -1;
        } else {
          gasera.printStatus(dev_status);
        }
      }
      SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(2000));
      break;
    default:
      break;
  }
}

static void MeasurementTask(uint8_t event, uint8_t idx) {
  static int state = 0;
  static int wait = GASERA_MEASUREMENT_TIME;
  int dev_status;

  bool timer_event = (event == TIMER_OVF_OCCURRED && idx == SYS_TMR0);

  auto& gasera = GaseraController::getInstance();

  switch (state) {
    case 0:
      if (event == RC_TASK_TRIGGERED) {
        if (GASERAReady) {
          log_println("New Task Triggered!");
          SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(100));
          state = 1;
        } else {
          log_println("GASERA NOT Ready!");
          RCTaskState = RC_TASK_IDLE;
        }
      }
      break;

    case 1:
      if (timer_event) {
        if (gasera.getDeviceStatus(wifiClient, gaseraRxBuffer)) {
          dev_status = gasera.parseResponse(gaseraRxBuffer);
          if (dev_status == GaseraController::DeviceIdle) {
            state = 2;
          } else {
            gasera.printStatus(dev_status);
          }
        }
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
      }
      break;

    case 2:
      if (timer_event) {
        HandleUserInstruction("gmark");
        SYS_TIMER_SetupTimer(SYS_TMR0, SEC_To_Ticks(MOTOR_FULL_MOVEMENT_TIME_SEC));
        state = 3;
      }
      break;

    case 3:
      if (timer_event) {
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(2000));
        log_println("Start New Measurement");
        if (gasera.startNewMeasurement(wifiClient, gaseraRxBuffer)) {
          dev_status = gasera.parseResponse(gaseraRxBuffer);
          if (dev_status == GaseraController::DeviceIdle) {
            wait = GASERA_MEASUREMENT_TIME;
            RCTaskState = RC_TASK_IDLE;
            state = 4;
          } else {
            gasera.printStatus(dev_status);
          }
        }
      }
      break;

    case 4:
      if (event == RC_TASK_ABORTED) {
        RCTaskState = RC_TASK_IDLE;
        log_println("Abort Measurement!");
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
        state = 5;
      } else if (timer_event) {
        if (wait > 0) {
          log_printf_alt("Awaiting: %03d Secs.", wait);
          wait -= 10;
          SYS_TIMER_SetupTimer(SYS_TMR0, SEC_To_Ticks(10));  // Wait 10 secs
        } else {
          log_println("Stop Measurement!");
          SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
          state = 5;
        }
      }
      break;

    case 5:
      if (timer_event) {
        if (gasera.stopCurrentMeasurement(wifiClient, gaseraRxBuffer)) {
          dev_status = gasera.parseResponse(gaseraRxBuffer);
          if (dev_status == GaseraController::DeviceIdle) {
            HandleUserInstruction("ghome");
            SYS_TIMER_SetupTimer(SYS_TMR0, SEC_To_Ticks(MOTOR_FULL_MOVEMENT_TIME_SEC));
            state = 6;
            return;
          } else {
            gasera.printStatus(dev_status);
          }
        }
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(2000));
      }
      break;

    case 6:
      if (timer_event) {
        RCTaskState = RC_TASK_IDLE;
        state = 0;   
      }
      break;

    default:
      break;
  }
}

static void HandleUserInstruction(const char* inst) {
  int32_t targetPosition;

  if (strlen(inst) < 3) {
    return;
  }

  if (strncmp(inst, "print", 5) == 0) {
    log_printf_alt("CurrPos: 0");
  } else if (strncmp(inst, "reset", 5) == 0) {
    log_println("Reset Home CurrPos: 0");
  } else if (strncmp(inst, "mark", 4) == 0) {
    log_printf_alt("Mark Pos: 0");
  } else if (strncmp(inst, "gmark", 5) == 0) {
    log_printf_alt("Goto Mark Position");
    motorA.forward();
    motorB.forward();
  } else if (strncmp(inst, "pmark", 5) == 0) {
    log_printf_alt("Mark Pos: 0");
  } else if (strncmp(inst, "smark", 5) == 0) {
    targetPosition = atol(&inst[5]);
    log_printf_alt("Mark Pos: %ld", targetPosition);
  } else if (strncmp(inst, "ghome", 5) == 0) {
    log_println("Going Home");
    motorA.reverse();
    motorB.reverse();
  } else {
    log_println("Unk Cmd!");
    // log_println("Try: print, home, mark, xon, xoff, m1000, m-1000, g2500, g-2500");
  }
}

static void handleMotorAction(RelayMotorDriver& motor, MotorDirection direction) {
  const char* motorName = (&motor == &motorA) ? "MotorA" : "MotorB";
  if (motor.isStopped()) {
    if (direction == MOTOR_UP) {
      motor.forward();
      log_printf_alt("Jog Run %s UP\n", motorName);
    } else {
      motor.reverse();
      log_printf_alt("Jog Run %s Down\n", motorName);
    }
  } else {
    motor.stop();
    log_printf_alt("%s Stop\n", motorName);
  }
}

static void HandleAsyncEvents(void) {
  if (buttonStates[BTN1_UP] == BUTTON_SHORT_PRESS) {
    handleMotorAction(motorA, MOTOR_UP);
    buttonStates[BTN1_UP] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN1_DOWN] == BUTTON_SHORT_PRESS) {
    handleMotorAction(motorA, MOTOR_DOWN);
    buttonStates[BTN1_DOWN] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN2_UP] == BUTTON_SHORT_PRESS) {
    handleMotorAction(motorB, MOTOR_UP);
    buttonStates[BTN2_UP] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN2_DOWN] == BUTTON_SHORT_PRESS) {
    handleMotorAction(motorB, MOTOR_DOWN);
    buttonStates[BTN2_DOWN] = BUTTON_NO_PRESS;
  } else if (RCTaskState == RC_TASK_TRIGGERED) {
    MeasurementTask(RC_TASK_TRIGGERED, 0);
  } else if (RCTaskState == RC_TASK_ABORTED) {
    MeasurementTask(RC_TASK_ABORTED, 0);
  } else {
  }
}

static void checkRCTrigger(void) {
  static bool pinStateOld = HIGH;
  static int counter = 0;
  bool pinState;

  pinState = digitalRead(RCTriggerPin);
  if (pinStateOld != pinState) {
    if (++counter > MS_To_Ticks(2000)) {
      counter = 0;
      pinStateOld = pinState;
      if (pinState == LOW) {
        RCTaskState = RC_TASK_TRIGGERED;
      } else {
        RCTaskState = RC_TASK_ABORTED;
      }
    }
  }
}

void checkButton(ButtonId id) {
  static bool buttonPinStatesOld[BTN_COUNT] = {HIGH, HIGH, HIGH, HIGH};
  static int buttonCounters[BTN_COUNT] = {0, 0, 0, 0};

  bool pinState = digitalRead(buttonPins[id]);

  if (pinState == LOW) {
    if (buttonPinStatesOld[id] != pinState) {
      if (++buttonCounters[id] >= MS_To_Ticks(2000)) {
        buttonStates[id] = BUTTON_LONG_PRESS;
        buttonPinStatesOld[id] = pinState;
        buttonCounters[id] = 0;
      }
    }
  } else {
    if (buttonCounters[id] >= MS_To_Ticks(100)) {
      buttonStates[id] = BUTTON_SHORT_PRESS;
    }
    buttonCounters[id] = 0;
    buttonPinStatesOld[id] = pinState;
  }
}

void oledPrintln(const char* str) {
  oled.clear();
  oled.home();
  oled.println(str);
  oled.update();
}

void log_println(const char* message) {
  Serial.println(message);
  oledPrintln(message);
}

void log_printf_alt(const char* format, ...) {
  static char txBuffer[64];
  va_list args;
  va_start(args, format);
  vsprintf(txBuffer, format, args);
  va_end(args);
  log_println(txBuffer);
}

// SetupSysTickTimer: Generates an interrupt every 20 ms (CLOCK_TICK_RESOLUTION)
void SetupSysTickTimer() {
  const int TIMER_NUM = 0;       // ESP32 has timers 0-3
  const int PRESCALER = 80;      // 80 MHz / 80 = 1 MHz (1 us per tick)

  hw_timer_t* sysTimer = timerBegin(TIMER_NUM, PRESCALER, true);  // countUp = true
  timerAttachInterrupt(sysTimer, &onSysTick, true);   // Edge triggered

  const uint64_t ALARM_VALUE_US = CLOCK_TICK_RESOLUTION * 1000;  // 20ms = 20000 us

  timerAlarmWrite(sysTimer, ALARM_VALUE_US, true);   // Auto-reload = true
  timerAlarmEnable(sysTimer);                        // Start timer
}
