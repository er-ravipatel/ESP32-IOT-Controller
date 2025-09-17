// Minimal: turn on the onboard LED and keep it on
#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for ESP32 DevKit boards
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // Use LOW if your LED is active‑low
}

void loop() {
  // Nothing to do
}
