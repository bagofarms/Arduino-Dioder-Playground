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
  pinMode(thisPin, OUTPUT); 
  analogWrite(thisPin, 255);
  
  appearToHaveValidMessage = false;

  // initialize the serial communication:
  Serial.begin(57600);
}


void loop () {
  
  int availableBytes = Serial.available();
  
  if (!appearToHaveValidMessage) {
    
    // If we haven't found a header yet, look for one.
    if (availableBytes >= kProtocolHeaderLength) {
      
      // Read then peek in case we're only one byte away from the header.
      byte firstByte = Serial.read();
      byte secondByte = Serial.peek();
      
      if (firstByte == kProtocolHeaderFirstByte &&
          secondByte == kProtocolHeaderSecondByte) {
            
          // We have a valid header. We might have a valid message!
          appearToHaveValidMessage = true;
          
          // Read the second header byte out of the buffer and refresh the buffer count.
          Serial.read();
          availableBytes = Serial.available();
      }
    }
  }
  
  if (availableBytes >= (kProtocolBodyLength + kProtocolChecksumLength) && appearToHaveValidMessage) {
     
    // Read in the body, calculating the checksum as we go.
    byte calculatedChecksum = 0;
    
    for (int i = 0; i < kProtocolBodyLength; i++) {
      receivedMessage[i] = Serial.read();
      calculatedChecksum ^= receivedMessage[i];
    }
    
    byte receivedChecksum = Serial.read();
    
    if (receivedChecksum == calculatedChecksum) {
      // Hooray! Push the values to the output pins.
      for (int i = 0; i < kProtocolBodyLength; i++) {
        analogWrite(i + kChannel1FirstPin, receivedMessage[i]);
      }
      
      Serial.print("OK");
      Serial.write(byte(10));
      
    } else {
      
      Serial.print("FAIL");
      Serial.write(byte(10));
    }
    
    appearToHaveValidMessage = false;
  }
}

