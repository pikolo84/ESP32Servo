#include <arduino.h>
#include <ESP32_Servo.h>

// create four servo objects 
Servo servo1(19,0);
Servo servo2(18,1); 

void setup()
{ 
  Serial.begin(115200);
  Serial.println("SERVO TEST");
  servo1.attach();
  servo2.attach();   
}

void loop() {
  servo1.writeMicroseconds(1000);
  servo2.writeMicroseconds(2000);
  Serial.printf("Servo 1: %i, Servo 2: %i\n", servo1.readMicroseconds(), servo2.readMicroseconds());
  delay(2000);
  servo1.writeMicroseconds(2000);
  servo2.writeMicroseconds(1000);
  Serial.printf("Servo 1: %i, Servo 2: %i\n", servo1.readMicroseconds(), servo2.readMicroseconds());
  delay(2000);

}

