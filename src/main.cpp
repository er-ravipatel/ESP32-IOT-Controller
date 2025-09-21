// Minimal: connect ESP32 to Wi-Fi and drive LED
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for common ESP32 dev boards
#endif

// If your onboard LED is active-low, set this to false
static const bool LED_ACTIVE_HIGH = true;

static inline void writeLed(bool on) {
  digitalWrite(LED_BUILTIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW);
}

// Connect with timeout and simple logs
static bool connectWiFi(uint32_t timeoutMs = 20000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to '%s'", WIFI_SSID);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    Serial.print('.');
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  }
  Serial.println("\nWi-Fi connect timed out.");
  return false;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  writeLed(false);  // start off

  Serial.begin(115200);
  delay(50);
  Serial.println();
  Serial.println(F("ESP32 Wi-Fi + LED status"));

  // Initial connect; loop() will keep retrying
  connectWiFi();
}

void loop() {
  static uint32_t lastAttempt = 0;

  // Reconnect every 5s if needed
  if (WiFi.status() != WL_CONNECTED) {
    uint32_t now = millis();
    if (now - lastAttempt > 5000) {
      Serial.println(F("Reconnecting to Wi-Fi..."));
      connectWiFi(8000);
      lastAttempt = now;
    }
  }

  // LED behavior
  static bool blinkState = false;              // remembers current LED state while blinking
  static uint32_t lastToggle = 0;              // last toggle time
  const uint32_t BLINK_INTERVAL_MS = 167;      // ~3 blinks per second (3 Hz)

  if (WiFi.status() == WL_CONNECTED) {
    // Solid ON when connected
    writeLed(true);
    blinkState = true; // keep internal state in sync
  } else {
    // Not connected: blink at ~3 Hz (toggle about every 167 ms)
    uint32_t now = millis();
    if (now - lastToggle >= BLINK_INTERVAL_MS) {
      blinkState = !blinkState;
      writeLed(blinkState);
      lastToggle = now;
    }
  }

  delay(10);
}
