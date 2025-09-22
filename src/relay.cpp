// Minimal Relay control implementation
#include "relay.h"

namespace {
  int  g_pin = RELAY_PIN;
  bool g_activeHigh = (RELAY_ACTIVE_HIGH != 0);
  bool g_state = false;

  inline void writeRelay(bool on) {
    digitalWrite(g_pin, (on ^ !g_activeHigh) ? HIGH : LOW);
  }
}

namespace Relay {
  void begin(int pin, bool activeHigh) {
    g_pin = pin; g_activeHigh = activeHigh; g_state = false;
    pinMode(g_pin, OUTPUT);
    writeRelay(false);
  }

  void set(bool on) { g_state = on; writeRelay(on); }
  bool get() { return g_state; }
  void toggle() { set(!get()); }
  int  pin() { return g_pin; }
  bool activeHigh() { return g_activeHigh; }
}

