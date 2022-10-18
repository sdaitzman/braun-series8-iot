#include <Arduino.h>

// Manual button press read status.
bool button_is_pressed = 0;

// Next time to send buttonpress signal.
unsigned long next_pressed = 0;

// Should we run a debug loop that presses the button every 10 seconds?
#define DEBUG_LOOP 1

// Pin assignments for LED read and button read/write
#define BRAUN_BUTTON_PIN GPIO_NUM_25
#define CENTER_LED_PIN GPIO_NUM_34

void setup() {
  // Begin Serial communications. Wait 800ms to stabilize.
  Serial.begin(115200);
  delay(800);

  // Set ESP32 pin 25 to output, and turn it off.
  pinMode(BRAUN_BUTTON_PIN, OUTPUT);
  digitalWrite(BRAUN_BUTTON_PIN, LOW);
}

void loop() {
  // Get and print the current time for logging purposes
  unsigned long time = millis();
  Serial.print(time);
  Serial.print("\t");

  // We want to check if the button is currently being pressed by the user.
  // Switching to INPUT mode will cause the Braun Series 8 dock to read a press.
  // So, we make it such a brief press (5ms) that the unit simply ignores it.
  // Then switch back to the regular OUTPUT mode.
  pinMode(BRAUN_BUTTON_PIN, INPUT_PULLUP);
  delay(5);
  button_is_pressed = !digitalRead(BRAUN_BUTTON_PIN);
  Serial.print(button_is_pressed);
  pinMode(BRAUN_BUTTON_PIN, OUTPUT);

  // Start out our event loop...
  Serial.println("\tHello Braun Series 8 Dock Event Loop...");
  
  // If debug loop (press button every 10 seconds) is enabled, run this block.
  if(DEBUG_LOOP && time > next_pressed) {
    
    // Print the time again, and that we're going to press the button.
    Serial.print(time);
    Serial.println("\tPressing that button...");
    
    // Set the next button press time to 10 seconds in the future.
    next_pressed += 10000;

    // Press the button for 0.8 seconds.
    digitalWrite(GPIO_NUM_25, HIGH);
    delay(800);
    digitalWrite(GPIO_NUM_25, LOW);
  }

  // If the physical button has been manually pressed, pass that signal through.
  if(button_is_pressed) {
    Serial.println("Passing through manual button press.");
    digitalWrite(BRAUN_BUTTON_PIN, HIGH);
    delay(800);
    digitalWrite(BRAUN_BUTTON_PIN, LOW);
  }

  // Wait before the next event loop runthrough.
  delay(50);
}