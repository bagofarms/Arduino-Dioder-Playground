/* Serial Time Updater for Dioder Arduino Project */

import processing.serial.*;

Serial port;

int brightness = 0;

void setup(){
  size(200,200);
  port = new Serial(this, Serial.list()[0], 9600);
  port.bufferUntil('\n');
  
}

void draw(){
  background(0,0,brightness);
}

void serialEvent(Serial port){
  println(port.readStringUntil('\n'));
}
