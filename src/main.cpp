#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(800);
  pinMode(GPIO_NUM_25, OUTPUT);
}

void loop() {
  Serial.print(millis());
  Serial.println("\tHello Braun Series 8");
  delay(1000)
}