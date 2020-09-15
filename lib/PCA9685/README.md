# PCA9685-Arduino
Arduino Library for the PCA9685 16-Channel PWM Driver Module.

**PCA9685-Arduino - Version 1.2.14**

Library to control a PCA9685 16-channel PWM driver module from an Arduino board.  
Licensed under the copy-left GNU GPL v3 license.

Created by Kasper Skårhøj, August 3rd, 2012.  
Forked by Vitska, June 18th, 2016.  
Forked by NachtRaveVL, July 29th, 2016.

This library allows communication with boards running a PCA6985 16-channel PWM driver module. It supports a wide range of available functionality, from setting the output PWM frequecy, allowing multi-device proxy addressing, and provides an assistant class for working with Servos. Newer versions should work with PlatformIO, ESP32/8266, Teensy, and others (although one might want to ensure i2c BUFFER_LENGTH is properly defined for those architectures).

The datasheet for the IC is available from <http://www.nxp.com/documents/data_sheet/PCA9685.pdf>.

## Library Setup

### Header Defines

There are several defines inside of the library's main header file that allow for more fine-tuned control of the library. You may edit and uncomment these lines directly, or supply them as a compilation flag via custom build system. While editing the main header file isn't the most ideal, it is often the easiest way when using the Arduino IDE, as it doesn't support custom build flags. Be aware that editing this header file directly will affect all projects on your system using this library.

In PCA9685.h:
```Arduino
// Uncomment this define to enable use of the software i2c library (min 4MHz+ processor required).
//#define PCA9685_ENABLE_SOFTWARE_I2C     1   // http://playground.arduino.cc/Main/SoftwareI2CLibrary

// Uncomment this define if wanting to exclude extended functionality from compilation.
//#define PCA9685_EXCLUDE_EXT_FUNC        1

// Uncomment this define if wanting to exclude ServoEvaluator assistant from compilation.
//#define PCA9685_EXCLUDE_SERVO_EVAL      1

// Uncomment this define to enable debug output.
//#define PCA9685_ENABLE_DEBUG_OUTPUT     1
```

### Library Initialization

There are several initialization mode flags exposed through this library that are used for more fine-tuned control. These flags are expected to be provided to the library's `init(...)` function, commonly called inside of the sketch's `setup()` function. These init mode flags can be treated as a bitfield, and can be bitwise-OR'ed together to combine multiple flags together. The default init mode of the library, if left unspecified, is `PCA9685_MODE_OUTDRV_TPOLE`, which seems to work for most of the PCA9685 breakouts on market, but may be incorrect for custom PCA9685 integrations.

See Section 7.3.2 of the datasheet for more details.

From PCA9685.h:
```Arduino
// Channel update strategy used when multiple channels are being updated in batch:
#define PCA9685_MODE_OCH_ONACK      (byte)0x08  // Channel updates commit after individual channel update ACK signal, instead of after full-transmission STOP signal

// Output-enabled/active-low-OE-pin=LOW driver control modes (see datasheet Table 12 and Fig 13, 14, and 15 concerning correct usage of INVRT and OUTDRV):
#define PCA9685_MODE_INVRT          (byte)0x10  // Enables channel output polarity inversion (applicable only when active-low-OE-pin=LOW)
#define PCA9685_MODE_OUTDRV_TPOLE   (byte)0x04  // Enables totem-pole (instead of open-drain) style structure to be used for driving channel output, allowing use of an external output driver
// NOTE: 1) Chipset's breakout must support this feature (most do, some don't)
//       2) When in this mode, INVRT mode should be set according to if an external N-type external driver (should use INVRT) or P-type external driver (should not use INVRT) is more optimal
//       3) From datasheet Table 6. subnote [1]: "Some newer LEDs include integrated Zener diodes to limit voltage transients, reduce EMI, and protect the LEDs, and these -MUST BE- driven only in the open-drain mode to prevent overheating the IC."

// Output-not-enabled/active-low-OE-pin=HIGH driver control modes (see datasheet Section 7.4 concerning correct usage of OUTNE):
// NOTE: Active-low-OE pin is typically used to synchronize multiple PCA9685 devices together, or as an external dimming control signal.
#define PCA9685_MODE_OUTNE_HIGHZ    (byte)0x02  // Sets all channel outputs to high-impedance state (applicable only when active-low-OE-pin=HIGH)
#define PCA9685_MODE_OUTNE_TPHIGH   (byte)0x01  // Sets all channel outputs to HIGH (applicable only when in totem-pole mode and active-low-OE-pin=HIGH)
```

## Hookup Callouts

### Servo Control

Many 180 degree controlled digital servos run on a 20ms pulse width (50Hz update frequency) based duty cycle, and do not utilize the entire pulse width for their -90/+90 degree control. Typically, 2.5% of the 20ms pulse width (0.5ms) is considered -90 degrees, and 12.5% of the 20ms pulse width (2.5ms) is considered +90 degrees. This roughly translates to raw PCA9685 PWM values of 102 and 512 (out of the 4096 value range) for -90 to +90 degree control, but may need to be adjusted to fit your specific servo (e.g. some I've tested run ~130 to ~525 for their -90/+90 degree control).

Also, please be aware that driving some servos past their -90/+90 degrees of movement can cause a little plastic limiter pin to break off and get stuck inside of the gearing, which could potentially cause the servo to become jammed and no longer function.

See the PCA9685_ServoEvaluator class to assist with calculating PWM values from Servo angle values, if you desire that level of fine tuning.

## Memory Callouts

### Extended Functions

This library has an extended list of functionality for those who care to dive into such, but isn't always particularly the most useful for various general use cases. If one uncomments the line below inside the main header file (or defines it via custom build flag), this extended functionality can be manually compiled-out.

In PCA9685.h:
```Arduino
// Uncomment this define if wanting to exclude extended functionality from compilation.
#define PCA9685_EXCLUDE_EXT_FUNC        1
```

## Example Usage

Below are several examples of library usage.

### Simple Example

```Arduino
#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController;                  // Library using default Wire and default linear phase balancing scheme

void setup() {
    Serial.begin(115200);

    Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.resetDevices();       // Software resets all PCA9685 devices on Wire line

    pwmController.init(B000000);        // Address pins A5-A0 set to B000000, default mode settings
    pwmController.setPWMFrequency(100); // Default is 200Hz, supports 24Hz to 1526Hz

    pwmController.setChannelPWM(0, 128 << 4); // Set PWM to 128/255, but in 4096 land

    Serial.println(pwmController.getChannelPWM(0)); // Should output 2048, which is 128 << 4
}

```

### Batching Example

In this example, we randomly select PWM frequencies on all 12 outputs and allow them to drive for 5 seconds before changing them.

```Arduino
#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController;                  // Library using default Wire and default linear phase balancing scheme

void setup() {
	Serial.begin(115200);

    Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.resetDevices();       // Software resets all PCA9685 devices on Wire line

    pwmController.init(B010101);        // Address pins A5-A0 set to B010101, default mode settings
    pwmController.setPWMFrequency(500); // Default is 200Hz, supports 24Hz to 1526Hz

    randomSeed(analogRead(0));          // Use white noise for our randomness
}

void loop() {
    word pwms[12];
    pwms[0] = random(0, 4096);
    pwms[1] = random(0, 4096);
    pwms[2] = random(0, 4096);
    pwms[3] = random(0, 4096);
    pwms[4] = random(0, 4096);
    pwms[5] = random(0, 4096);
    pwms[6] = random(0, 4096);
    pwms[7] = random(0, 4096);
    pwms[8] = random(0, 4096);
    pwms[9] = random(0, 4096);
    pwms[10] = random(0, 4096);
    pwms[11] = random(0, 4096);
    pwmController.setChannelsPWM(0, 12, pwms);
    delay(5000);

    // Note: Only 7 channels can be written in one i2c transaction due to a
    // BUFFER_LENGTH limit of 32, so 12 channels will take two i2c transactions.
}

```

### Multi-Device Proxy Example

In this example, we use a special instance to control other modules attached to it via the ALL_CALL register.

```Arduino
#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController1;                 // Library using default Wire and default linear phase balancing scheme
PCA9685 pwmController2;                 // Library using default Wire and default linear phase balancing scheme

PCA9685 pwmControllerAll;               // Not a real device, will act as a proxy to pwmController1 and pwmController2

void setup() {
    Serial.begin(115200);

    Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController1.resetDevices();      // Software resets all PCA9685 devices on Wire line (including pwmController2 in this case)

    pwmController1.init(B000000);       // Address pins A5-A0 set to B000000, default mode settings
    pwmController2.init(B000001);       // Address pins A5-A0 set to B000001, default mode settings

    pwmController1.setChannelOff(0);    // Turn channel 0 off
    pwmController2.setChannelOff(0);    // On both

    pwmController1.enableAllCallAddress(); // Default address of 0xE0
    pwmController2.enableAllCallAddress(); // Same default address

    pwmControllerAll.initAsProxyAddresser(); // Same default address of 0x0E as used in enable above

    pwmControllerAll.setChannelPWM(0, 4096); // Enables full on on both pwmController1 and pwmController2

    Serial.println(pwmController1.getChannelPWM(0)); // Should output 4096
    Serial.println(pwmController2.getChannelPWM(0)); // Should also output 4096

    // Note: Various parts of functionality of the proxy class instance are actually
    // disabled - typically anything that involves a read command being issued.
}

```

### Servo Evaluator Example

In this example, we utilize the ServoEvaluator class to assist with setting PWM frequencies when working with servos.

We will be using Wire1, which is only available on boards with SDA1/SCL1 (Due, Zero, etc.) - change to Wire if Wire1 is unavailable.

```Arduino
#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController(Wire1, PCA9685_PhaseBalancer_Weaved); // Library using Wire1 and weaved phase balancing scheme

// Linearly interpolates between standard 2.5%/12.5% phase length (102/512) for -90°/+90°
PCA9685_ServoEvaluator pwmServo1;

// Testing our second servo has found that -90° sits at 128, 0° at 324, and +90° at 526.
// Since 324 isn't precisely in the middle, a cubic spline will be used to smoothly
// interpolate PWM values, which will account for said discrepancy. Additionally, since
// 324 is closer to 128 than 526, there is less resolution in the -90° to 0° range, and
// more in the 0° to +90° range.
PCA9685_ServoEvaluator pwmServo2(128,324,526);

void setup() {
    Serial.begin(115200);

    Wire1.begin();                      // Wire must be started first
    Wire1.setClock(400000);             // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.resetDevices();       // Software resets all PCA9685 devices on Wire line

    pwmController.init(B000000);        // Address pins A5-A0 set to B000000, default mode settings
    pwmController.setPWMFrequency(50);  // 50Hz provides 20ms standard servo phase length

    pwmController.setChannelPWM(0, pwmServo1.pwmForAngle(-90));
    Serial.println(pwmController.getChannelPWM(0)); // Should output 102 for -90°

    // Showing linearity for midpoint, 205 away from both -90° and 90°
    Serial.println(pwmServo1.pwmForAngle(0));   // Should output 307 for 0°

    pwmController.setChannelPWM(0, pwmServo1.pwmForAngle(90));
    Serial.println(pwmController.getChannelPWM(0)); // Should output 512 for +90°

    pwmController.setChannelPWM(1, pwmServo2.pwmForAngle(-90));
    Serial.println(pwmController.getChannelPWM(1)); // Should output 128 for -90°

    // Showing less resolution in the -90° to 0° range
    Serial.println(pwmServo2.pwmForAngle(-45)); // Should output 225 for -45°, 97 away from -90°

    pwmController.setChannelPWM(1, pwmServo2.pwmForAngle(0));
    Serial.println(pwmController.getChannelPWM(1)); // Should output 324 for 0°

    // Showing more resolution in the 0° to +90° range
    Serial.println(pwmServo2.pwmForAngle(45));  // Should output 424 for +45°, 102 away from +90°

    pwmController.setChannelPWM(1, pwmServo2.pwmForAngle(90));
    Serial.println(pwmController.getChannelPWM(1)); // Should output 526 for +90°
}

```

### Software I2C Example

In this example, we utilize the software I2C functionality for chips that do not have a hardware I2C bus.

If one uncomments the line below inside the main header file (or defines it via custom build flag), software I2C mode for the library will be enabled.

In PCA9685.h:
```Arduino
// Uncomment this define to enable use of the software i2c library (min 4MHz+ processor required).
#define PCA9685_ENABLE_SOFTWARE_I2C     1   // http://playground.arduino.cc/Main/SoftwareI2CLibrary
```

In main sketch:
```Arduino
#include "PCA9685.h"

#define SCL_PIN 2                       // Setup defines are written before library include
#define SCL_PORT PORTD 
#define SDA_PIN 0 
#define SDA_PORT PORTC 

#if F_CPU >= 16000000
#define I2C_FASTMODE 1                  // Running a 16MHz processor allows us to use I2C fast mode
#endif

#include "SoftI2CMaster.h"              // Include must come after setup defines

PCA9685 pwmController;                  // Library using default linear phase balancing scheme

void setup() {
	Serial.begin(115200);

    i2c_init();                         // Software I2C must be started first

    pwmController.resetDevices();       // Software resets all PCA9685 devices on software I2C line

    pwmController.init(B000000);        // Address pins A5-A0 set to B000000, default mode settings

    pwmController.setChannelPWM(0, 2048); // Should see a 50% duty cycle along the 5ms phase width
}

```

## Module Info

In this example, we enable debug output support.

If one uncomments the line below inside the main header file (or defines it via custom build flag), debug output support will be enabled and the printModuleInfo() method will become available. Calling this method will display information about the module itself, including initalized states, register values, current settings, etc. Additionally, all library calls being made will display internal debug information about the structure of the call itself. An example of this output is shown below.

In PCA9685.h:
```Arduino
// Uncomment this define to enable debug output.
#define PCA9685_ENABLE_DEBUG_OUTPUT       1
```

In main sketch:
```Arduino
#include "PCA9685.h"

PCA9685 pwmController;

void setup() {
    Serial.begin(115200);

    Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.printModuleInfo();
}

```

In serial monitor:
```
  ~~~ PCA9685 Module Info ~~~

i2c Address:
0x40

Phase Balancer:
PCA9685_PhaseBalancer_Linear

Proxy Addresser:
false

Mode1 Register:
  PCA9685::readRegister regAddress: 0x0
    PCA9685::readRegister retVal: 0x20
0x20, Bitset: PCA9685_MODE_AUTOINC

Mode2 Register:
  PCA9685::readRegister regAddress: 0x1
    PCA9685::readRegister retVal: 0x0
0x0, Bitset:

SubAddress1 Register:
  PCA9685::readRegister regAddress: 0x2
    PCA9685::readRegister retVal: 0xE2
0xE2

SubAddress2 Register:
  PCA9685::readRegister regAddress: 0x3
    PCA9685::readRegister retVal: 0xE4
0xE4

SubAddress3 Register:
  PCA9685::readRegister regAddress: 0x4
    PCA9685::readRegister retVal: 0xE8
0xE8

AllCall Register:
  PCA9685::readRegister regAddress: 0x5
    PCA9685::readRegister retVal: 0xE0
0xE0

```
