// Multiple Relay control implementation
#include "relays.h"

namespace {
  constexpr size_t kMaxRelays = 8;
  int   g_pins[kMaxRelays];
  bool  g_activeHigh[kMaxRelays];
  bool  g_state[kMaxRelays];
  size_t g_count = 0;

  inline void writeRelay(size_t idx, bool on) {
    if (idx >= g_count) return;
    digitalWrite(g_pins[idx], (on ^ !g_activeHigh[idx]) ? HIGH : LOW);
  }
}

namespace Relays {
  void begin(const int* pins, size_t n, bool activeHigh) {
    g_count = 0;
    for (size_t i = 0; i < n && g_count < kMaxRelays; ++i) {
      int p = pins[i];
      if (p < 0) continue;
      g_pins[g_count] = p;
      g_activeHigh[g_count] = activeHigh;
      g_state[g_count] = false;
      pinMode(p, OUTPUT);
      writeRelay(g_count, false);
      ++g_count;
    }
  }

  size_t count() { return g_count; }
  void set(size_t index, bool on) { if (index < g_count) { g_state[index] = on; writeRelay(index, on); } }
  bool get(size_t index) { return (index < g_count) ? g_state[index] : false; }
  void toggle(size_t index) { set(index, !get(index)); }
  int  pin(size_t index) { return (index < g_count) ? g_pins[index] : -1; }
  bool activeHigh(size_t index) { return (index < g_count) ? g_activeHigh[index] : (RELAY_ACTIVE_HIGH != 0); }
}

