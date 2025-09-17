// Minimal Web server wrapper for LED control
#pragma once

namespace Web {
  // Start a Wi‑Fi SoftAP and HTTP server (serves /index.html from LittleFS)
  void beginAp(const char* ssid, const char* pass);

  // Pump web events; call from loop()
  void loop();
}

