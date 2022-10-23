#include "config.h"
#include <Arduino.h>

// Manual button press read status.
bool button_is_pressed = 0;

// Debug loop parameters
#define DEBUG_LOOP 0
#define DEBUG_LOOP_PERIOD 10*1000
unsigned long debug_next_pressed = 0;

// Pin assignments
#define ISESP32 // ISESP32 or ISESP8266

// These are for Wemos S2 Mini, which fits comfortably inside.
#ifdef ISESP32
  #define BRAUN_BUTTON_PIN GPIO_NUM_4 // E.g. GPIO_NUM_25 for ESP32-WROOM-32
  #define DOCK_STATUS_PIN  GPIO_NUM_8 // E.g. GPIO_NUM_33 for ESP32-WROOM-32
  #define CONTROLLER_LED_PIN 15       // For Wemos S2 Mini
#endif

#ifdef ISESP8266
  #define BRAUN_BUTTON_PIN 4               
  #define DOCK_STATUS_PIN A0
#endif

// Is the shaver currently docked? Is the center LED on?
bool shaver_docked_pin_raw = 0;
int shaver_docked_confidence = 10; // -5:20
bool shaver_is_docked = 0; // docked if confidence > 10

// Button press function and tracker
unsigned long button_last_pressed = 0;
unsigned long last_cleaned = 0;
unsigned long due_for_clean = 0;

void button_press() {
  button_last_pressed = millis();
  // TODO: more intelligent logic that accounts for button presses that just
  // *stop* the ongoing cleaning... or read from motor leads.
  if(shaver_is_docked) last_cleaned = button_last_pressed;

  // Blink LED off, if present, and press button
  #ifdef CONTROLLER_LED_PIN
    analogWrite(CONTROLLER_LED_PIN, 0);
  #endif
  digitalWrite(BRAUN_BUTTON_PIN, HIGH);
  delay(800);
  digitalWrite(BRAUN_BUTTON_PIN, LOW);
  #ifdef CONTROLLER_LED_PIN
    analogWrite(CONTROLLER_LED_PIN, 512);
  #endif
}

void setup() {
  // Set initial pin statuses...
  pinMode(BRAUN_BUTTON_PIN, OUTPUT);
  digitalWrite(BRAUN_BUTTON_PIN, LOW);
  pinMode(DOCK_STATUS_PIN, INPUT);
  #ifdef CONTROLLER_LED_PIN
    pinMode(CONTROLLER_LED_PIN, OUTPUT);
    analogWrite(CONTROLLER_LED_PIN, 512);
  #endif

  // Startup can trigger false press. Uncomment to cancel that false press.
  // button_press();

  // Begin Serial communications. Wait 100ms to stabilize.
  Serial.begin(115200);
  delay(100);

}

void loop() {
  unsigned long time = millis(); // Get the current time
  
  // Make this read so brief (5ms) that the unit ignores the erroneous press...
  pinMode(BRAUN_BUTTON_PIN, INPUT_PULLUP);
  delay(5);
  button_is_pressed = !digitalRead(BRAUN_BUTTON_PIN);
  pinMode(BRAUN_BUTTON_PIN, OUTPUT);

  // Get current dock status from top-right pin on dock PCB.
  #ifdef ISESP32
    shaver_docked_pin_raw = analogReadMilliVolts(DOCK_STATUS_PIN) > 300;
  #elif ISESP8266
    shaver_docked_pin_raw = analogRead(DOCK_STATUS_PIN) > 90;
  #endif

  // Update shaver docked confidence and status.
  if(shaver_docked_pin_raw && shaver_docked_confidence<20) shaver_docked_confidence++;
  if(!shaver_docked_pin_raw &&shaver_docked_confidence>-5) shaver_docked_confidence--;
  shaver_is_docked = 1 ? shaver_docked_confidence > 10 : 0;

  // If the last clean was at least N ms ago, and shaver is not docked...
  // OR if we are in the first M ms of power, and shaver is not docked...
  // Then we are due for a clean O seconds after the shaver is docked again!
  if(
    ( last_cleaned+1000*60*60<time || time<1000*60*60 )
    && !shaver_is_docked
  ) due_for_clean = time + 1000*30;

  // If we're due for a clean, clean!
  if(due_for_clean && time > due_for_clean) {
    Serial.println("CLEANING DUE!!! ATTEMPTING IT NOW. ");
    due_for_clean = 0;
    button_press();
  }
  
  // If debug loop (press button every 10 seconds) is enabled, run this block.
  if(DEBUG_LOOP && time > debug_next_pressed) {
    // Print the time again, and that we're going to press the button.
    Serial.print(time);
    Serial.println("Debug loop is enabled. Pressing the button! ");
    // Set the next button press time.
    debug_next_pressed += DEBUG_LOOP_PERIOD;
    // Press the button
    button_press();
  }

  // If the physical button has been manually pressed, pass that signal through.
  if(button_is_pressed) {
    Serial.println("Passing through detected manual button press. ");
    button_press();
  }

  // Log everything important about dock status
  Serial.print(time);
  Serial.print("\t");
  Serial.print("Btn:" + String(button_is_pressed));
  // Serial.print("\tDocked_RAW:" + String(shaver_docked_pin_raw));
  // Serial.print("\tDocked_Confidence:" + String(shaver_docked_confidence));
  Serial.print("\tDocked?:" + String(shaver_is_docked));
  Serial.print("\tLast_Cleaned:" + String(last_cleaned));
  Serial.print("\tDue_For_Clean:" + String(due_for_clean));
  // Serial.print("\tButton_Last_Pressed:" + String(button_last_pressed));
  Serial.println("\tHello Braun Series 8 Dock Event Loop. ");
  

  // Wait before the next event loop runthrough.
  delay(25);
}