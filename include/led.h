// Simple LED helper (minimal and reusable)
#pragma once
#include <Arduino.h>

namespace Led {
  // Call once: choose pin and polarity (true = active-high, false = active-low)
  void begin(uint8_t pin, bool activeHigh = true);

  // Basic control
  void on();
  void off();
  void toggle();

  // Query
  bool isOn();
}

