#include <Arduino.h>
#include <GyverOLED.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "IOConfig.h"
#include "relay.h"
#include "AsyncTimer.h"
#include "SerialHandler.h"
#include "LogUtils.h"
#include "GaseraProtocol.h"
#include "GaseraController.h"
#include "ApiHandler.h"
#include "WebUI.h"

#define LCD_I2C_DEVICE_ADDRESS		0x27

#define BUTTON_NO_PRESS 0x00
#define BUTTON_SHORT_PRESS 0x01
#define BUTTON_LONG_PRESS 0x02

static void checkRCTrigger(void);

static void SetupSysTickTimer(void);
static void HandleAsyncEvents(void);
static void ConnectionMonitoringTask();

GyverOLED<SSH1106_128x64> oled;
WiFiClient wifiClient;
WebServer server(80);

// Motor A and B Instances
RelayMotorDriver motorA(MOTOR_A_FORWARD_PIN, MOTOR_A_REVERSE_PIN, MotorALimitSwitchPin);
RelayMotorDriver motorB(MOTOR_B_FORWARD_PIN, MOTOR_B_REVERSE_PIN, MotorBLimitSwitchPin);

// Button IDs
enum ButtonId { BTN1_UP = 0, BTN1_DOWN, BTN2_UP, BTN2_DOWN, RC_TRIG, BTN_COUNT };
const uint8_t buttonPins[BTN_COUNT] = {UserButton1Pin, UserButton2Pin, UserButton3Pin, UserButton4Pin, RCTriggerPin};
volatile uint8_t buttonStates[BTN_COUNT] = {BUTTON_NO_PRESS, BUTTON_NO_PRESS, BUTTON_NO_PRESS, BUTTON_NO_PRESS, RC_TASK_IDLE};

void checkButton(ButtonId id);

constexpr size_t bufferSize = 32;
char bufferA[bufferSize];
char bufferB[bufferSize];

SerialHandler serialHandler(bufferA, bufferB, bufferSize);

/*
Example Serial Commands:
GET /errors
GET /status
GET /info
POST /startMeasurement?taskId=2
POST /onlineOn
POST /onlineOff
POST /stopMeasurement
*/
void onSerialMessage(const char* message, size_t length) {
  String request = String(message, length);
  request.trim();

  LogUtils::info("Received Serial Message: %s\n", message);

  if (serialHandler.isMessageTruncated()) {
      LogUtils::warn("Message was truncated!\n");
  }

  String response = ApiHandler::handleRequest(request);
  LogUtils::info("Response: %s\n", response.c_str());
}

void handleApiRequest() {
  String path = server.method() == HTTP_POST ? "POST " : "GET ";
  path += server.uri();
  String response = ApiHandler::handleRequest(path);
  server.send(200, "application/json", response);
}

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(UserButton1Pin, INPUT_PULLUP);
  pinMode(UserButton2Pin, INPUT_PULLUP);
  pinMode(UserButton3Pin, INPUT_PULLUP);
  pinMode(UserButton4Pin, INPUT_PULLUP);

  pinMode(MotorALimitSwitchPin, INPUT_PULLUP);
  pinMode(MotorBLimitSwitchPin, INPUT_PULLUP);

  pinMode(RCTriggerPin, INPUT_PULLUP);
  pinMode(UserLEDPin, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  serialHandler.setCallback(onSerialMessage);
  GaseraProtocol::setClient(wifiClient);

  motorA.begin();
  motorB.begin();

  oled.init();
  oled.autoPrintln(true);
  oled.setScale(2);

  SetupSysTickTimer();
  LogUtils::info("GASERA Remote Control Project\n");

  WiFi.begin(NAME_OF_SSID, PASSWORD_OF_SSID);
  LogUtils::info("Connecting to WiFi...\n");
  Timer::start(TMR_CHECK_CONNECTION, 1000);  // Check connection every second
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    serialHandler.onReceiveChar(c);
  }

  serialHandler.process();
  GaseraController::MeasurementTask();
  ConnectionMonitoringTask();
  HandleAsyncEvents();

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}

void IRAM_ATTR onSysTick() {
  Timer::tick();
  checkRCTrigger();
  checkButton(BTN1_UP);
  checkButton(BTN1_DOWN);
  checkButton(BTN2_UP);
  checkButton(BTN2_DOWN);
  checkButton(RC_TRIG);
  motorA.update();
  motorB.update();
}

static void ConnectionMonitoringTask() {
  static bool connectionStatus = false;

  if (Timer::expired(TMR_CHECK_CONNECTION)) {
    Timer::restart(TMR_CHECK_CONNECTION, 1000);  // Reset timer for next check
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    digitalWrite(UserLEDPin, wifiConnected ? HIGH : LOW);

    if (connectionStatus != wifiConnected) {
      connectionStatus = wifiConnected;
      LogUtils::info("WiFi connection status changed: %s\n", wifiConnected ? "Connected" : "Disconnected");

      GaseraController::getInstance().setConnectionEstablished(wifiConnected);
      if (!wifiConnected) {
        LogUtils::warn("WiFi not connected, retrying...\n");
        WiFi.reconnect();
      } else {
        IPAddress ip = WiFi.localIP();
        LogUtils::info("IP: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);

        server.onNotFound(handleApiRequest);
        server.on("/", HTTP_GET, []() {
          server.send_P(200, "text/html", htmlPage);
        });

        server.begin();  // ✅ Start HTTP server AFTER registering routes
        LogUtils::info("HTTP server started.\n");
      }
    }
  }
}

static void HandleAsyncEvents(void) {
  if (buttonStates[BTN1_UP] == BUTTON_SHORT_PRESS) {
    motorA.forward();
    LogUtils::info("Jog Run MotorA UP\n");
    buttonStates[BTN1_UP] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN1_DOWN] == BUTTON_SHORT_PRESS) {
    motorA.reverse();
    LogUtils::info("Jog Run MotorA Down\n");
    buttonStates[BTN1_DOWN] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN2_UP] == BUTTON_SHORT_PRESS) {
    motorB.forward();
    LogUtils::info("Jog Run MotorB UP\n");
    buttonStates[BTN2_UP] = BUTTON_NO_PRESS;
  } else if (buttonStates[BTN2_DOWN] == BUTTON_SHORT_PRESS) {
    motorB.reverse();
    LogUtils::info("Jog Run MotorB Down\n");
    buttonStates[BTN2_DOWN] = BUTTON_NO_PRESS;
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
        GaseraController::getInstance().setRCTaskState(RC_TASK_TRIGGERED);
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
