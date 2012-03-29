/*

This program uses the current time to slowly change the color of the Ikea DIODER lights.

This is a fork of a project by Daniel Kennett.  The original used an Arduino Mega, which 
has enough PWM pins to do 4 independent channels.  I am only using 3 pins here to control 
one channel, since i want the color to be uniform across all of the lights.

*/

/* Choose the first 3 PWM pins.  9,10,11 are the other 3 */
const int channel1PinR = 3;
const int channel1PinG = 5;
const int channel1PinB = 6;
const int del = 50;
const int maxbr = 255;
const int minbr = 50;

void setup() {
  // set pins as outputs
  pinMode(channel1PinR, OUTPUT); 
  analogWrite(channel1PinR, 0);
  pinMode(channel1PinG, OUTPUT); 
  analogWrite(channel1PinG, 0);
  pinMode(channel1PinB, OUTPUT); 
  analogWrite(channel1PinB, 0);
}


void loop () {
  /*
  analogWrite(channel1PinR, random(0,255));
  analogWrite(channel1PinG, random(0,255));
  analogWrite(channel1PinB, random(0,255));
  delay(random(30,200));
  */
  for(int i=3; i>0; i--){
    analogWrite(channel1PinR, random(minbr, maxbr));
    delay(del);
    analogWrite(channel1PinR, 0);
    delay(del);
  }
  
  for(int i=3; i>0; i--){
    analogWrite(channel1PinB, random(minbr, maxbr));
    delay(del);
    analogWrite(channel1PinB, 0);
    delay(del);
  }
}

