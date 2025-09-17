// Minimal sketch: turn the onboard LED on
#include <Arduino.h>

constexpr uint8_t LED_PIN = 2; // Onboard LED on most ESP32 dev kits

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Turn LED on and keep it on
}

void loop() {
  // Nothing to do
}

