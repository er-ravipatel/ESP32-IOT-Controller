// Minimal Relay control for ESP32
// -------------------------------
// Encapsulates a single relay on a GPIO with configurable polarity.
#pragma once

#include <Arduino.h>

namespace Relay {
  // Defaults (override with -DRELAY_PIN=xx and -DRELAY_ACTIVE_HIGH=0/1)
  #ifndef RELAY_PIN
  #define RELAY_PIN 26
  #endif
  #ifndef RELAY_ACTIVE_HIGH
  #define RELAY_ACTIVE_HIGH 1
  #endif

  void begin(int pin = RELAY_PIN, bool activeHigh = (RELAY_ACTIVE_HIGH != 0));
  void set(bool on);
  bool get();
  void toggle();
  int  pin();
  bool activeHigh();
}

