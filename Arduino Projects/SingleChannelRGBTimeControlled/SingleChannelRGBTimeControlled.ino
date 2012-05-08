
/* 
 * This program uses the current time to slowly change the color of the Ikea DIODER lights.
 * 
 * This is a fork of a project by Daniel Kennett.  The original used an Arduino Mega, which 
 * has enough PWM pins to do 4 independent channels.  I am only using 3 pins here to control 
 * one channel, since i want the color to be uniform across all of the lights.
 */
 
#include <TimeLord.h> // Provides sunrise/sunset information
#include <Wire.h>
#include <RTClib.h>

/*
#define DS1307_I2C_ADDRESS 0x68
#define DS1307_SQWE 0x07 // 07h - squarewave control register
#define DS1307_HI_SQWE 0x10 // SQWE = 1 (enabled), RS1 = 0, RS0 = 0
*/

RTC_DS1307 RTC;

// Set the three PWM pins to use for each color.  9,10,11 are the other 3
const int redPin = 9;
const int grnPin = 10;
const int bluPin = 11;

// Set input pin for square wave from RTC
//const int swIn = 3;

// Color arrays
int black[3]  = {   0,   0,   0 };
int white[3]  = { 255, 255, 255 };
int red[3]    = { 255,   0,   0 };
int green[3]  = {   0, 255,   0 };
int ltgreen[3]= { 100, 255, 100 };
int blue[3]   = {   0,   0, 255 };
int ltblue[3] = { 100, 100, 255 };
int orange[3] = { 255, 101,   0 };
int purple[3] = { 255,   0, 255 };

int redVal;
int grnVal;
int bluVal;

// Sunrise/sunset variables
TimeLord timeLord;
byte sunRise[6] = {0, 0, 0, 0, 0, 0};
byte sunSet[6] = {0, 0, 0, 0, 0, 0};

// Transistion variables
uint32_t cycleTimes[9];
uint32_t cycleOffsets[3] = {1800, 3600, 1800};
int* cycleColors[9] = {purple, red, orange, blue, white, green, orange, red, purple};
uint32_t NOON = 43200;
byte prevHour = 0;
byte prevDayOfMonth = 0;

boolean DEBUG = true;


void setup() {
  Wire.begin();
  RTC.begin();

  // Initialize clock with correct time
  RTC.adjust(DateTime(__DATE__, __TIME__));
  
  // Color Pin Setup
  pinMode(redPin, OUTPUT);   // sets the pins as output
  pinMode(grnPin, OUTPUT);   
  pinMode(bluPin, OUTPUT);
  
  setToColor(white);
  
  // Configure TimeLord
  timeLord.Position(28.6, -81.2);
  timeLord.TimeZone(-5 * 60);
  
  if(DEBUG){
    Serial.begin(9600);
    DateTime now = RTC.now();
    printTime(now);
  }
}


void loop() {
  DateTime now = RTC.now();
  
  if(DEBUG){
    printTime(now);
  }
  
  // Get sunrise and sunset values and update the cycle times
  if(now.day() != prevDayOfMonth){
    updateSunriseSunset(now);
    updateCycleTimes();
  }
  prevDayOfMonth = now.day();    //Update the day so that we can compare on the next loop
  
  updateColor(now);
  
  delay(1000);
}


void updateSunriseSunset(DateTime now){
  sunRise[3] = byte(now.day());
  sunRise[4] = byte(now.month());
  sunRise[5] = byte(now.year());
  timeLord.SunRise((byte*)sunRise);
  
  sunSet[3] = byte(now.day());
  sunSet[4] = byte(now.month());
  sunSet[5] = byte(now.year());
  timeLord.SunSet((byte*)sunSet);
  
  if(DEBUG){
    printSunriseSunset();
  }
}


void updateCycleTimes(){
  uint32_t sunRiseTimestamp = secondsFromMidnight(sunRise[2], sunRise[1], sunRise[0]);
  uint32_t sunSetTimestamp = secondsFromMidnight(sunSet[2], sunSet[1], sunSet[0]);
  
  cycleTimes[0] = sunRiseTimestamp - cycleOffsets[0];  //Before sunrise
  cycleTimes[1] = sunRiseTimestamp;                    //Sunrise
  cycleTimes[2] = sunRiseTimestamp + cycleOffsets[0];  //After sunrise
  cycleTimes[3] = NOON - cycleOffsets[1];              //Before noon
  cycleTimes[4] = NOON;                                //Noon
  cycleTimes[5] = NOON + cycleOffsets[1];              //After Noon
  cycleTimes[6] = sunSetTimestamp - cycleOffsets[2];   //Before noon
  cycleTimes[7] = sunSetTimestamp;                     //Noon
  cycleTimes[8] = sunSetTimestamp + cycleOffsets[2];   //After Noon
}


void updateColor(DateTime now){
  // If statements detecting each cycleTime
  uint32_t curTime = secondsFromMidnight(byte(now.hour()), byte(now.minute()), byte(now.second()));
  
  if(curTime <= cycleTimes[0]){
    setToColor(cycleColors[0]);
  }else if(curTime > cycleTimes[8]){
    setToColor(cycleColors[8]);
  }else{
    for(int i=0; i<9; i++){
      if(curTime > cycleTimes[i] && curTime <= cycleTimes[i+1]){
        calculateColors(curTime, cycleTimes[i], cycleTimes[i+1], cycleColors[i], cycleColors[i+1]);
        break;
      }
    }
  }
}


uint32_t secondsFromMidnight(byte in_hour, byte in_min, byte in_sec){
  return uint32_t(in_hour*3600 + in_min*60 + in_sec);
}


void calculateColors(uint32_t curTime, uint32_t startTime, uint32_t endTime, int startColor[3], int endColor[3]){
  //Calculate time elapsed per step
  int dTRed = int( (endTime - startTime) / (endColor[0] - startColor[0]) );
  int dTGrn = int( (endTime - startTime) / (endColor[1] - startColor[1]) );
  int dTBlu = int( (endTime - startTime) / (endColor[2] - startColor[2]) );
  
  //Calculate current time elapsed in the section of the day
  uint32_t dTime = curTime - startTime;
  
  int setColors[3];
  
  setColors[0] = findColor(redVal, dTRed, dTime);
  setColors[1] = findColor(grnVal, dTGrn, dTime);
  setColors[2] = findColor(bluVal, dTBlu, dTime);
  
  setToColor(setColors);
}


//Finds the current correct value for the color
int findColor(int curColor, int dTColor, uint32_t dTime){
  if( dTColor && ( dTime % dTColor) ){
    //find out how many steps we've done
    return curColor + ( dTime / dTColor );
    //Add that to curColor and return
  }else{
    return curColor;
  }
}


void setToColor(int color[3]){
  redVal = color[0];
  grnVal = color[1];
  bluVal = color[2];
  
  analogWrite(redPin, color[0]);
  analogWrite(grnPin, color[1]);      
  analogWrite(bluPin, color[2]); 
}


void printTime(DateTime now){
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}


void printSunriseSunset(){
  Serial.print("Sunrise Time = ");
  Serial.print(sunRise[2], DEC);
  Serial.print(':');
  Serial.print(sunRise[1], DEC);
  Serial.print(':');
  Serial.print(sunRise[0], DEC);
  Serial.println();
  Serial.print("Sunset Time = ");
  Serial.print(sunSet[2], DEC);
  Serial.print(':');
  Serial.print(sunSet[1], DEC);
  Serial.print(':');
  Serial.print(sunSet[0], DEC);
  Serial.println();
}
