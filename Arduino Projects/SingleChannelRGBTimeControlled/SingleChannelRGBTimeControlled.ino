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
  analogWrite(channel1PinG, 0);
  analogWrite(channel1PinB, 0);
  
  for(int brightness = 0; brightness <= 255; brightness = brightness++){
    analogWrite(channel1PinR, brightness);
    delay(5);
  }
  for(int brightness = 254; brightness >= 0; brightness--){
    analogWrite(channel1PinR, brightness);
    delay(5);
  }
}

