
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
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

/*
#define DS1307_I2C_ADDRESS 0x68
#define DS1307_SQWE 0x07 // 07h - squarewave control register
#define DS1307_HI_SQWE 0x10 // SQWE = 1 (enabled), RS1 = 0, RS0 = 0
*/

// Matrix raw display values
#define MATRIX_BLANK 0B000000000

RTC_DS1307 RTC;

// Instantiate clock display
Adafruit_7segment matrix = Adafruit_7segment();

// Set the three PWM pins to use for each color.  9,10,11 are the other 3
const int redPin = 3;
const int grnPin = 5;
const int bluPin = 6;

// Set input pin for square wave from RTC
//const int swIn = 3;

// Color arrays
int black[3]    = {   0,   0,   0 };
int white[3]    = { 255, 255, 255 };
int red[3]      = { 255,   0,   0 };
int green[3]    = {   0, 255,   0 };
int blue[3]     = {   0,   0, 255 };
int ltgreen[3]  = { 100, 255, 100 };
int ltblue[3]   = { 100, 100, 255 };
int ltyellow[3] = { 255, 200,  75 };
int orange[3]   = { 255, 101,   0 };
int purple[3]   = { 255,   0, 255 };
int yellow[3]   = { 255, 200,   0 };

// Variables that will hold the current color value
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
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  // Example initialization with a specific time (useful for debugging)
  //RTC.adjust(DateTime(__DATE__, "10:59:00"));
  
  // Color Pin Setup
  pinMode(redPin, OUTPUT);   // sets the pins as output
  pinMode(grnPin, OUTPUT);   
  pinMode(bluPin, OUTPUT);
  
  // Configure TimeLord for Orlando, FL and GMT-5 (Eastern)
  timeLord.Position(28.6, -81.2);
  timeLord.TimeZone(-5 * 60);
  
  if(DEBUG){
    Serial.begin(9600);
    DateTime now = RTC.now();
    printTime(now);
  }
  
  // Run startup test sequence
  setToColor(purple);
  delay(200);
  setToColor(red);
  delay(200);
  setToColor(orange);
  delay(200);
  setToColor(blue);
  delay(200);
  setToColor(green);
  delay(200);
  setToColor(white);

  // Init display
  matrix.begin(0x70);
  matrix.setBrightness(1);
  matrix.drawColon(true);
  matrix.writeDigitNum(0, 0, false);
  matrix.writeDigitNum(1, 1, false);
  matrix.writeDigitNum(3, 2, false);
  matrix.writeDigitNum(4, 3, false);
  matrix.writeDisplay();
  
  delay(500);
}


void loop() {
  // Get the current time
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
  updateClock(now);
  
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
  
  if(DEBUG){
    Serial.print("Sunrise Timestamp = ");
    Serial.println(sunRiseTimestamp);
    Serial.print("Sunset Timestamp = ");
    Serial.println(sunSetTimestamp);
  }
  
  cycleTimes[0] = sunRiseTimestamp - cycleOffsets[0];  //Before sunrise
  cycleTimes[1] = sunRiseTimestamp;                    //Sunrise
  cycleTimes[2] = sunRiseTimestamp + cycleOffsets[0];  //After sunrise
  cycleTimes[3] = NOON - cycleOffsets[1];              //Before noon
  cycleTimes[4] = NOON;                                //Noon
  cycleTimes[5] = NOON + cycleOffsets[1];              //After Noon
  cycleTimes[6] = sunSetTimestamp - cycleOffsets[2];   //Before sunset
  cycleTimes[7] = sunSetTimestamp;                     //Sunset
  cycleTimes[8] = sunSetTimestamp + cycleOffsets[2];   //After Sunset
  
  if(DEBUG){
    printCycleTimes();
  }
}

void updateClock(DateTime now){
  // Display hour
  int leftHour = now.twelveHour() / 10;
  if (leftHour == 0)
  {
    matrix.writeDigitRaw(0, MATRIX_BLANK);
  }
  else
  {
    matrix.writeDigitNum(0, (now.twelveHour() / 10), false);
  }
  matrix.writeDigitNum(1, now.twelveHour() % 10, false);

  // Display minute
  matrix.writeDigitNum(3, (now.minute() / 10), false);
  matrix.writeDigitNum(4, now.minute() % 10, now.isPM());

  // Write out our changes to the display
  matrix.writeDisplay();
}

void updateColor(DateTime now){
  // If statements detecting each cycleTime
  uint32_t curTime = secondsFromMidnight(byte(now.hour()), byte(now.minute()), byte(now.second()));
  
  if(curTime <= cycleTimes[0]){
    setToColor(black);
    if(DEBUG){
      Serial.println("--Before sunrise");
    }
  }else if(curTime > cycleTimes[8]){
    setToColor(ltyellow);
    if(DEBUG){
      Serial.println("--After sunset");
    }
  }else{
    for(int i=0; i<9; i++){
      if(curTime > cycleTimes[i] && curTime <= cycleTimes[i+1]){
        
        
        if(DEBUG){
          Serial.println("Time Period Found: ");
          Serial.print("  i = ");
          Serial.println(i);
          Serial.print("  curTime = ");
          Serial.println(curTime);
          Serial.print("  cycleTimes[i] = ");
          Serial.println(cycleTimes[i]);
          Serial.print("  cycleTimes[i+1] = ");
          Serial.println(cycleTimes[i+1]);
          Serial.print("  cycleColors[i] = ");
          printColors(cycleColors[i]);
          Serial.print("  cycleColors[i+1] = ");
          printColors(cycleColors[i+1]);
        }
        
        
        calculateColors(curTime, cycleTimes[i], cycleTimes[i+1], cycleColors[i], cycleColors[i+1]);
        break;
      }
    }
  }
}


uint32_t secondsFromMidnight(byte in_hour, byte in_min, byte in_sec){
  uint32_t hours_in_seconds = uint32_t(in_hour) * 3600;
  uint32_t minutes_in_seconds = uint32_t(in_min) * 60;
  uint32_t seconds = uint32_t(in_sec);
  /*
  if(DEBUG){
    Serial.print("{h,m,s} = {");
    Serial.print(hours_in_seconds);
    Serial.print(',');
    Serial.print(minutes_in_seconds);
    Serial.print(',');
    Serial.print(seconds);
    Serial.println('}');
  }
  */
  
  return hours_in_seconds + minutes_in_seconds + seconds;
}


void calculateColors(uint32_t curTime, uint32_t startTime, uint32_t endTime, int startColor[3], int endColor[3]){
  //Calculate time elapsed per step
  int dTRed = calculateStep(startTime, endTime, startColor[0], endColor[0]);
  int dTGrn = calculateStep(startTime, endTime, startColor[1], endColor[1]);
  int dTBlu = calculateStep(startTime, endTime, startColor[2], endColor[2]);
  
  //Calculate current time elapsed in the section of the day
  uint32_t dTime = curTime - startTime;
  
  if(DEBUG){
    Serial.println("Calculating Colors:");
    Serial.print("  dTRed = ");
    Serial.println(dTRed);
    Serial.print("  dTGrn = ");
    Serial.println(dTGrn);
    Serial.print("  dTBlu = ");
    Serial.println(dTBlu);
    Serial.print("  dTime = ");
    Serial.println(dTime);
  }
  
  int setColors[3];
  
  setColors[0] = findColor(startColor[0], redVal, dTRed, dTime);
  setColors[1] = findColor(startColor[1], grnVal, dTGrn, dTime);
  setColors[2] = findColor(startColor[2], bluVal, dTBlu, dTime);
  
  setToColor(setColors);
}

// Calculates number of seconds between each step of the color
int calculateStep(uint32_t startTime, uint32_t endTime, int startColor, int endColor){
  int retVal;
  int colorDiff = endColor - startColor;
  
  if(DEBUG){
    Serial.println("Calculating Step:");
    Serial.print("  startTime = ");
    Serial.println(startTime);
    Serial.print("  endTime = ");
    Serial.println(endTime);
    Serial.print("  startColor = ");
    Serial.println(startColor);
    Serial.print("  endColor = ");
    Serial.println(endColor);
    Serial.print("  colorDiff = ");
    Serial.println(colorDiff);
  }
  
  // Protect against division by zero
  if(colorDiff == 0){
    retVal = 0;
  }else{
    int32_t timeDiff = int32_t(endTime) - int32_t(startTime);
    retVal = int(timeDiff / int32_t(colorDiff));
  }
  
  return retVal;
}


//Finds the current correct value for the color
int findColor(int startColor, int curColor, int dTColor, uint32_t dTime){
  int retVal;
  
  if(dTColor == 0){
    //If the color is already correct, just set it to what the start color was for this segment of the day
    retVal =  startColor;
  }else if( dTime % dTColor ){
    //find out how many steps we've done, add that to starting color for this segment
    retVal = startColor + int( int32_t(dTime) / int32_t(dTColor) );
  }else{
    retVal = curColor;
  }
  
  // Check to make sure value is within allowed range
  // If the color value is negative, it will evaluate to 255.
  // If it's greater than 255, it will evaluate to 0.
  if(retVal < 0){
    retVal = 0;
  }else if(retVal > 255){
    retVal = 255;
  }
  
  return retVal;
}


void setToColor(int color[3]){
  redVal = color[0];
  grnVal = color[1];
  bluVal = color[2];
  
  analogWrite(redPin, color[0]);
  analogWrite(grnPin, color[1]);      
  analogWrite(bluPin, color[2]);
  
  if(DEBUG){
    printColors(color);
  }
}


void printColors(int color[3]){
  Serial.print('{');
  Serial.print(color[0]);
  Serial.print(',');
  Serial.print(color[1]);
  Serial.print(',');
  Serial.print(color[2]);
  Serial.print('}');
  Serial.println();
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

void printCycleTimes(){
  Serial.print('{');
  Serial.print(cycleTimes[0]);
  Serial.print(',');
  Serial.print(cycleTimes[1]);
  Serial.print(',');
  Serial.print(cycleTimes[2]);
  Serial.print(',');
  Serial.print(cycleTimes[3]);
  Serial.print(',');
  Serial.print(cycleTimes[4]);
  Serial.print(',');
  Serial.print(cycleTimes[5]);
  Serial.print(',');
  Serial.print(cycleTimes[6]);
  Serial.print(',');
  Serial.print(cycleTimes[7]);
  Serial.print(',');
  Serial.print(cycleTimes[8]);
  Serial.print('}');
  Serial.println();
}
