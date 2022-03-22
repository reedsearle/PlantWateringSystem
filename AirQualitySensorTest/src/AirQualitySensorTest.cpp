/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/AirQualitySensorTest/src/AirQualitySensorTest.ino"
/*
 * Project AirQualitySensorTest
 * Description:
 * Author:
 * Date:
 */

#include "Air_Quality_Sensor.h"
void setup();
void loop();
#line 9 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/AirQualitySensorTest/src/AirQualitySensorTest.ino"
SYSTEM_MODE(SEMI_AUTOMATIC);
const int AQSPIN = A0;

int currentQuality;

// Constructor
AirQualitySensor airQualitySensor(AQSPIN);

void setup()
{
    Serial.begin(9600);
    currentQuality =-1;
    airQualitySensor.init();
}
void loop()
{
    currentQuality=airQualitySensor.slope();

    switch (currentQuality) {
      case 0:
            Serial.println("High pollution! Force signal active");
      break;
      
      case 1:
            Serial.println("High pollution!");
      break;
      
      case 2:
            Serial.println("Low pollution!");
      break;
      
      case 3:
            Serial.println("Fresh air");
      break;      
    }

 }
// ISR(TIMER2_OVF_vect)  //  Need to determine how to use this vector
// {
//     if(airqualitysensor.counter==122)//set 2 seconds as a detected duty
//     {
//         airqualitysensor.last_vol=airqualitysensor.first_vol;
//         airqualitysensor.first_vol=analogRead(A0);
//         airqualitysensor.counter=0;
//         airqualitysensor.timer_index=1;
//         PORTB=PORTB^0x20;
//     }
//     else
//     {
//         airqualitysensor.counter++;
//     }
// }