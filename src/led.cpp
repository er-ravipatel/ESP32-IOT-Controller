#include "led.h"

namespace {
  uint8_t s_pin = 255;
  bool s_activeHigh = true;
  bool s_on = false;

  inline void writeLevel(bool on) {
    // active-high: on->HIGH, off->LOW; active-low: inverse
    digitalWrite(s_pin, (on ? s_activeHigh : !s_activeHigh) ? HIGH : LOW);
  }
}

namespace Led {
  void begin(uint8_t pin, bool activeHigh) {
    s_pin = pin;
    s_activeHigh = activeHigh;
    pinMode(s_pin, OUTPUT);
    s_on = false;
    writeLevel(false);
  }

  void on()    { s_on = true;  writeLevel(true); }
  void off()   { s_on = false; writeLevel(false); }
  void toggle(){ s_on = !s_on; writeLevel(s_on); }
  bool isOn()  { return s_on; }
}


