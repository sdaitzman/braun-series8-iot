#include <Arduino.h>

unsigned long next_pressed = 0;

void setup() {
  Serial.begin(115200);
  delay(800);
  pinMode(GPIO_NUM_25, OUTPUT);
  digitalWrite(GPIO_NUM_25, LOW);
}

void loop() {
  unsigned long time = millis();
  Serial.print(time);
  Serial.print("\t");
  pinMode(GPIO_NUM_25, INPUT_PULLUP);
  delay(5);
  Serial.print(digitalRead(GPIO_NUM_25));
  pinMode(GPIO_NUM_25, OUTPUT);
  Serial.println("\tHello Braun Series 8 Dock Event Loop...");
  if(time > next_pressed) {
    Serial.print(time);
    Serial.println("\tPressing that button...");
    next_pressed += 10000;
    digitalWrite(GPIO_NUM_25, HIGH);
    delay(800);
    digitalWrite(GPIO_NUM_25, LOW);
  }
  delay(500);
}