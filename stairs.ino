#include <PCA9685.h>
#include <TimedAction.h>
#include <EEPROM.h>

#define LED_POWER 8
#define LED_SENSOR_DOWN 9
#define LED_SENSOR_UP 10
#define LED_ERROR 11

#define LED_OUTPUT1 5

#define SENSOR_DOWN 2 // INT0
#define SENSOR_UP 3 // INT1

#define MANUAL_SWITCH 4

#define ACTIVE_INTERVAL 20000
#define LED_PERIOD_INTERVAL 1000
#define SCAN_SPEED 1000
#define FULL_LIGHT_INTENSITY 2048

#define DEBUG 1

#define NIGHT_LIGHT_ENABLED
#define NIGHT_LIGHT_INTENSITY 10

#define SLOW_START_ENABLED
#define SLOW_START_DELAY 800

#define LED_OUTPUT1_ENABLE 1

#define EEPROM_VARIABLE_DELAY_INTERVAL 0
#define EEPROM_VARIABLE_LED_PERIOD 1

enum eDirection {
  DIRECTION_UP,
  DIRECTION_DOWN
};

enum eState {
  STATE_INACTIVE = 0,
  STATE_DIRECTION_UP = 1,
  STATE_DIRECTION_DOWN = 2
};

void reset();
void setup();
void loop();
void checkSerial();
void fullOn();
void fullOff();
void enableLED(int led);
void disableLED(int led);
void enableOutput1();
void disableOutput1();
void fadeIn(eDirection dir);
void fadeOut(eDirection dir);
static void sensorDown();
static void sensorUp();
static void checkState();
static void blink();

int activeInterval = ACTIVE_INTERVAL;
int outputPeriod1 = LED_PERIOD_INTERVAL;

TimedAction t1 = TimedAction(0, NULL);
TimedAction t2 = TimedAction(0, NULL);
volatile eState state = STATE_INACTIVE;
volatile bool on = false;
volatile bool manual = false;
PCA9685 pwm;

void reset()
{
  Serial.println("Reseting...");
  Serial.flush();
  delay(100);
  asm volatile("  jmp 0");  
}  

void setup() {  
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_SENSOR_DOWN, OUTPUT);
  pinMode(LED_SENSOR_UP, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

#ifdef LED_OUTPUT1_ENABLE
  pinMode(LED_OUTPUT1, OUTPUT);
#endif

  pinMode(SENSOR_DOWN, INPUT);
  pinMode(SENSOR_UP, INPUT);
  pinMode(MANUAL_SWITCH, INPUT_PULLUP);

  Wire.begin();
  Wire.setClock(400000);
  
  pwm.resetDevices();
  pwm.init(0, PCA9685_MODE_OCH_ONACK | PCA9685_MODE_OUTDRV_TPOLE);
  pwm.setPWMFrequency(80);

  enableLED(LED_POWER);
  enableLED(LED_SENSOR_DOWN);
  enableLED(LED_SENSOR_UP);
  enableLED(LED_ERROR);
  
  fullOn();
#ifdef LED_OUTPUT1_ENABLE
  enableOutput1();
#endif
  delay(250);
  fullOff();
#ifdef LED_OUTPUT1_ENABLE
  disableOutput1();
#endif

  noInterrupts();
  
  attachInterrupt(digitalPinToInterrupt(SENSOR_DOWN), sensorDown, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_UP), sensorUp, RISING);

  disableLED(LED_SENSOR_DOWN);
  disableLED(LED_SENSOR_UP);
  disableLED(LED_ERROR);

  if(EEPROM.read(EEPROM_VARIABLE_DELAY_INTERVAL) > 0) {
    activeInterval = EEPROM.read(EEPROM_VARIABLE_DELAY_INTERVAL) * 1000;
  } 
  if(EEPROM.read(EEPROM_VARIABLE_LED_PERIOD) > 0) {
    outputPeriod1 = EEPROM.read(EEPROM_VARIABLE_LED_PERIOD) * 1000;
  } 
  
  t1 = TimedAction(activeInterval, checkState);
  t2 = TimedAction(outputPeriod1, blink);

  delay(500);
  
  interrupts();

#ifdef DEBUG
  Serial.println("Start...");
#endif
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
#ifdef LED_OUTPUT1_ENABLE
    //enableOutput1();
#endif
    manual = true;
  } else {
    if(manual) {
      disableLED(LED_SENSOR_DOWN);
      disableLED(LED_SENSOR_UP);
      fullOff();
#ifdef LED_OUTPUT1_ENABLE
      //disableOutput1();
#endif
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

  t2.check();

  checkSerial();
}

void checkSerial() {
  String command = "";
  byte index = 0;
  boolean started = false;
  boolean ended = false;

  while(Serial.available() > 0) {
    byte someByte = Serial.read();
    delay(10);
    if(started && !ended) {
      if(someByte == ';') {
        ended = true;
      } else {
        index++;
        command.concat((char) someByte);
      }
    } else if(!started && !ended) {
      if(someByte == 's' || someByte == 'r') {
        index = 0;
        command.concat((char) someByte);
        started = true;
      } else if(someByte == 0x0A) {
        Serial.println("Available commands:");
        Serial.println(" setInterval(x); - set delay interval in seconds");
        Serial.println(" setLED(x); - set period for blinking LED in seconds");
        Serial.println(" setDefaults(); - set all the parametres to default values");
        Serial.println(" sensorUp(); - activate the UP sensor");
        Serial.println(" sensorDown(); - activate the DOWN sensor");
        Serial.println(" reset(); - reset the controller");
      }
    } 
    if(started && ended) {
      int first = command.indexOf('(');
      int last = command.lastIndexOf(')');
      long value = command.substring(first + 1, last).toInt();
        
      if(command.startsWith("setInterval")) {
        EEPROM.write(EEPROM_VARIABLE_DELAY_INTERVAL, (int) value);
#ifdef DEBUG
        Serial.print("Command setInterval with value ");
        Serial.print(value);
        Serial.println(" saved into memory");
#endif
        reset();
      } else if(command.startsWith("setLED")) {
        EEPROM.write(EEPROM_VARIABLE_LED_PERIOD, (int) value);
#ifdef DEBUG
        Serial.print("Command setLED with value ");
        Serial.print(value);
        Serial.println(" saved into memory");
#endif
        reset();
      } else if(command.startsWith("setDefaults")) {
        EEPROM.write(EEPROM_VARIABLE_DELAY_INTERVAL, ACTIVE_INTERVAL / 1000);
        EEPROM.write(EEPROM_VARIABLE_LED_PERIOD, LED_PERIOD_INTERVAL / 1000);
#ifdef DEBUG
        Serial.println("Command setDefaults executed");
#endif
        reset();
      } else if(command.startsWith("sensorUp")) {
        sensorDown();
#ifdef DEBUG
        Serial.println("Command sensorUp executed");
#endif
      } else if(command.startsWith("sensorDown")) {
        sensorUp();
#ifdef DEBUG
        Serial.println("Command sensorDown executed");
#endif
      } else if(command.startsWith("reset")) {
#ifdef DEBUG
        Serial.println("Command reset executed");
#endif
        reset();
      } else {
        Serial.print("Unknown command ");
        Serial.println(command);
      }
      
      started = false;
      ended = false;      
      command = "";
    }
  }
} 

void fadeIn(eDirection dir) {
#ifdef LED_OUTPUT1_ENABLE
  enableOutput1();
#endif
  if(dir == DIRECTION_DOWN) {
#ifdef DEBUG
    Serial.print("Enable DOWN direction for ");
    Serial.print(activeInterval);
    Serial.println("ms");
#endif
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
#ifdef DEBUG
    Serial.print("Enable UP direction for ");
    Serial.print(activeInterval);
    Serial.println("ms");
#endif
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

void enableOutput1() {
  for (int i = 0; i <= 255; i+=5) {
    analogWrite(LED_OUTPUT1, i);
    delay(6);
  }
}

void disableOutput1() {
  for (int i = 255; i >= 0; i-=5) {
    analogWrite(LED_OUTPUT1, i);
    delay(6);
  }
}

void fadeOut(eDirection dir) {
#ifdef LED_OUTPUT1_ENABLE
  disableOutput1();
#endif
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

static void blink() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
