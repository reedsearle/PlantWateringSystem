/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/SoilExperiment/src/SoilExperiment.ino"
/*
 * Project      SoilExperiment
 * Description: MEasure the diffusion of water into both the air and the plant over a weekend
 * Author:      Reed Searle
 * Date:        17 March 2022
 */

void setup();
void loop();
void write_SFFII_ToSD(String item1, float item2, float item3, int item4, int item5);
void write_SFFII_ToOLED(String item1, float item2, float item3, int item4, int item5);
#line 8 "c:/Users/reed_/Documents/IoT/PredictiveWateringSystem/SoilExperiment/src/SoilExperiment.ino"
SYSTEM_MODE(SEMI_AUTOMATIC);

#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include <Adafruit_BME280.h>
#include <SPI.h>
#include "SdFat.h"

//****************************
// Constants
//****************************
  const int SCREENWIDTH  = 128;    //  Width of screen in pixels
  const int SCREENHEIGHT = 64;     //  Height of screen in pixels
  const int OLEDRESET    = -1;     //  OLED Reset shared with Teensy
  byte      OLEDADDRESS  = 0x3C;   //  Address of the OLED on the IIC bus
  byte      BMEADDRESS   = 0x76; //  Address of the OLED on the IIC bus
  const int SAMPLETIME   = 60000;  //  Sample rate = 1 min
  const int MOISTPIN     = A1;     //  Moisture sensor is on Pin  A0
  const int MOINSTINITH  = 0;      //  Reset value for moistureH variable
  const int MOINSTINITL  = 4096;   //  Reset value for moistureL variable (maximum value of analogRead)
  const int SD_CS_PIN    = SS;     //  uSD chip select on the SS pin

  
//****************************
// Variables
//****************************
  //  Time & timing Variables
  String     dateTime, timeOnly;   
  int endTime;

  //  BME280 Variables
  float tempC;
  float tempF;
  float pressPA;
  float pressInHg;
  float humidRH;

  // Moisture Sensor Variables 
  int moisture;
  int moistureH;
  int moistureL;
  float temp, humid;

  char lastName[]  = "Searle"; // ???
  char firstName[] = "Reed";

  //  uSD Card Variables
  File testFile;   //  **** Change this after unit testing
  int status, toldYa;  //  Setup variables for uSD

//****************************
// Constructors
//****************************
  Adafruit_SSD1306 displayOne(OLEDRESET);
  Adafruit_BME280    bmeOne;
  SdFat SD;


//****************************
//****************************
//      SETUP
//****************************
//****************************  
void setup() {
  Serial.begin(9600);
  while(!Serial) {}
//  Initialize the BME280 
  status = bmeOne.begin(BMEADDRESS);
  if(!status){
    Serial.printf("BME 280 did not initialize correctly.  Please reset.\n");
    while(1);  //  You shall not pass 
  }
  Serial.printf("BME 280 running\n");


//  Initialize the uSD Card
  // initialize uSD card module CS to off
  pinMode     (SD_CS_PIN,OUTPUT); 
  digitalWrite(SD_CS_PIN,HIGH);

  toldYa = 0;
  status = 0;
  while (!status) {
    status = SD.begin(SD_CS_PIN);
    if (!status && !toldYa) {  // if status is false
      Serial.printf("Card failed, or not present\n");
      toldYa = 1;
    }
    else if (status){
      Serial.printf("uSD card initialized.\n");
    }
  }
    //  Print file headers to uSD
    testFile = SD.open("testFile.csv", FILE_WRITE);
    if (testFile) {
      testFile.printf("Time, Temp, Pres, MoistH, MoistL \n");
      testFile.close();
      Serial.printf("Time, Temp, Pres, MoistH, MoistL \n");
    }

  Time.zone(-6);                  //  Set time zone to MDT -6 from UTC
  Particle.syncTime();

  pinMode(MOISTPIN, INPUT);       //  Moisture pin is an input
  

  //  Setup OLED
  displayOne.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);  // initialize with the I2C addr above
  displayOne.setTextSize(1);
  displayOne.setTextColor(WHITE);
  displayOne.clearDisplay();//  Clear the display before going further
  displayOne.display(); // Force display

  endTime = millis();
  moistureH = MOINSTINITH;  //  Reset moisture high & low indicators
  moistureL = MOINSTINITL;
}




//****************************
//****************************
//      LOOP
//****************************
//****************************
void loop() {
  //  Get current time
  dateTime = Time.timeStr();                          //  get current value of date and time
  timeOnly = dateTime.substring(11,19);               //  Extract value of time from dateTime

  //  Get current temp & humidity
  temp  = bmeOne.readTemperature();
  humid = bmeOne.readHumidity();

  //  Get current moisture levels
  moisture = analogRead(MOISTPIN);
  if (moisture > moistureH) {
    moistureH = moisture;
  }
  if (moisture < moistureL) {
    moistureL = moisture;
  }

  //  Write values to OLED coninuously
  write_SFFII_ToOLED(timeOnly.c_str(), temp, humid, moistureH, moistureL);

  //  SAMPLE DATA once per minute
  if (millis() - endTime > SAMPLETIME) {
    //  Write to the uSD card
    write_SFFII_ToSD(timeOnly.c_str(), temp, humid, moistureH, moistureL);
    //  Reset v ariables forthe next sample
    moistureH = MOINSTINITH;  //  Reset moisture high & low indicators
    moistureL = MOINSTINITL;
    endTime = millis();
  }
}




//****************************
//****************************
//      write_SFFII_ToSD
//****************************
//****************************
void write_SFFII_ToSD(String item1, float item2, float item3, int item4, int item5) {

  testFile = SD.open("testFile.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (testFile) {
    testFile.printf("%s, %0.2f, %0.2f, %i, %i \n", item1.c_str(), item2, item3, item4, item5);
    testFile.close();
    Serial.printf("%s, %0.2f, %0.2f, %i, %i \n", item1.c_str(), item2, item3, item4, item5);
  }  
  else {
    Serial.printf("error opening testFile.csv \n");
  }
  return;
}




//****************************
//****************************
//      write_SFFII_ToOLED
//****************************
//****************************
void write_SFFII_ToOLED(String item1, float item2, float item3, int item4, int item5) {
    // Write to the OLED display
    displayOne.clearDisplay();//  Clear the display before going further
    displayOne.drawRect(0,0,SCREENWIDTH,SCREENHEIGHT,WHITE);
    displayOne.setCursor(7,5);
    displayOne.printf("    Time is: %s\n", item1.c_str());
    displayOne.setCursor(7,15);
    displayOne.printf("Temperature: %0.2f\n", item2);
    displayOne.setCursor(7,25);
    displayOne.printf("   Humidity: %0.2f\n", item3);
    displayOne.setCursor(7,35);
    displayOne.printf("Moisture Hi: %i\n", item4);
    displayOne.setCursor(7,45);
    displayOne.printf("Moisture Lo: %i\n", item5);
    displayOne.display(); // Force display
}