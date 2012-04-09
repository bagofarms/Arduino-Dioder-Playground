/* Serial Time Updater for Dioder Arduino Project */

import processing.serial.*;

Serial port;

int brightness = 0;

void setup(){
  println("Connected to:  "+Serial.list()[0]);
  port = new Serial(this, Serial.list()[0], 9600);
  port.bufferUntil(1);
  
}

void draw(){
  
}

void serialEvent(Serial port){
  println(second());
  port.write(second());
}
