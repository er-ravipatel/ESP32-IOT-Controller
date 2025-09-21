// Minimal: connect ESP32 to Wi-Fi and drive LED
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"
#include "portal.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for common ESP32 dev boards
#endif

// If your onboard LED is active-low, set this to false
static const bool LED_ACTIVE_HIGH = true;

static inline void writeLed(bool on) {
  digitalWrite(LED_BUILTIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW);
}

// Connect with timeout and simple logs
static bool connectWiFi(const String& ssid, const String& pass, uint32_t timeoutMs = 20000) {
  // Keep AP running if already active (AP or AP+STA)
  wifi_mode_t m = WiFi.getMode();
  if (m == WIFI_AP || m == WIFI_AP_STA) WiFi.mode(WIFI_AP_STA); else WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("Connecting to '%s'", ssid.c_str());
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
  // Start setup portal (AP) so user can connect immediately
  Portal::begin(nullptr, nullptr);

  // Try saved credentials; if none or fails, start config portal
  String ssid, pass;
  if (WifiConfig::load(ssid, pass)) {
    if (!connectWiFi(ssid, pass)) {
      Serial.println(F("Starting config portal (failed to connect)"));
      /* portal running */ (void)0;
    }
  } else {
    Serial.println(F("No saved Wi‑Fi. Starting config portal."));
    /* portal running */ (void)0;
  }
}

void loop() {
  static uint32_t lastAttempt = 0;

  // Reconnect every 5s if needed (when not in portal)
  if (WiFi.status() != WL_CONNECTED && !Portal::running()) {
    uint32_t now = millis();
    if (now - lastAttempt > 5000) {
      Serial.println(F("Reconnecting to Wi-Fi..."));
      String ssid, pass;
      if (WifiConfig::load(ssid, pass))
        connectWiFi(ssid, pass, 8000);
      lastAttempt = now;
    }
  }

  // Run portal if active, and attempt connect when new creds saved
  if (Portal::running()) {
    Portal::loop();
    if (Portal::hasNewCredentials()) {
      String ssid, pass;
      if (WifiConfig::load(ssid, pass)) {
        Serial.println(F("Credentials saved. Trying to connect..."));
        if (connectWiFi(ssid, pass, 15000)) {
          Serial.println(F("Connected. Portal remains active."));
        } else {
          Serial.println(F("Connect failed. Portal stays open."));
        }
      }
    }
  }

  // LED behavior
  static bool blinkState = false;              // remembers current LED state while blinking
  static uint32_t lastToggle = 0;              // last toggle time
  const uint32_t BLINK_INTERVAL_MS = 167;      // ~3 blinks per second (3 Hz)

  if (WiFi.status() == WL_CONNECTED) {
    // Connected: controller (web) can manage LED; no blink here
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






