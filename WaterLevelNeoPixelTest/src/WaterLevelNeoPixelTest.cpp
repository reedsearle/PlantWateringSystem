/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/WaterLevelNeoPixelTest/src/WaterLevelNeoPixelTest.ino"
/*
 * Project       WaterLevelNeoPixelTest
 * Description:  Test relation between Water level and neo pixel
 * Author:       Reed Searle
 * Date:         18 March 2022
 */
#include "neopixel.h"
#include <math.h>

void setup();
void loop();
int changeWaterLevel (int sensPwrPin, int sensPin, int motPwrPin);
void waterPixelBlink (int levelWater);
float callSin(int amp, int deltaAmp, int period);
#line 10 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/WaterLevelNeoPixelTest/src/WaterLevelNeoPixelTest.ino"
SYSTEM_MODE(SEMI_AUTOMATIC);

const int H2OLEVELPIN = A2;  //  Water Level Sensor on Analog Pin A2
const int H2OLEVELPWR = D8;  //  Water Level Sensor Power on digital Pin D8
const int MOTORPIN    = D7;
const int PIXELPIN    = D6;  //  First NeoPixel on pin D6
const int PIXELNUM    = 1;   //  1 pixel in string

int waterLevel;

int bright;

int endTime;

//  Constructors
Adafruit_NeoPixel waterPixel(PIXELNUM, PIXELPIN, WS2812B);

void setup() {
  waterPixel.begin();
  waterPixel.show();               // Initialize all pixels to 'off'

  Serial.begin(9600);

  pinMode(H2OLEVELPIN, INPUT);
  pinMode(H2OLEVELPWR, OUTPUT);
  digitalWrite(H2OLEVELPWR, LOW);   // Drive water level sensor power to zero to limit corrosion

  pinMode(MOTORPIN, OUTPUT);
  digitalWrite(MOTORPIN, LOW);      //  Drive motor pin low to ensure motor does not run accidentally

  waterLevel = 0;
}

void loop() {
  waterLevel = changeWaterLevel(H2OLEVELPWR, H2OLEVELPIN, MOTORPIN);
  waterPixelBlink(waterLevel);

  Serial.printf("Water level: %i \n",waterLevel);
  }


int changeWaterLevel (int sensPwrPin, int sensPin, int motPwrPin) {
  int h2oLvl;
  digitalWrite(sensPwrPin, HIGH);    // Turn on water level sensor power
  digitalWrite(motPwrPin,  HIGH);    // Turn on water pump
  delay(500);                        //  DELAY here on purpose to PRECISELY control water delivery
  digitalWrite(motPwrPin,  LOW);     // Turn off water pump
  h2oLvl = analogRead(sensPin);      //  Read water level
  digitalWrite(sensPwrPin, LOW);     //  Turn off water levell sensor power to reduce galvanic corrosion
  return h2oLvl;                     //  Return water level value  
}

void waterPixelBlink (int levelWater) {
   if (levelWater < 2000) {                         //  water level empty
    bright = callSin (127, 127, 1000);              //  Set brightness to flash at 1S interval
    waterPixel.setPixelColor(0,bright,0,0);         //  Set pixel color RED
  } if (levelWater < 2150 && levelWater >= 2000) {  //  water level empty
    bright = callSin (63, 63, 3000);                //  Set brightness to flash at 5S interval
    waterPixel.setPixelColor(0,bright,bright,0);    //  Set pixel color YELLOW
  } if (levelWater < 2500 && levelWater >= 2150) {  //  water level empty
    bright = callSin (31, 31, 10000);               //  Set brightness to flash at 10S interval
    waterPixel.setPixelColor(0,0,bright,0);         //  Set pixel color GREEN
  } if (levelWater > 2500) {                        //  water level full
    waterPixel.setPixelColor(0,0,63,0);             //  Set pixel color GREEN
  }
    waterPixel.show();
}

float callSin(int amp, int deltaAmp, int period) {
  float sinWave;
  float t;

  t = millis();

  sinWave = amp * sin(2 * M_PI * t / period) + deltaAmp;
  return sinWave;
}
