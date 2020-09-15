# LED-stairs
Arduino project for automatic stairs LED lightning. It's activated by a human motion on both ends of the stairs. It supports various effects:
* Night inactivity light - first and last strip shine at very low intensity to help people with orientation
* Fade in/Fade out effect - strips are activated with a little delay one-by-one from the direction that the motion occured
* Activation time extending - while LED strips are already activated by a sensor, every other sensor signal extend the actiovation time to default (LED will never goes off while sensors are constantly activated)
* Default shine intensity - support interval level from 0 to 2048 (0% fully off to 100% fully on state)

# Prerequisites
* PCA9685 16-Channel PWM Driver Module Library (https://github.com/NachtRaveVL/PCA9685-Arduino)
* TimedAction Library (https://playground.arduino.cc/Code/TimedAction)

# Main modules
* 16x 60cm 12V 3528 LED strip (warm white, non waterproof)
* 2x 12V Infrared PIR motion automatic sensor switch
* 1x DIY MOSFET LED driver (16x BS170 MOSFET transistor)
* 1x PCA9685 16-channel 12-bit PWM driver I2C module
* 1x Arduino Nano V3.0 ATmega328P CH340G 5V 16MHz micro-controller board


# Remote commands
There are a couple of remote commands sent via serial line to set the controller. Just connect the controller with USB port and press Enter into a serial console.
Available commands:
```
setInterval(x); - set delay interval in seconds
setLED(x); - set period for blinking LED in seconds
setDefaults(); - set all the parametres to default values
sensorUp(); - activate the UP sensor
sensorDown(); - activate the DOWN sensor
reset(); - reset the controller
```

# Photo
![Main modules together](https://github.com/dukeczech/LED-stairs/blob/master/20171013_171300.jpg)

![LED stairs](https://github.com/dukeczech/LED-stairs/blob/master/20171013_171417.jpg)

![Indoor box opened](https://github.com/dukeczech/LED-stairs/blob/master/24650949_10204163787930488_1215584388_o.jpg)

![Indoor box closed](https://github.com/dukeczech/LED-stairs/blob/master/24819211_10204163787890487_510508127_o.jpg)
