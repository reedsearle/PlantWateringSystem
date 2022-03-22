/*
 * Project       DustSensorTest
 * Description:  Test the interface to the dust sensor
 * Author:       Reed Searle
 * Date:         18 March 2022
 */

#include <math.h>
SYSTEM_MODE(SEMI_AUTOMATIC);
const int DUSTPIN    = D5;      // Dust sensor Pin
const int SAMPLETIME = 30000;   // sample time of 30 sec
const int DUSTTO     = 100;     // Sample time out = 100uS

  //  Dust Sensor variables
  unsigned int duration;                 // duration that the sensor pin is low during a given sample - uS
  int          lowPulseOccupancy;        // Duration the pin is low for the sample time of 30 sec - uS
  float        ratio;                    // Ration of LPO time to total time
  float        concentration;            // Concentration of dut in sample
  bool         dustFlag;                 // Flag for pulse start
  int          dustTime;                 //  Time the current pulse started
  int          throwAwayOld;             //  edge detection
  int          dustStart;                //  Start the wait-for-pulse timer
  int          throwAway;                //  level detection
  int          sampleStart;
  



void setup() {
  Serial.begin(9600);
  sampleStart = millis();

  pinMode(DUSTPIN, INPUT);

  lowPulseOccupancy = 0;        //  Initialize all variables
  ratio             = 0;
  concentration     = 0;  
}

void loop() {
  //  Get current Dust levels
  duration     = 0;                                    //  Reset duration timer
  dustFlag     = 0;                                    //  Pulse has not started
  dustTime     = 0;                                    //  Time the current pulse started
  throwAwayOld = digitalRead(DUSTPIN);                 //  Set edge detection
  dustStart    = micros();                             //  Start the wait-for-pulse timer
  while(micros() - dustStart < DUSTTO && !dustFlag) {  //  Start looking for pulse. timeout after 1000uS
    throwAway = digitalRead(DUSTPIN);                  //  Read the Dust Sensor in put pin. value irrelevant at this time
    if (throwAway == 0 && throwAway != throwAwayOld){  //  Low pulse detected and edge detected.
       dustFlag = 1;                                   //  Pulse has started set flag
       dustTime = micros();                            //  Set Time the current pulse started
     } else {                                          //  No pulse detected
       throwAwayOld = throwAway;                       //  Update edge detector
       dustStart = micros();                           //  Increment timeout timer
     }
  }
  while(throwAway == 0 && dustFlag) {                  //  Measure length of low pulse ONLY IF pulse was discovered during timeout interval
    duration = micros() - dustTime;                    //  Measure length of pulse through time difference
    throwAway = digitalRead(DUSTPIN);                  //  ReCheck for end of pulse
  }
//  duration = pulseIn(DUSTPIN, LOW);                   //  Read the input pin for low pulses
  
  lowPulseOccupancy += duration;                       //  summ the total amount of low time
  if (millis() - sampleStart > SAMPLETIME) {           //  Once the air has been samples for 30 sec
    ratio = lowPulseOccupancy / (SAMPLETIME * 10.0);   // calculate the ratio
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) +520.0 * ratio + 0.62;  //  Calculate the concentration
    Serial.printf("LPO %i, concen %0.2f, ratio %0.2f\n", lowPulseOccupancy, concentration, ratio);
    lowPulseOccupancy = 0;                             // reset the LPO
    sampleStart = millis();                            // Reset the timer
  }

}