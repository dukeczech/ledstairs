// PCA9685-Arduino Module Info
// In this example, we enable debug output support. If one uncomments the line below
// inside the main header file (or defines it via custom build flag), debug output
// support will be enabled and the printModuleInfo() method will become available.
// Calling this method will display information about the module itself, including
// initalized states, register values, current settings, etc. Additionally, all
// library calls being made will display internal debug information about the
// structure of the call itself.
//
// In PCA9685.h:
// // Uncomment this define to enable debug output.
// #define PCA9685_ENABLE_DEBUG_OUTPUT     1

#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController;

void setup() {
    Serial.begin(115200);

    Wire.begin();                       // Wire must be started first
    Wire.setClock(400000);              // Supported baud rates are 100kHz, 400kHz, and 1000kHz

    pwmController.printModuleInfo();
}

void loop() {
}
