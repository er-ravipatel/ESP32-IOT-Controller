// Multiple Relay control for ESP32 (N channels)
#pragma once

#include <Arduino.h>

#ifndef RELAY_ACTIVE_HIGH
#define RELAY_ACTIVE_HIGH 1
#endif

// Optional pin macros for convenience (override in build_flags)
#ifndef RELAY1_PIN
#define RELAY1_PIN 26
#endif
#ifndef RELAY2_PIN
#define RELAY2_PIN -1
#endif
#ifndef RELAY3_PIN
#define RELAY3_PIN -1
#endif
#ifndef RELAY4_PIN
#define RELAY4_PIN -1
#endif

namespace Relays {
  // Initialize multiple relays. Pass an array of pins; entries < 0 are ignored.
  // All channels use the same polarity (activeHigh) which matches common 4-channel boards.
  void begin(const int* pins, size_t n, bool activeHigh = (RELAY_ACTIVE_HIGH != 0));

  // Number of configured channels (pins >= 0 passed to begin)
  size_t count();

  // 0-based channel accessors
  void set(size_t index, bool on);
  bool get(size_t index);
  void toggle(size_t index);
  int  pin(size_t index);
  bool activeHigh(size_t index);
}

