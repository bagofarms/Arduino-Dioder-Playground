
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

#define DS1307_I2C_ADDRESS 0x68
#define DS1307_SQWE 7 // 07h - squarewave control register
#define DS1307_HI_SQWE B00010000 // SQWE = 1 (enabled), RS1 = 0, RS0 = 0

RTC_DS1307 RTC;

volatile byte lastHour = 0;
volatile byte lastDayOfMonth = 0;

// Set the three PWM pins to use for each color.  9,10,11 are the other 3
const int redPin = 3;
const int grnPin = 5;
const int bluPin = 6;

// Set input pin for square wave from RTC
const int swIn = 2;

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

// Set initial color
int redVal = white[0];
int grnVal = white[1]; 
int bluVal = white[2];

// Initialize color variables
int prevR = redVal;
int prevG = grnVal;
int prevB = bluVal;

// Sunrise/sunset variables
TimeLord timeLord;
volatile byte sunRise[6] = {0, 0, 0, 0, 0, 0};
volatile byte sunSet[6] = {0, 0, 0, 0, 0, 0};

int wait = 5;      // 10ms internal crossFade delay; increase for slower fades
int hold = 5000;       // Optional hold when a color is complete, before the next crossFade

boolean updateColorFlag = false;

// Will be called every second by the RTC
// Based on the current time, looks up what color the Dioder should have
void updateColor(){
  updateColorFlag = true;
}

void setup() {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();

  // Initialize clock with correct time
  RTC.adjust(DateTime(__DATE__, __TIME__));
  
  // Set 1Hz Square Wave output
  /*
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(DS1307_SQWE);                    //Select the SQWE byte
  Wire.write(DS1307_HI_SQWE);                 //Set the SQWE flag to 1, with everything else 0
  Wire.endTransmission();
  */
  
  
  // Color Pin Setup
  pinMode(redPin, OUTPUT);   // sets the pins as output
  pinMode(grnPin, OUTPUT);   
  pinMode(bluPin, OUTPUT);
  
  setToColor(white);
  
  pinMode(swIn, INPUT);
  attachInterrupt(0, updateColor, RISING);  // Digital pin 2
  
  // Configure TimeLord
  timeLord.Position(28.6, -81.2);
  timeLord.TimeZone(-5 * 60);
  
  //delay(1000);
}


void loop() {
  // Color update logic 
  if(updateColorFlag){
    DateTime now = RTC.now();
    
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
    
    // If it's the same day, update the sunRise and sunSet times
    if(now.day() != lastDayOfMonth){
      sunRise[3] = byte(now.day());
      sunRise[4] = byte(now.month());
      sunRise[5] = byte(now.year());
      timeLord.SunRise((byte*)sunRise);
      
      sunSet[3] = byte(now.day());
      sunSet[4] = byte(now.month());
      sunSet[5] = byte(now.year());
      timeLord.SunRise((byte*)sunSet);
      Serial.print("Sunrise Time = ");
      Serial.print(sunRise[2], DEC);
      Serial.print(':');
      Serial.print(sunRise[1], DEC);
      Serial.print(':');
      Serial.print(sunRise[0], DEC);
      Serial.println();
    }
  
    lastDayOfMonth = now.day();
    
    updateColorFlag = false;
  }
  /*
  // Skip this if we're still in the same hour
  if(now.hour() != lastHour){
    switch(now.hour()){
      case 6:
        crossFade(purple);
        break;
      case 7:
        crossFade(red);
        break;
      case 8:
        crossFade(orange);
        break;
      case 9:
        crossFade(blue);
        break;
      case 10:
        crossFade(ltblue);
        break;
      case 11:
        crossFade(ltblue);
        break;
      case 12:
        crossFade(white);
        break;
      case 13:
        crossFade(white);
        break;
      case 14:
        crossFade(ltblue);
        break;
      case 15:
        crossFade(ltgreen);
        break;
      case 16:
        crossFade(green);
        break;
      case 17:
        crossFade(orange);
        break;
      case 18:
        crossFade(orange);
        break;
      case 19:
        crossFade(orange);
        break;
      case 20:
        crossFade(red);
        break;
      case 21:
        crossFade(purple);
        break;
      case 22:
        crossFade(white);
        break;
    }
  }
  
  lastHour = now.hour();
  */
  delay(50);
}


/**** Color Functions ****/
void setToColor(int color[3]){
  analogWrite(redPin, color[0]);
  analogWrite(grnPin, color[1]);      
  analogWrite(bluPin, color[2]); 
}

/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
* 
* The program works like this:
* Imagine a crossfade that moves the red LED from 0-10, 
*   the green from 0-5, and the blue from 10 to 7, in
*   ten steps.
*   We'd want to count the 10 steps and increase or 
*   decrease color values in evenly stepped increments.
*   Imagine a + indicates raising a value by 1, and a -
*   equals lowering it. Our 10 step fade would look like:
* 
*   1 2 3 4 5 6 7 8 9 10
* R + + + + + + + + + +
* G   +   +   +   +   +
* B     -     -     -
* 
* The red rises from 0 to 10 in ten steps, the green from 
* 0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
* 
* In the real program, the color percentages are converted to 
* 0-255 values, and there are 1020 steps (255*4).
* 
* To figure out how big a step there should be between one up- or
* down-tick of one of the LED values, we call calculateStep(), 
* which calculates the absolute gap between the start and end values, 
* and then divides that gap by 1020 to determine the size of the step  
* between adjustments in the value.
*/

int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero, 
    step = 1020/step;              //   divide by 1020
  } 
  return step;
}

/* The next function is calculateVal. When the loop value, i,
*  reaches the step size appropriate for one of the
*  colors, it increases or decreases the value of that color by 1. 
*  (R, G, and B are each calculated separately.)
*/

int calculateVal(int step, int val, int i) {
  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;           
    } 
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    } 
  }
  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  } 
  else if (val < 0) {
    val = 0;
  }
  return val;
}

/* crossFade() converts the percentage colors to a 
*  0-255 range, then loops 1020 times, checking to see if  
*  the value needs to be updated each time, then writing
*  the color values to the correct pins.
*/

void crossFade(int color[3]) {
  int stepR = calculateStep(prevR, color[0]);
  int stepG = calculateStep(prevG, color[1]); 
  int stepB = calculateStep(prevB, color[2]);

  for (int i = 0; i <= 1020; i++) {
    redVal = calculateVal(stepR, redVal, i);
    grnVal = calculateVal(stepG, grnVal, i);
    bluVal = calculateVal(stepB, bluVal, i);

    analogWrite(redPin, redVal);   // Write current values to LED pins
    analogWrite(grnPin, grnVal);      
    analogWrite(bluPin, bluVal); 

    delay(wait); // Pause for 'wait' milliseconds before resuming the loop
  }
  
  // Update current values for next loop
  prevR = redVal; 
  prevG = grnVal; 
  prevB = bluVal;
  delay(hold); // Pause for optional 'wait' milliseconds before resuming the loop
}
