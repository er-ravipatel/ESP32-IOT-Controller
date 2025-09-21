// Wi‑Fi configuration helpers (persist credentials in NVS)
#pragma once
#include <Arduino.h>

namespace WifiConfig {
  // Load saved credentials from NVS (returns true if present)
  bool load(String& ssid, String& pass);

  // Save credentials to NVS (overwrites existing)
  bool save(const String& ssid, const String& pass);

  // Remove saved credentials
  void clear();
}
