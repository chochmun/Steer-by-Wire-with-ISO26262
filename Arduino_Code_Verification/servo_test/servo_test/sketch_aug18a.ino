#include <Servo.h>

Servo myservo;

void setup() {
  Serial.begin(9600);
  myservo.attach(9);
  myservo.write(0);
  delay(100);
  
}

void loop() {
  
  char c = Serial.read();
  
  
  
  
  int data = analogRead(A0);
  Serial.print(",");
  Serial.println(data);
  

  
}