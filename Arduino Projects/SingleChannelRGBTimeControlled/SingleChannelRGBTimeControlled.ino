/* 
 * This program uses the current time to slowly change the color of the Ikea DIODER lights.
 * 
 * This is a fork of a project by Daniel Kennett.  The original used an Arduino Mega, which 
 * has enough PWM pins to do 4 independent channels.  I am only using 3 pins here to control 
 * one channel, since i want the color to be uniform across all of the lights.
 * 
 * With this code you can set the date/time, retreive the date/time and use the extra memory of an RTC DS1307 chip.  
 * The program also sets all the extra memory space to 0xff.
 * Serial Communication method with the Arduino that utilizes a leading CHAR for each command described below. 
 *
 * Commands:
 * T(00-59)(00-59)(00-23)(1-7)(01-31)(01-12)(00-99) - T(sec)(min)(hour)(dayOfWeek)(dayOfMonth)(month)(year) -
 * T - Sets the date of the RTC DS1307 Chip. 
 * Example to set the time for 25-Jan-2012 @ 19:57:11 for the 4 day of the week, use this command - T1157194250112
 * Q(1-2) - (Q1) Memory initialization  (Q2) RTC - Memory Dump  
 * R - Read/display the time, day and date
 */

// Provides sunrise/sunset information
#include <TimeLord.h> 

#include "Wire.h"
#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
// Arduino version compatibility Pre-Compiler Directives
#if defined(ARDUINO) && ARDUINO >= 100   // Arduino v1.0 and newer
  #define I2C_WRITE Wire.write 
  #define I2C_READ Wire.read
#else                                   // Arduino Prior to v1.0 
  #define I2C_WRITE Wire.send 
  #define I2C_READ Wire.receive
#endif

// Global Variables
int command = 0;       // This is the command char, in ascii form, sent from the serial port     
int i;
long previousMillis = 0;        // will store last time Temp was updated
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
byte test;
byte zero;
char  *Day[] = {"","Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char  *Mon[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

/* Set the three PWM pins to use for each color.  9,10,11 are the other 3 */
const int redPin = 3;
const int grnPin = 5;
const int bluPin = 6;

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
byte sunRise[] = {0, 0, 0, 0, 0, 0};
byte sunSet[] = {0, 0, 0, 0, 0, 0};

int wait = 5;      // 10ms internal crossFade delay; increase for slower fades
int hold = 5000;       // Optional hold when a color is complete, before the next crossFade

// Calendar values

/**** Clock Functions ****/
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}
 
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return ( (val/16*10) + (val%16) );
}
 
// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock
// Assumes you're passing in valid numbers, Probably need to put in checks for valid numbers.
 
void setDateDs1307(){
   second = (byte) ((Serial.read() - 48) * 10 + (Serial.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  
   minute = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   hour  = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   dayOfWeek = (byte) (Serial.read() - 48);
   dayOfMonth = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   month = (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   year= (byte) ((Serial.read() - 48) *10 +  (Serial.read() - 48));
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   I2C_WRITE(zero);
   I2C_WRITE(decToBcd(second) & 0x7f);    // 0 to bit 7 starts the clock
   I2C_WRITE(decToBcd(minute));
   I2C_WRITE(decToBcd(hour));      // If you want 12 hour am/pm you need to set
                                   // bit 6 (also need to change readDateDs1307)
   I2C_WRITE(decToBcd(dayOfWeek));
   I2C_WRITE(decToBcd(dayOfMonth));
   I2C_WRITE(decToBcd(month));
   I2C_WRITE(decToBcd(year));
   Wire.endTransmission();
}
 
// Gets the date and time from the ds1307 and prints result
void getDateDs1307(){
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  I2C_WRITE(zero);
  Wire.endTransmission();
 
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
 
  // A few of these need masks because certain bits are control bits
  second     = bcdToDec(I2C_READ() & 0x7f);
  minute     = bcdToDec(I2C_READ());
  hour       = bcdToDec(I2C_READ() & 0x3f);  // Need to change this if 12 hour am/pm
  dayOfWeek  = bcdToDec(I2C_READ());
  dayOfMonth = bcdToDec(I2C_READ());
  month      = bcdToDec(I2C_READ());
  year       = bcdToDec(I2C_READ());
 
  if (hour < 10)
    Serial.print("0");
  Serial.print(hour, DEC);
  Serial.print(":");
  if (minute < 10)
    Serial.print("0");
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second < 10)
    Serial.print("0");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(Day[dayOfWeek]);
  Serial.print(", ");
  Serial.print(dayOfMonth, DEC);
  Serial.print(" ");
  Serial.print(Mon[month]);
  Serial.print(" 20");
  if (year < 10)
    Serial.print("0");
  Serial.println(year, DEC);
}

/**** Color Functions ****/
void setToColor(int color[3]){
  analogWrite(redPin, color[0]);
  analogWrite(grnPin, color[1]);      
  analogWrite(bluPin, color[2]); 
}

void setDate(){
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(byte(0x00));
  Wire.write(decToBcd(second));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));    // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
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

// Will be called every second by the RTC
// Based on the current time, looks up what color the Dioder should have
void updateColor(){
  byte prev_dayOfMonth = dayOfMonth;
  // Get current date and time
  getDateDs1307();
  
  // If it's the same day, update the sunRise and sunSet times
  if(dayOfMonth != prev_dayOfMonth){
    sunRise = {0, 0, 0, dayOfMonth, month, year};
    timeLord.SunRise(sunRise);
    
    sunSet = {0, 0, 0, dayOfMonth, month, year};
    timeLord.SunRise(sunSet);
  }
  
  Serial.println("Heartbeat");
}

void setup() {
  // Clock Setup
  Wire.begin();
  Serial.begin(57600); 
  zero=0x00;
  
  // Color Pin Setup
  pinMode(redPin, OUTPUT);   // sets the pins as output
  pinMode(grnPin, OUTPUT);   
  pinMode(bluPin, OUTPUT);
  
  setToColor(white);
  attachInterrupt(0, updateColor, RISING);  // Digital pin 2
  
  // Configure TimeLord
  timeLord.Position(28.6, -81.2);
  timeLord.TimeZone(-5 * 60);
  
  delay(1000);
}


void loop() {
  byte lastHour = hour;
  /* Color update logic */
  getDateDs1307();
  
  // Skip this if we're still in the same hour
  if(hour != lastHour){
    switch(hour){
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
  
  
  /* Clock Maintenance Code */
  if (Serial.available()) {      // Look for char in serial que and process if found
    command = Serial.read();
    if (command == 84 || command == 116) {      //If command = "Tt" Set Date
      setDateDs1307();
      getDateDs1307();
      Serial.println(" ");
    } 
    else if (command == 82 || command == 114) {      //If command = "Rr" Read Date ... BBR
      getDateDs1307();
      Serial.println(" ");
    } 
    else if (command == 81 || command == 113) {      //If command = "Qq" RTC1307 Memory Functions
      delay(100);     
      if (Serial.available()) {
        command = Serial.read(); 
        if (command == 49) {        //If command = "1" RTC1307 Initialize Memory - All Data will be set to 
                                    // 255 (0xff).  Therefore 255 or 0 will be an invalid value.  
          Wire.beginTransmission(DS1307_I2C_ADDRESS);   // 255 will be the init value and 0 will be considered 
                                          // an error that occurs when the RTC is in Battery mode.
          I2C_WRITE(0x08); // Set the register pointer to be just past the date/time registers.
          for (i = 1; i <= 24; i++) {
            I2C_WRITE(0Xff);
            delay(10);
          }   
          Wire.endTransmission();
          Wire.beginTransmission(DS1307_I2C_ADDRESS);   
          I2C_WRITE(0x21); // Set the register pointer to 33 for second half of registers. Only 32 writes per connection allowed.
          for (i = 1; i <= 33; i++) {
            I2C_WRITE(0Xff);
            delay(10);
          }   
          Wire.endTransmission();
          getDateDs1307();
          Serial.println(": RTC1307 Initialized Memory");
        }
        else if (command == 50) {      //If command = "2" RTC1307 Memory Dump
          getDateDs1307();
          Serial.println(": RTC 1307 Dump Begin");
          Wire.beginTransmission(DS1307_I2C_ADDRESS);
          I2C_WRITE(zero);
          Wire.endTransmission();
          Wire.requestFrom(DS1307_I2C_ADDRESS, 32);
          for (i = 0; i <= 31; i++) {  //Register 0-31 - only 32 registers allowed per I2C connection
            test = I2C_READ();
            Serial.print(i);
            Serial.print(": ");
            Serial.print(test, DEC);
            Serial.print(" : ");
            Serial.println(test, HEX);
          }
          Wire.beginTransmission(DS1307_I2C_ADDRESS);
          I2C_WRITE(0x20);
          Wire.endTransmission();
          Wire.requestFrom(DS1307_I2C_ADDRESS, 32);  
          for (i = 32; i <= 63; i++) {         //Register 32-63 - only 32 registers allowed per I2C connection
            test = I2C_READ();
            Serial.print(i);
            Serial.print(": ");
            Serial.print(test, DEC);
            Serial.print(" : ");
            Serial.println(test, HEX);
          }
          Serial.println(" RTC1307 Dump end");
        } 
      }  
    }
    Serial.print("Command: ");
    Serial.println(command);     // Echo command CHAR in ascii that was sent
  }
  command = 0;                 // reset command 
  /* End Clock Maintenance Code */
  
  
  /* Delay between loops */
  delay(10000);
}
