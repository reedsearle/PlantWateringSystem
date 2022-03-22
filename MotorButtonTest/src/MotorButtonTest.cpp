/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/MotorButtonTest/src/MotorButtonTest.ino"
/*
 * Project MotorButtonTest
 * Description: tests input button and output to motor through relay
 * Author:      Reed Searle
 * Date:        18 March 2022
 */

#include <math.h>

void setup();
void loop();
#line 10 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/MotorButtonTest/src/MotorButtonTest.ino"
SYSTEM_MODE(SEMI_AUTOMATIC);

const int BUTTONPIN = D4;
const int MOTORPIN  = A3;

int button;
int motor;

int       angle;            // angle for servo output
float     period;           // Period of the resulting SIN wave
float     currentTime;                // Time in Seconds
float     endLoopTime;                // Time in Seconds
float     deltaTime;                // Time in Seconds
float     servoTime;                // Time in Seconds
int       offsetAmp;        // Positive offset of SIN wave amplitude
int       amp;              // amplitude of SIN wave


void setup() {
  pinMode (BUTTONPIN, INPUT);
  pinMode (MOTORPIN, OUTPUT);

  period            = 5000.0;       // Set Flash rate to 5000 milliseconds
  amp               = 127;           // Set amplitude to 90
  offsetAmp         = 127;           // Set amplitude offset to 90 for full range
  endLoopTime       = millis();     // get current time and convert into seconds


}

void loop() {
  currentTime = millis();                                      // get current time and convert into seconds
  deltaTime = currentTime - endLoopTime;

  button = digitalRead(BUTTONPIN);

  if (button == 1) {
    servoTime += deltaTime;
    angle = amp * sin(2*M_PI*servoTime/period) + offsetAmp;  //  determine the current value of angle
    analogWrite(MOTORPIN, angle);
  } else {
    digitalWrite(MOTORPIN, LOW);
  }
 endLoopTime = currentTime;

}