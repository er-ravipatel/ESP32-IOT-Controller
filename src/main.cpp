// Modular minimal Web LED controller
#include <Arduino.h>
#include "led.h"
#include "web.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Fallback for typical ESP32 DevKit boards
#endif

// SoftAP credentials (connect and open http://192.168.4.1)
static const char* AP_SSID = "ESP32-LED";
static const char* AP_PASS = "12345678"; // >= 8 chars

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("ESP32 Web LED (modular)"));

  // Initialize LED module: change 'true' to 'false' if your LED is active-low
  Led::begin(LED_BUILTIN, /*activeHigh=*/true);

  // Start Wi‑Fi AP + HTTP server; serves /index.html from LittleFS
  Web::beginAp(AP_SSID, AP_PASS);
}

void loop() {
  Web::loop();
}


