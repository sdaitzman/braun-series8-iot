#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

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

void setLED(int brightness) {
  #ifdef CONTROLLER_LED_PIN
    analogWrite(CONTROLLER_LED_PIN, brightness);
  #endif
}

void button_press() {
  button_last_pressed = millis();
  // TODO: more intelligent logic that accounts for button presses that just
  // *stop* the ongoing cleaning... or read from motor leads.
  if(shaver_is_docked) last_cleaned = button_last_pressed;

  // Blink LED off, if present, and press button
  setLED(0);
  digitalWrite(BRAUN_BUTTON_PIN, HIGH);
  delay(800);
  digitalWrite(BRAUN_BUTTON_PIN, LOW);
  setLED(512);
}


void setup() {
  // Set initial pin statuses...
  pinMode(BRAUN_BUTTON_PIN, OUTPUT);
  digitalWrite(BRAUN_BUTTON_PIN, LOW);
  pinMode(DOCK_STATUS_PIN, INPUT);
  #ifdef CONTROLLER_LED_PIN
    pinMode(CONTROLLER_LED_PIN, OUTPUT);
  #endif
  setLED(512);

  // Startup can trigger false press. Uncomment to cancel that false press.
  // button_press();

  // Begin Serial communications. Wait for serial connection to stabilize.
  Serial.begin(115200);
  for(int i = 0; i < 100; i++) {
    delay(25);
    Serial.print(".");
  }

  // Attempt network connection
  setLED(0);
  Serial.println("Will attempt to connect to network SSID: " + String(NETWORK_SSID));
  delay(500);
  WiFi.mode(WIFI_STA);
  Serial.println("Set WiFI mode to WIFI_STA");
  delay(500);
  WiFi.begin(NETWORK_SSID, NETWORK_PSK);
  delay(500);
  Serial.println("Began WIFi Connection...");
  delay(500);
  while(WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi connection failed~ Rebooting...");
    delay(5000);
    ESP.restart(); // TODO: rather than rebooting, continue and normal and attempt reconnect later
  }
  Serial.println("Setting hostname...");
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
  Serial.println("WiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Hostname: " + String(DEVICE_HOSTNAME));
  delay(200);
  ArduinoOTA.onStart([]() {
    String type;
    if(ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    Serial.println("Updating: " + type);
  }).onEnd([]() {
    Serial.println("\nEnd OTA!");
  }).onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  }).onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle(); // Handle OTA if needed...
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
  Serial.print("\tIP: ");
  Serial.print(WiFi.localIP());
  Serial.print("\tHostname: " + String(DEVICE_HOSTNAME));
  Serial.println("\tHello Braun Series 8 Dock Event Loop. ");
  

  // Wait before the next event loop runthrough.
  setLED(0);
  delay(25);
  setLED(512);
}