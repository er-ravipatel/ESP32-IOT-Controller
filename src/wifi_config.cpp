// Wi‑Fi credentials persisted in NVS (Preferences)
#include "wifi_config.h"
#include <Preferences.h>

namespace {
  Preferences prefs;
  const char* NS = "wifi"; // namespace
}

namespace WifiConfig {
  bool load(String& ssid, String& pass) {
    if (!prefs.begin(NS, /*readOnly=*/true)) return false;
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
    return ssid.length() > 0; // consider valid only if SSID present
  }

  bool save(const String& ssid, const String& pass) {
    if (!prefs.begin(NS, /*readOnly=*/false)) return false;
    bool ok1 = prefs.putString("ssid", ssid) > 0 || ssid.length() == 0;
    bool ok2 = prefs.putString("pass", pass) >= 0;
    prefs.end();
    return ok1 && ok2;
  }

  void clear() {
    if (!prefs.begin(NS, /*readOnly=*/false)) return;
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
  }
}
