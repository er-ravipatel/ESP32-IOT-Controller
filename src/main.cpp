// Minimal: connect ESP32 to Wi‑Fi and drive LED
// ------------------------------------------------
// What this sketch does
// - Brings up a configuration portal (SoftAP + captive DNS) so users
//   can enter Wi‑Fi credentials on first boot.
// - Attempts to connect to saved Wi‑Fi credentials on boot.
// - Reflects connection status on the onboard LED (blink when offline).
// - Leaves the web server running so /setup.html and /localcontroller.html
//   are reachable over AP (when active) and STA (when connected).
//
// This file is intentionally minimal and well‑commented; no MQTT or other
// features are included to keep the flow easy to understand.
// ------------------------------------------------
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"
#include "portal.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for common ESP32 dev boards
#endif

// LED polarity
// Some ESP32 boards wire the LED as active‑low. If your LED turns ON when you
// write LOW, set this to false.
static const bool LED_ACTIVE_HIGH = true;

// LED driver (tiny helper)
// Drives the onboard LED respecting the configured polarity flag above.
static inline void writeLed(bool on) {
  digitalWrite(LED_BUILTIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW);
}

// Wi‑Fi connect helper
// Tries to connect to the given SSID/PASS and waits up to timeoutMs.
// If the setup AP is already running, we keep it alive (AP+STA mode)
// so users can still access the portal while STA negotiates.
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
  // Start the setup portal
  // Brings up the SoftAP + captive DNS; users can join the AP and
  // visit /setup.html to scan and save Wi‑Fi credentials.
  Portal::begin(nullptr, nullptr);

  // Try saved credentials; if none or connect fails, keep the portal up
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

  // Background: reconnect every 5s if STA is down and portal is not running
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

  // Service the portal and apply credentials when the user saves them
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

  // LED: blink while not connected
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

  // Small idle delay to keep the loop yielding
  delay(10);
}







