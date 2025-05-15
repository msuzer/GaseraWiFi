/*
 * To Upload Sketch, Set Dipswitches to:
 * OFF	OFF	ON	ON	OFF	OFF	OFF (Upload Sketch)
 * 
 * To Connect Serial Port, Set Dipswitches to:
 *
 * ON	  ON	ON	ON	OFF	OFF	OFF	NoUSE (USB to MEGA)
 *
 */

 #include "relay.h"
#include "gasera.h"
#include "sys_config.h"
#include "sys_timer.h"
#include <stdarg.h>
#include <WiFiEspAT.h>
#include <GyverOLED.h>

#define MOTOR_A_FORWARD_PIN  2
#define MOTOR_A_REVERSE_PIN  3

// Motor B Relay Pins
#define MOTOR_B_FORWARD_PIN  4
#define MOTOR_B_REVERSE_PIN  5

#define RCTriggerPin 9    //
#define MarkButtonPin A0  // BROWN
#define DownButtonPin A1  // ORANGE
#define UpButtonPin A2    // RED
#define HomeButtonPin A3  // YELLOW

#define LimitSwitch1Pin A4  // Conflicts I2C SDA!
#define LimitSwitch2Pin A5  // Conflicts I2C SCL!

#define BUTTON_NO_PRESS 0x00
#define BUTTON_SHORT_PRESS 0x01
#define BUTTON_LONG_PRESS 0x02

#define RC_TASK_IDLE 0x00
#define RC_TASK_TRIGGERED 0x04
#define RC_TASK_ABORTED 0x08

#define SERIAL_PORT_IDLE 0x00
#define SERIAL_DATA_RECEIVED 0x10

#define TIMER_OVF_OCCURRED 0x20

static void checkRCTrigger(void);
static void checkHomeButton(void);
static void checkMarkButton(void);
static void checkSerialData(void);

static void jogRunMotors(void);

static void SetupSysTickTimer(void);
static void OnTimerOverFlowEvent(uint8_t);

static void HandleAsyncEvents(void);
static void HandleUserInstruction(const char*);
static void ConnectionTask(uint8_t event, uint8_t idx);
static void MeasurementTask(uint8_t event, uint8_t idx);

static void GASERA_PrintStatus(int);
static int GASERA_ParseResponse(const char*);

GyverOLED<SSH1106_128x64> oled;

// Motor A and B Instances
RelayMotorDriver motorA(MOTOR_A_FORWARD_PIN, MOTOR_A_REVERSE_PIN);
RelayMotorDriver motorB(MOTOR_B_FORWARD_PIN, MOTOR_B_REVERSE_PIN);

static uint8_t btnHomeState = BUTTON_NO_PRESS;
static uint8_t btnMarkState = BUTTON_NO_PRESS;
static uint8_t RCTaskState = RC_TASK_IDLE;
static uint8_t SerialPortState = SERIAL_PORT_IDLE;

static bool GASERAReady = false;

static char gaseraRxBuffer[64];
static char serialBuffer[32];

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(RCTriggerPin, INPUT_PULLUP);
  pinMode(UpButtonPin, INPUT_PULLUP);
  pinMode(DownButtonPin, INPUT_PULLUP);
  pinMode(HomeButtonPin, INPUT_PULLUP);
  pinMode(MarkButtonPin, INPUT_PULLUP);

  //pinMode(LimitSwitch1Pin, INPUT_PULLUP);  // set pull-up
  //pinMode(LimitSwitch2Pin, INPUT_PULLUP);  // set pull-up

  oled.init();
  oled.autoPrintln(true);
  oled.setScale(2);

  SetupSysTickTimer();
  SYS_TIMER_Initialize();
  SYS_TIMER_SetOnTimerOverFlowEventHandler(OnTimerOverFlowEvent);

  log_println("GASERA Remote Control Project");
  HandleUserInstruction("print");

  Serial3.begin(115200);
  WiFi.init(Serial3);

  if (WiFi.status() != WL_NO_MODULE) {
    WiFi.begin(NAME_OF_SSID, PASSWORD_OF_SSID);
    log_println("WiFi module OK!");
    SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(1000));  // Set Timer to Fire ConnectionTask()
  } else {
    log_println("No WiFi Module!");
  }
}

void loop() {
  SYS_TIMER_Main_Tasks();
  jogRunMotors();
  HandleAsyncEvents();
}

ISR(TIMER1_COMPA_vect) {  // timer1 compare interrupt service routine
  SYS_TIMER_Periodic_Tasks();
  checkSerialData();
  checkRCTrigger();
  checkHomeButton();
  checkMarkButton();
}

static void OnTimerOverFlowEvent(uint8_t idx) {
  ConnectionTask(TIMER_OVF_OCCURRED, idx);
  MeasurementTask(TIMER_OVF_OCCURRED, idx);
}

static void ConnectionTask(uint8_t event, uint8_t idx) {
  static int state = 0;
  char buf[16];
  int dev_status;

  if (event != TIMER_OVF_OCCURRED || idx != SYS_TMR1) {
    return;
  }

  switch (state) {
    case 0:
      if (WiFi.status() == WL_CONNECTED) {
        StringPrinter(buf).print(WiFi.localIP());
        log_printf("IP: %s", buf);
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
      if (getDeviceStatus(gaseraRxBuffer)) {
        dev_status = GASERA_ParseResponse(gaseraRxBuffer);
        GASERA_PrintStatus(dev_status);
        if (dev_status == device_idle_state) {
          state = 3;
        } else if (dev_status == measurement_in_progress) {
          stopCurrentMeasurement(gaseraRxBuffer);
        }
      }
      SYS_TIMER_SetupTimer(SYS_TMR1, MS_To_Ticks(2000));
      break;
    case 3:
      if (setOnlineMeasurementMode(gaseraRxBuffer)) {
        dev_status = GASERA_ParseResponse(gaseraRxBuffer);
        if (dev_status == device_idle_state) {
          log_println("GASERA Ready!");
          GASERAReady = true;
          state = -1;
        } else {
          GASERA_PrintStatus(dev_status);
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

  switch (state) {
    case 0:
      if (event == RC_TASK_TRIGGERED) {
        if (GASERAReady) {
          log_println("New Task  Triggered!");
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
        if (getDeviceStatus(gaseraRxBuffer)) {
          dev_status = GASERA_ParseResponse(gaseraRxBuffer);
          if (dev_status == device_idle_state) {
            state = 2;
          } else {
            GASERA_PrintStatus(dev_status);
          }
        }
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
      }
      break;
    case 2:
      if (timer_event) {
        HandleUserInstruction("gmark");
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
        log_println("Start New Measuremnt");
        state = 3;
      }
      break;
    case 3:
      if (timer_event) {
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(2000));
        if (startNewMeasurement(gaseraRxBuffer)) {
          dev_status = GASERA_ParseResponse(gaseraRxBuffer);
          if (dev_status == device_idle_state) {
            wait = GASERA_MEASUREMENT_TIME;
            RCTaskState = RC_TASK_IDLE;
            state = 4;
          } else {
            GASERA_PrintStatus(dev_status);
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
          log_printf("Awaiting: %03d Secs.", wait);
          wait -= 10;
          SYS_TIMER_SetupTimer(SYS_TMR0, SEC_To_Ticks(10));  // Wait 5 mins
        } else {
          log_println("Stop Measurement!");
          SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(1000));
          state = 5;
        }
      }
      break;
    case 5:
      if (timer_event) {
        if (stopCurrentMeasurement(gaseraRxBuffer)) {
          dev_status = GASERA_ParseResponse(gaseraRxBuffer);
          if (dev_status == device_idle_state) {
            HandleUserInstruction("ghome");
            RCTaskState = RC_TASK_IDLE;
            state = 0;
          } else {
            GASERA_PrintStatus(dev_status);
          }
        }
        SYS_TIMER_SetupTimer(SYS_TMR0, MS_To_Ticks(2000));
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
    log_printf("CurrPos: 0");
  } else if (strncmp(inst, "reset", 5) == 0) {
    log_println("Reset Home CurrPos: 0");
  } else if (strncmp(inst, "mark", 4) == 0) {
    log_printf("Mark Pos: 0");
  } else if (strncmp(inst, "gmark", 5) == 0) {
    log_printf("Goto Mark Position");
    motorA.forward();
    motorB.forward();
  } else if (strncmp(inst, "pmark", 5) == 0) {
    log_printf("Mark Pos: 0");
  } else if (strncmp(inst, "smark", 5) == 0) {
    targetPosition = atol(&inst[5]);
    log_printf("Mark Pos: %ld", targetPosition);
  } else if (strncmp(inst, "ghome", 5) == 0) {
    log_println("Going Home");
    motorA.reverse();
    motorB.reverse();
  } else {
    log_println("Unk Cmd!");
    // log_println("Try: print, home, mark, xon, xoff, m1000, m-1000, g2500, g-2500");
  }
}

static void GASERA_PrintStatus(int status) {
  switch (status) {
    case device_initializing:
      log_println(str_device_initializing);
      break;
    case initialization_error:
      log_println(str_initialization_error);
      break;
    case device_idle_state:
      log_println(str_device_idle_state);
      break;
    case device_self_test_in_progress:
      log_println(str_device_self_test_in_progress);
      break;
    case malfunction:
      log_println(str_malfunction);
      break;
    case measurement_in_progress:
      log_println(str_measurement_in_progress);
      break;
    case calibration_in_progress:
      log_println(str_calibration_in_progress);
      break;
    case canceling_measurement:
      log_println(str_canceling_measurement);
      break;
    case laserscan_in_progress:
      log_println(str_laserscan_in_progress);
      break;
    default:
      log_printf(str_device_error, status);  // Report Error
      break;
  }
}

static int GASERA_ParseResponse(const char* response) {
  int error = no_error;
  int device_status = device_idle_state;

  // Trim leading space!
  for (int i = 0; i < 64; i++) {
    if (*response == ' ') {
      response++;
    } else {
      break;
    }
  }

  if (strlen(response) < 6) {
    return -1;
  }

  if (strncmp(response, str_get_device_status, 4) == 0) {
    // response ASTS <errorstatus> <device_status> (errorstatus: 0=no errors, 1=error)
    error = response[5] & 1;
    if (error == 0) {
      device_status = response[7] & 0x0F;
    }
  } else if (strncmp(response, str_start_new_measurement, 4) == 0) {
    // response STAM <errorstatus> (0=no errors, 1=error)
    error = response[5] & 1;
    if (error == 0) {
      log_println("Measurement Started!");
    }
  } else if (strncmp(response, str_stop_current_measurement, 4) == 0) {
    // response STPM <errorstatus> (0=no errors, 1=error)
    error = response[5] & 1;
    if (error == 0) {
      log_println("Measurement Stop!");
    }
  } else if (strncmp(response, str_set_online_measurement_mode, 4) == 0) {
    // response SONL <errorstatus> (0=no errors, 1=error)
    error = response[5] & 1;
    if (error == 0) {
      log_println("Local Save Enabled!");
    }
  } else {
    return -1;
  }

  if (error != no_error) {
    return -1;
  }

  return device_status;
}

static void HandleAsyncEvents(void) {
  if (btnHomeState == BUTTON_LONG_PRESS) {
    // HandleUserInstruction("reset");
    btnHomeState = BUTTON_NO_PRESS;
  } else if (btnHomeState == BUTTON_SHORT_PRESS) {
    // HandleUserInstruction("ghome");
    btnHomeState = BUTTON_NO_PRESS;
  } else if (btnMarkState == BUTTON_LONG_PRESS) {
    // HandleUserInstruction("mark");
    btnMarkState = BUTTON_NO_PRESS;
  } else if (btnMarkState == BUTTON_SHORT_PRESS) {
    // HandleUserInstruction("gmark");
    btnMarkState = BUTTON_NO_PRESS;
  } else if (SerialPortState == SERIAL_DATA_RECEIVED) {
    HandleUserInstruction(serialBuffer);
    SerialPortState = SERIAL_PORT_IDLE;
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

static void checkHomeButton(void) {
  static bool pinStateOld = HIGH;
  static int counter = 0;
  bool pinState;

  pinState = digitalRead(HomeButtonPin);

  if (pinState == LOW) {
    if (pinStateOld != pinState) {
      if (++counter >= MS_To_Ticks(2000)) {
        btnHomeState = BUTTON_LONG_PRESS;
        pinStateOld = pinState;
        counter = 0;
      }
    }
  } else {
    if (counter >= MS_To_Ticks(100)) {
      btnHomeState = BUTTON_SHORT_PRESS;
    }
    counter = 0;
    pinStateOld = pinState;
  }
}

static void checkMarkButton(void) {
  static bool pinStateOld = HIGH;
  static int counter = 0;
  bool pinState;

  pinState = digitalRead(MarkButtonPin);
  if (pinState == LOW) {
    if (pinStateOld != pinState) {
      if (++counter >= MS_To_Ticks(2000)) {
        btnMarkState = BUTTON_LONG_PRESS;
        pinStateOld = pinState;
        counter = 0;
      }
    }
  } else {
    if (counter >= MS_To_Ticks(100)) {
      btnMarkState = BUTTON_SHORT_PRESS;
    }
    counter = 0;
    pinStateOld = pinState;
  }
}

static void checkSerialData(void) {
  static int i = 0;
  char inChar;

  while (Serial.available()) {
    inChar = Serial.read();
    if (inChar > 0) {
      if (i < 32) {
        serialBuffer[i++] = inChar;
      } else {
        i = 0;
      }
      if (inChar == '\n') {
        i = 0;
        SerialPortState = SERIAL_DATA_RECEIVED;
      }
    }
  }
}

static void jogRunMotors(void) {
  if (digitalRead(UpButtonPin) == LOW) {
    motorA.forward();
    log_println("Jog Run MotorA UP");
  } else if (digitalRead(DownButtonPin) == LOW) {
    motorA.reverse();
    log_println("Jog Run MotorA DN");
  } else if (digitalRead(HomeButtonPin) == LOW) {
    motorB.forward();
    log_println("Jog Run MotorB UP");
  } else if (digitalRead(MarkButtonPin) == LOW) {
    motorB.reverse();
    log_println("Jog Run MotorB DN");
  } else {
    motorA.stop();
    motorB.stop();
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

void log_printf(const char* format, ...) {
  static char txBuffer[64];
  va_list args;
  va_start(args, format);
  vsprintf(txBuffer, format, args);
  va_end(args);
  log_println(txBuffer);
}

/*
 * TIMER1 Prescaler Setup
 * 
 * CS12 CS11 CS10 Prescaler
 *  0    0    0      Timer Stop (No Clock)
 *  0    0    1      1
 *  0    1    0      8
 *  0    1    1      64
 *  1    0    0      256
 *  1    0    1      1024
 *  1    1    0      Ext Clock Falling Edge
 *  1    1    1      Ext Clock Rising Edge
 *
*/
static void SetupSysTickTimer(void) {
  noInterrupts();  // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 16000000 / 256 / (1000 / CLOCK_TICK_RESOLUTION);  // compare match register 16MHz/256/f
  TCCR1B |= (1 << WGM12);                                   // CTC mode
  TCCR1B |= (1 << CS12);                                    // 256 prescaler
  TCCR1B |= (0 << CS11);                                    // 256 prescaler
  TCCR1B |= (0 << CS10);                                    // 256 prescaler
  TIMSK1 |= (1 << OCIE1A);                                  // enable timer compare interrupt
  interrupts();                                             // enable all interrupts
}
