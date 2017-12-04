#include <PCA9685.h>
#include <TimedAction.h>

#define LED_POWER 8
#define LED_SENSOR_DOWN 9
#define LED_SENSOR_UP 10
#define LED_ERROR 11

#define SENSOR_DOWN 2 // INT0
#define SENSOR_UP 3 // INT1

#define MANUAL_SWITCH 4

#define ACTIVE_INTERVAL 18000
#define SCAN_SPEED 1000
#define FULL_LIGHT_INTENSITY 2048

//#define DEBUG

#define NIGHT_LIGHT_ENABLED
#define NIGHT_LIGHT_INTENSITY 10

#define SLOW_START_ENABLED
#define SLOW_START_DELAY 800

enum eDirection {
  DIRECTION_UP,
  DIRECTION_DOWN
};

enum eState {
  STATE_INACTIVE = 0,
  STATE_DIRECTION_UP = 1,
  STATE_DIRECTION_DOWN = 2
};

void setup();
void loop();
void fullOn();
void fullOff();
void enableLED(int led);
void disableLED(int led);
void fadeIn(eDirection dir);
void fadeOut(eDirection dir);
static void sensorDown();
static void sensorUp();
static void checkState();

TimedAction t1 = TimedAction(ACTIVE_INTERVAL, checkState);
volatile eState state = STATE_INACTIVE;
volatile bool on = false;
volatile bool manual = false;
PCA9685 pwm;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_SENSOR_DOWN, OUTPUT);
  pinMode(LED_SENSOR_UP, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  
  pinMode(SENSOR_DOWN, INPUT);
  pinMode(SENSOR_UP, INPUT);
  pinMode(MANUAL_SWITCH, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(400000);
  
  pwm.resetDevices();
  pwm.init(0, PCA9685_MODE_OUTPUT_ONACK | PCA9685_MODE_OUTPUT_TPOLE);
  pwm.setPWMFrequency(80);

#ifdef DEBUG
  pwm.printModuleInfo();
#endif

  enableLED(LED_POWER);
  enableLED(LED_SENSOR_DOWN);
  enableLED(LED_SENSOR_UP);
  enableLED(LED_ERROR);
  
  fullOn();
  delay(250);
  fullOff();
  
  attachInterrupt(digitalPinToInterrupt(SENSOR_DOWN), sensorDown, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_UP), sensorUp, RISING);

#ifdef DEBUG
  Serial.println("Start...");
#endif

  disableLED(LED_SENSOR_DOWN);
  disableLED(LED_SENSOR_UP);
  disableLED(LED_ERROR);
  
  interrupts();
}

void loop() {

  const int sw = digitalRead(MANUAL_SWITCH);
  if(sw == LOW) {
#ifdef DEBUG
        Serial.println("Manual switch is activated");
#endif
    enableLED(LED_SENSOR_DOWN);
    enableLED(LED_SENSOR_UP);
    fullOn();
    manual = true;
  } else {
    if(manual) {
      disableLED(LED_SENSOR_DOWN);
      disableLED(LED_SENSOR_UP);
      fullOff();
      manual = false;  
    }
    
    if(!on) {
      switch(state) {
        case STATE_INACTIVE:
          break;
        case STATE_DIRECTION_UP:
#ifdef DEBUG
          Serial.println("Sensor DOWN is activated");
#endif
          if(!on) {
            fadeIn(DIRECTION_UP);
            on = true;
          }
          break;
        case STATE_DIRECTION_DOWN:
  #ifdef DEBUG
          Serial.println("Sensor UP is activated");
  #endif
          if(!on) {
            fadeIn(DIRECTION_DOWN);
            on = true;
          }
          break;
        default:
          break;
      }
    }
  
    if(on) t1.check();
  }
}

void fadeIn(eDirection dir) {
  if(dir == DIRECTION_DOWN) {
    for (int i = 0; i < 16; i++) {
      pwm.setChannelPWM(i, FULL_LIGHT_INTENSITY);
      delay(SCAN_SPEED / 16);
#ifdef SLOW_START_ENABLED
      if(i == 0) {
        delay(SLOW_START_DELAY);
      }
#endif
    }
  } else {
    for (int i = 15; i >= 0; i--) {
      pwm.setChannelPWM(i, FULL_LIGHT_INTENSITY);
      delay(SCAN_SPEED / 16);
#ifdef SLOW_START_ENABLED
      if(i == 15) {
        delay(SLOW_START_DELAY);
      }
#endif
    }
  }
}

void fullOn() {
  pwm.setAllChannelsPWM(4096);
}

void fullOff() {
  pwm.setAllChannelsPWM(0);
#ifdef NIGHT_LIGHT_ENABLED
  pwm.setChannelPWM(0, NIGHT_LIGHT_INTENSITY);
  pwm.setChannelPWM(15, NIGHT_LIGHT_INTENSITY);
#endif
}

void enableLED(int led) {
  digitalWrite(led, LOW);
}

void disableLED(int led) {
  digitalWrite(led, HIGH);
}

void fadeOut(eDirection dir) {
  if(dir == DIRECTION_DOWN) {
    for (int i = 0; i < 16; i++) {
#ifdef NIGHT_LIGHT_ENABLED
      if(i == 0 || i == 15) {
        pwm.setChannelPWM(i, NIGHT_LIGHT_INTENSITY);
        continue;
      }
#endif
      pwm.setChannelOff(i);
      delay(SCAN_SPEED / 16);
    }
  } else {
    for (int i = 15; i >= 0; i--) {
#ifdef NIGHT_LIGHT_ENABLED
      if(i == 0 || i == 15) {
        pwm.setChannelPWM(i, NIGHT_LIGHT_INTENSITY);
        continue;
      }
#endif
      pwm.setChannelOff(i);
      delay(SCAN_SPEED / 16);
    }
  }
}

static void sensorDown() {
  static unsigned long last = 0;
  unsigned long m = millis();
  if (m - last < 200) { 
    // Ignore interrupt: probably a bounce problem
  } else {
    switch(state) {
      case STATE_INACTIVE:
        enableLED(LED_SENSOR_DOWN);
        state = STATE_DIRECTION_UP;
        break;
      case STATE_DIRECTION_UP:
        break;
      case STATE_DIRECTION_DOWN:
        break;
      default:
        break;
    }
    t1.reset();
  }
  last = m;
}

static void sensorUp() {
  static unsigned long last = 0;
  unsigned long m = millis();
  if (m - last < 200) { 
    // Ignore interrupt: probably a bounce problem
  } else {
    switch(state) {
      case STATE_INACTIVE:
        enableLED(LED_SENSOR_UP);
        state = STATE_DIRECTION_DOWN;
        break;
      case STATE_DIRECTION_UP:
        break;
      case STATE_DIRECTION_DOWN:
        break;
      default:
        break;
    }
    t1.reset();
  }
  last = m;
}

static void checkState() {
  switch(state) {
    case STATE_INACTIVE:
      break;
    case STATE_DIRECTION_UP:
#ifdef DEBUG
      Serial.println("Disable UP direction");
#endif
      disableLED(LED_SENSOR_DOWN);
      fadeOut(DIRECTION_UP);
      state = STATE_INACTIVE;
      break;
    case STATE_DIRECTION_DOWN:
#ifdef DEBUG
      Serial.println("Disable DOWN direction");
#endif
      disableLED(LED_SENSOR_UP);
      fadeOut(DIRECTION_DOWN);
      state = STATE_INACTIVE;
      break;
    default:
      break;
  }

  on = false;
}
