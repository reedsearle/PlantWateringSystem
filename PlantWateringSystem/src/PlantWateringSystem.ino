/*
 * Project      Plant Watering System
 * Description: Maintain moisture level in a plant and measure environmental variables
 * Author:      Reed Searle
 * Date:        21 March 2022
 */

// SYSTEM_MODE(SEMI_AUTOMATIC);

#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "creds.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include <Adafruit_BME280.h>
#include <SPI.h>
#include "SdFat.h"
#include "neopixel.h"
#include <math.h>
#include "Air_Quality_Sensor.h"


//**************************************
// Global State - DO NOT CHANGE THIS
//**************************************
TCPClient TheClient; 

//****************************
// Constants
//****************************
  //  Pin Assignments
  const int SD_CS_PIN    = SS;       //  uSD chip select on the SS pin
  const int AQSPIN       = A0;       //  Air Quality sensor on analog pin A0
  const int MOISTPIN     = A1;       //  Moisture sensor is on Pin  A1
  // const int H2OLEVELPIN  = A2;       //  Water Level Sensor on Analog Pin A2
  // const int MOTORPIN     = D8;       //  Motor relay on digital pin D8
  // const int H2OLEVELPWR  = D7;       //  Water Level Sensor Power on digital Pin D7
  const int PIXELPIN     = D6;       //  First NeoPixel on pin D6
  const int DUSTPIN      = D5;       //  Dust sensor on digital Pin D5
  const int BUTTONPIN    = D4;       //  Button on digital Pin D4
 
  //  I2C Addresses
  byte      OLEDADDRESS  = 0x3C;     //  Address of the OLED on the IIC bus
  byte      BMEADDRESS   = 0x76;     //  Address of the OLED on the IIC bus

  const int SCREENWIDTH  = 128;      //  Width of screen in pixels
  const int SCREENHEIGHT = 64;       //  Height of screen in pixels
  const int OLEDRESET    = -1;       //  OLED Reset shared with Teensy
  const int WATERTIME    = 1800000;  //  Sample rate = 30 min
  const int MOISTSET     = 1900;     //  Moisture set point
  const int PIXELNUM     = 1;        //  1 pixel in string
  const int SAMPLETIME   = 30000;    // sample time of 30 sec
  const int PINGTIME     = 120000;   // MQTT Ping time of 2 min
  const int PUBLISHTIME  = 30000;    // Publish time of 30 sec
  const int DUSTTO       = 50;       // Timeout for dust sensor PULSEIN() read - 50uS
 

//****************************
// Variables
//****************************
  //  Time & timing Variables
  String     dateTime, timeOnly, timeOnlyOld;   
  int        endTime;
  int        waterTime;
  int        publishTime;
  int        pingTime;
  int        sampleStart;              // Timer for Air Quality sample
  int        BMEStart;                 // Timer for BME280 sample


  //  BME280 Variables
  float      tempC;
  float      tempF;
  float      pressPA;
  float      pressInHg;
  float      humidRH;
  int        status;                     //  Setup variables for BME280 & uSD

  // Moisture Sensor Variables 
  int        moisture;
  bool       feedMe;                    //  Boolean for Publish to indicate through Adafruit that the plant needs water

  //  uSD Card Variables
  File       testFile;                  //  **** Change this after unit testing

  //  Water Level variables
  int        waterLevel;                //  Input from water level sensor
  bool       fillMe;                    //  Boolean to Zapier to indicate reservoir is almost empty

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

  //  Air Quality Sensor variables
  int        currentQuality;
  String     AQString;

  //  Subscribe variables
  int        subBut;                     //  Temporary integer holder for value from subscribe
  bool       subscribeButton;            //  Boolean for water button on Adafruit

  //  Manual button variables
  bool       manualButton;            //  Boolean for water button on Adafruit


//****************************
// Constructors
//****************************
  Adafruit_SSD1306    displayOne(OLEDRESET);
  Adafruit_BME280     bmeOne;
  SdFat               SD;
  Adafruit_NeoPixel   waterPixel(PIXELNUM, PIXELPIN, WS2812B);
  AirQualitySensor    airQualitySensor(AQSPIN);
  Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 


//****************************
// Feeds
//****************************
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 
  Adafruit_MQTT_Publish    mqttObj1 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/Temp");
  Adafruit_MQTT_Publish    mqttObj2 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/Humidity");
  Adafruit_MQTT_Publish    mqttObj3 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/Pressure");
//  Adafruit_MQTT_Publish    mqttObj4 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/Moisture");
  Adafruit_MQTT_Publish    mqttObj5 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/WaterLevel");
  Adafruit_MQTT_Publish    mqttObj6 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/AirQuality");
  Adafruit_MQTT_Publish    mqttObj7 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/Dust");
  Adafruit_MQTT_Publish    mqttObj8 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/FillMe");
  Adafruit_MQTT_Publish    mqttObj9 = Adafruit_MQTT_Publish   (&mqtt, AIO_USERNAME "/feeds/FeedMe");

  Adafruit_MQTT_Subscribe  mqttObj10 = Adafruit_MQTT_Subscribe (&mqtt, AIO_USERNAME "/feeds/Water");

  
//********************************************************
//********************************************************
//      SETUP
//********************************************************
//********************************************************  
void setup() {
  Serial.begin(9600);
  while(!Serial) {}
//  Initialize NeoPixel
  waterPixel.begin();
  waterPixel.show();               // Initialize all pixels to 'off'

//  Initialize the BME280 
  status = bmeOne.begin(BMEADDRESS);
  if(!status){
    Serial.printf("BME 280 did not initialize correctly.  Please reset.\n");
    while(1);  //  You shall not pass 
  }
  BMEStart = millis();
  Serial.printf("BME 280 running\n");


//  Initialize the uSD Card
  // initialize uSD card module CS to off
  pinMode     (SD_CS_PIN,OUTPUT); 
  digitalWrite(SD_CS_PIN,HIGH);
    status = SD.begin(SD_CS_PIN);
    if (status == 0) {  // if status is false
      Serial.printf("Card failed, or not present\n");
    }
    else {
      Serial.printf("uSD card initialized.\n");
    } 
    // //  Print file headers to uSD
    // testFile = SD.open("testFile.csv", FILE_WRITE);
    // if (testFile) {
    //   testFile.printf("Time, Temp, Pres, MoistH, MoistL \n");
    //   testFile.close();
    //   Serial.printf("Time, Temp, Pres, MoistH, MoistL \n");
    // }

  //  Setup TIME
  Time.zone(-6);                                         //  Set time zone to MDT -6 from UTC
  Particle.syncTime();
  dateTime    = Time.timeStr();                          //  get current value of date and time
  timeOnlyOld = dateTime.substring(11,19);               //  Extract value of time from dateTime

  //  Setup Moisture Sensor
  pinMode(MOISTPIN, INPUT);                              //  Moisture pin is an input
  feedMe = false;
  
  //  Setup OLED
  displayOne.begin(SSD1306_SWITCHCAPVCC, OLEDADDRESS);   // initialize with the I2C addr above
  displayOne.setTextSize(1);
  displayOne.setTextColor(WHITE);
  displayOne.clearDisplay();                              //  Clear the display before going further
  displayOne.display();                                   // Force display

  //  Setup Dust Sensor
  pinMode(DUSTPIN, INPUT);
  lowPulseOccupancy = 0;                                 //  Initialize all Dust Sensor variables
  ratio             = 0;
  concentration     = 0;  
  sampleStart = millis();

  //  Setup Air Quality Sensor
  currentQuality =-1;
  airQualitySensor.init();

  //  Setup water level Sensor
  fillMe = false;
 
  //  Manual button setup
  pinMode(BUTTONPIN, INPUT);

  // Setup MQTT subscription for manual water button feed.
  mqtt.subscribe(&mqttObj10);



  endTime = millis();
}




//********************************************************
//********************************************************
//      LOOP
//********************************************************
//********************************************************
void loop() {

//****************************
// Get Environmental Data
//****************************
  //  Get current time
  dateTime = Time.timeStr();                          //  get current value of date and time
  timeOnly = dateTime.substring(11,19);               //  Extract value of time from dateTime

  //  Get current temp & humidity
  if (millis() - BMEStart > SAMPLETIME) {         //  Once the air has been samples for 30 sec
    tempC     = bmeOne.readTemperature();            //  Read temperature in Celcius
    tempF     = (tempC *9 / 5.0) + 32;               //  Convert temperature to Farenheight
    humidRH   = bmeOne.readHumidity();               //  Read relative humidity
    pressPA   = bmeOne.readPressure();               //  Read atmospheric pressure in Pascals
    pressInHg = pressPA * 0.00029530;                //  Convert pressure to inches of mercury
    BMEStart  = millis();                            // Reset the timer
  }

  //  Get current moisture levels
  moisture = analogRead(MOISTPIN);
  if (moisture > MOISTSET) {                         //  Plant is dry 
    feedMe = true;
  } else {                                           //  Plant is moist
    feedMe = false;
  }

  //  Get current Dust levels
  //  The following 2 WHILE statements replace a PULSEIN() statement to allow for a quicker timeout
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
  
  lowPulseOccupancy += duration;                       //  summ the total amount of low time
  if (millis() - sampleStart > SAMPLETIME) {           //  Once the air has been samples for 30 sec
    ratio = lowPulseOccupancy / (SAMPLETIME * 10.0);   // calculate the ratio
    concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) +520.0 * ratio + 0.62;  //  Calculate the concentration
    Serial.printf("LPO %i, concen %0.2f, ratio %0.2f\n", lowPulseOccupancy, concentration, ratio);
    lowPulseOccupancy = 0;                             // reset the LPO
    sampleStart = millis();                            // Reset the timer
  }

  //  Get current Air Quality levels
    currentQuality=airQualitySensor.slope();

    switch (currentQuality) {
      case 0:
            AQString = "**DON'T BREATHE**";
      break;
      
      case 1:
            AQString = " High pollution! ";
      break;
      
      case 2:
            AQString = " Low pollution!  ";
     break;
      
      case 3:
            AQString = "!!  Fresh air  !!";
      break;      
    }
//    Serial.printf("%s\n", AQString.c_str());

//**************************************
// Write values to OLED coninuously
//**************************************

  if (timeOnly.compareTo(timeOnlyOld)) {               //  When the seconds change, reprint the OLED
   write_SFFIS_ToOLED(timeOnly.c_str(), tempF, humidRH, moisture, AQString);
   timeOnlyOld = timeOnly;                             //  reset timeOnlyOld
  }

//****************************
// Write values to uSD Card
//****************************
//  Commented out for debug.  this code has been tested good
  // //  SAMPLE DATA once per minute
  // if (millis() - endTime > SAMPLETIME) {
  //   //  Write to the uSD card
  //   write_SFFII_ToSD(timeOnly.c_str(), temp, humid, moistureH, moistureL);
  //   //  Reset v ariables forthe next sample
  //   endTime = millis();
  // }

//**************************************
// Run water pump and check reservoir level
//**************************************
  manualButton = digitalRead(BUTTONPIN);
  if ((feedMe && (millis() - waterTime) > WATERTIME) || subscribeButton || manualButton) {  //  Plant is dry and it's watering time
    waterLevel = changeWaterLevel(true);                                         //  check reservoir water level and run motor
    waterTime = millis();                                                        //  Reset Watering timer
  }
  waterLevel = changeWaterLevel(false);                                          //  check reservoir water level and Dont run motor
  fillMe = waterPixelBlink(waterLevel);                                          //  Update the water level pixel

//****************************
// Publish and Subscribe Data
//****************************
  // Validate connected to MQTT Broker
  // NOTE:  Due to timing requirements, this does not fit well into a function
  MQTT_connect();

  // Ping MQTT Broker every 2 minutes to keep connection alive
  if ((millis()-pingTime)>PINGTIME) {
      Serial.printf("Pinging MQTT \n");
      if(! mqtt.ping()) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      pingTime = millis();
  }

  // publish to cloud every 30 seconds
  if((millis()-publishTime > PUBLISHTIME)) {
    if(mqtt.Update()) {
      mqttObj1.publish(tempF);
      mqttObj2.publish(humidRH);
      mqttObj3.publish(pressInHg);
//      mqttObj4.publish(moisture);
      mqttObj5.publish(waterLevel);
      mqttObj6.publish(AQString);
      mqttObj7.publish(concentration);
      mqttObj8.publish((int)fillMe);
      mqttObj9.publish((int)feedMe);
//      Serial.printf("Publishing %0.2f \n",value1); 
    // Serial.printf("Moisture: %i Moisture set point %i\n",moisture, MOISTSET);
    // Serial.printf("Feedme: %i \n",feedMe);
    // Serial.printf("FillMe: %i \n",fillMe);
      } 
    publishTime = millis();
  }

  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10))) {
    if (subscription == &mqttObj10) {
      subBut = atoi((char *)mqttObj10.lastread);
      Serial.printf("Received %i from Adafruit.io feed Water \n",subscribeButton);
      if (subBut == 1) {
        subscribeButton = true;
      } else {
        subscribeButton = false;      
      }
    }
  }
}




//********************************************************
//********************************************************
//      write_SFFIS_ToSD
//********************************************************
//********************************************************
void write_SFFIS_ToSD(String item1, float item2, float item3, int item4, String item5) {

  testFile = SD.open("testFile.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (testFile) {
    testFile.printf("%s, %0.2f, %0.2f, %i, %s \n", item1.c_str(), item2, item3, item4, item5.c_str());
    testFile.close();
    Serial.printf("%s, %0.2f, %0.2f, %i, %s \n", item1.c_str(), item2, item3, item4, item5.c_str());
  }  
  else {
    Serial.printf("error opening testFile.csv \n");
  }
  return;
}




// ********************************************************
// ********************************************************
//      write_SFFIS_ToOLED
// ********************************************************
// ********************************************************
void write_SFFIS_ToOLED(String item1, float item2, float item3, int item4, String item5) {
    // Write to the OLED display
    displayOne.clearDisplay();//  Clear the display before going further
    displayOne.drawRect(0,0,SCREENWIDTH,SCREENHEIGHT,WHITE);
    displayOne.setCursor(7,3);
    displayOne.printf(" Time is: %s\n", item1.c_str());
    displayOne.setCursor(7,13);
    displayOne.printf("    Temp: %0.1f%cF\n", item2, 247);
    displayOne.setCursor(7,23);
    displayOne.printf("Humidity: %0.1f%%\n", item3);
    displayOne.setCursor(7,33);
    displayOne.printf("Moisture: %i\n", item4);
    displayOne.setCursor(7,43);
    displayOne.printf("Air Quality: \n  %s", item5.c_str());
    displayOne.display(); // Force display
}


//********************************************************
//********************************************************
//      changeWaterLevel
//********************************************************
//********************************************************
int changeWaterLevel (bool motorRun) {
  const int H2OLEVELPIN  = A2;       //  Water Level Sensor on Analog Pin A2
  const int MOTORPIN     = D8;       //  Motor relay on digital pin D8
  const int H2OLEVELPWR  = D7;       //  Water Level Sensor Power on digital Pin D7
  int h2oLvl;
  
  //  Initialize Water level sensor
  pinMode(H2OLEVELPIN, INPUT);
  pinMode(H2OLEVELPWR, OUTPUT);
  digitalWrite(H2OLEVELPWR, HIGH);   // Drive water level sensor power to zero to limit corrosion
 
//  Initialize Water Pump
  pinMode(MOTORPIN, OUTPUT);
  digitalWrite(MOTORPIN, LOW);      //  Drive motor pin low to ensure motor does not run accidentally

if (motorRun) {
    digitalWrite(MOTORPIN,  HIGH);  // Turn on water pump
    delay(500);                        //  DELAY here on purpose to PRECISELY control water delivery
    digitalWrite(MOTORPIN,  LOW);     // Turn off water pump
  } else {
    delay(10);
  }
//  digitalWrite(H2OLEVELPWR, HIGH);    // Turn on water level sensor power
  h2oLvl = analogRead(H2OLEVELPIN);      //  Read water level
  digitalWrite(H2OLEVELPWR, LOW);     //  Turn off water levell sensor power to reduce galvanic corrosion
  return h2oLvl;                     //  Return water level value  
}

//********************************************************
//********************************************************
//      waterPixelBlink
//********************************************************
//********************************************************
// Code chaged to BANG-BANG because level sensor is not accurate enough for more detail
bool waterPixelBlink (int levelWater) {
  bool emptyRes;    // empty reservoir flag
  // const int HIGHWATER = 2000;
  // const int FULLWATER = 1800;
  // const int LOWWATER  = 1500;
  const int REFILLWATER = 1500;
  //  if (levelWater < 2000) {                         //  water level empty
  //   bright = callSin (127, 127, 1000);              //  Set brightness to flash at 1S interval
  //   waterPixel.setPixelColor(0,bright,0,0);         //  Set pixel color RED
  // } if (levelWater < 2150 && levelWater >= 2000) {  //  water level empty
  //   bright = callSin (63, 63, 3000);                //  Set brightness to flash at 5S interval
  //   waterPixel.setPixelColor(0,bright,bright,0);    //  Set pixel color YELLOW
  // } if (levelWater < 2500 && levelWater >= 2150) {  //  water level empty
  //   bright = callSin (31, 31, 10000);               //  Set brightness to flash at 10S interval
  //   waterPixel.setPixelColor(0,0,bright,0);         //  Set pixel color GREEN
  // } if (levelWater > 2500) {                        //  water level full
  //   waterPixel.setPixelColor(0,0,63,0);             //  Set pixel color GREEN
  // }
   if (levelWater < REFILLWATER) {                         //  water level empty
    waterPixel.setPixelColor(0,255,0,0);         //  Set pixel color RED
    emptyRes = true;
  } else if (levelWater > REFILLWATER) {                        //  water level full
    waterPixel.setPixelColor(0,0,63,0);             //  Set pixel color GREEN
    emptyRes = false;
  }
    waterPixel.show();
    return emptyRes;
}

//********************************************************
//********************************************************
//      callSin
//********************************************************
//********************************************************
float callSin(int amp, int deltaAmp, int period) {
  float sinWave;
  float t;

  t = millis();

  sinWave = amp * sin(2 * M_PI * t / period) + deltaAmp;
  return sinWave;
}

//********************************************************
//********************************************************
//      MQTT_connect
//********************************************************
//********************************************************
// Function to connect and reconnect as necessary to the MQTT server.
void MQTT_connect() {
  int8_t ret;
 
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("%s\n",(char *)mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds..\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.printf("MQTT Connected!\n");
}
