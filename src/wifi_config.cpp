// Wi‑Fi credentials persisted in NVS (Preferences)
// ------------------------------------------------
// Simple helpers to load/save Wi‑Fi SSID/password using the ESP32
// Preferences API. Values survive across reboots and power cycles.
#include "wifi_config.h"
#include <Preferences.h>

namespace {
  Preferences prefs;
  const char* NS = "wifi"; // namespace
}

namespace WifiConfig {
  // Load saved SSID/PASS (returns true if an SSID exists)
  bool load(String& ssid, String& pass) {
    if (!prefs.begin(NS, /*readOnly=*/true)) return false;
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
    return ssid.length() > 0; // consider valid only if SSID present
  }

  // Save SSID/PASS (overwrites existing values)
  bool save(const String& ssid, const String& pass) {
    if (!prefs.begin(NS, /*readOnly=*/false)) return false;
    bool ok1 = prefs.putString("ssid", ssid) > 0 || ssid.length() == 0;
    bool ok2 = prefs.putString("pass", pass) >= 0;
    prefs.end();
    return ok1 && ok2;
  }

  // Remove stored credentials
  void clear() {
    if (!prefs.begin(NS, /*readOnly=*/false)) return;
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
  }
}
