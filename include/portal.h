// Minimal Wi‑Fi config portal over SoftAP
#pragma once

namespace Portal {
  // Start AP + HTTP server for configuration.
  // AP SSID/password can be null to use defaults (ESP32-Setup-XXXX / 12345678).
  void begin(const char* apSsid = nullptr, const char* apPass = nullptr);

  // Service incoming HTTP requests; call from loop() when running.
  void loop();

  // Stop the portal and AP.
  void stop();

  // Is portal running?
  bool running();

  // Returns true if new credentials were saved; resets the flag.
  bool hasNewCredentials();

  // LED control helpers (for cross-module control, e.g., keypad triggers)
  void setLed(bool on);
  bool getLed();
}

