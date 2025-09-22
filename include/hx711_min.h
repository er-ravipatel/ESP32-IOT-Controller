// Minimal HX711 driver (header-only)
// ----------------------------------
// Non-blocking friendly: call available() and only read when data ready.
// Provides raw 24-bit measurement, tare (offset), and scale for engineering units.
#pragma once

#include <Arduino.h>

class HX711Min {
 public:
  // gain: 128 (A), 64 (A), or 32 (B). Default 128.
  void begin(uint8_t pinDout, uint8_t pinSck, uint8_t gain = 128) {
    dout_ = pinDout; sck_ = pinSck; setGain(gain);
    pinMode(dout_, INPUT);
    pinMode(sck_, OUTPUT); digitalWrite(sck_, LOW);
  }

  // True when a new sample is ready on DOUT (goes LOW when ready)
  inline bool available() const { return digitalRead(dout_) == LOW; }

  // Read one raw 24-bit signed sample. Waits up to timeoutMs for readiness.
  // Returns true on success; 'out' receives sign-extended value.
  bool readRaw(int32_t& out, uint32_t timeoutMs = 50) {
    uint32_t start = millis();
    while (!available()) { if (millis() - start >= timeoutMs) return false; delayMicroseconds(50); }
    // Read 24 bits MSB first
    uint32_t data = 0;
    noInterrupts();
    for (int i = 0; i < 24; ++i) {
      digitalWrite(sck_, HIGH);
      // minimum high time ~1 us
      delayMicroseconds(1);
      data = (data << 1) | (digitalRead(dout_) ? 1u : 0u);
      digitalWrite(sck_, LOW);
      delayMicroseconds(1);
    }
    // Set gain for next conversion by additional pulses (1 for 128, 3 for 64, 2 for 32)
    for (int i = 0; i < gainPulses_; ++i) {
      digitalWrite(sck_, HIGH); delayMicroseconds(1);
      digitalWrite(sck_, LOW);  delayMicroseconds(1);
    }
    interrupts();
    // Sign extend 24-bit to 32-bit
    if (data & 0x800000) data |= 0xFF000000; // negative
    out = static_cast<int32_t>(data);
    lastRaw_ = out;
    return true;
  }

  // Convenience: convert to units using offset + scale
  bool readUnits(float& out, uint32_t timeoutMs = 50) {
    int32_t r; if (!readRaw(r, timeoutMs)) return false; out = toUnits(r); return true;
  }

  // Set tare offset to current average of N readings (non-strict, uses blocking quickly)
  bool tare(uint8_t samples = 5, uint32_t timeoutPer = 60) {
    int64_t acc = 0; uint8_t ok = 0; int32_t v = 0;
    for (uint8_t i = 0; i < samples; ++i) { if (readRaw(v, timeoutPer)) { acc += v; ++ok; } }
    if (!ok) return false; offset_ = static_cast<int32_t>(acc / ok); return true;
  }

  // Configure a scale factor such that units = (raw - offset)/scale.
  void setScale(float scale) { scale_ = (scale == 0.f) ? 1.f : scale; }
  float scale() const { return scale_; }
  void setOffset(int32_t off) { offset_ = off; }
  int32_t offset() const { return offset_; }

  // Convert a raw reading to user units using current offset/scale.
  inline float toUnits(int32_t raw) const { return (static_cast<float>(raw - offset_) / scale_); }

  // Configure gain: 128 (A), 64 (A), 32 (B)
  void setGain(uint8_t gain) {
    switch (gain) {
      case 128: gainPulses_ = 1; break; // channel A, gain 128
      case 64:  gainPulses_ = 3; break; // channel A, gain 64
      case 32:  gainPulses_ = 2; break; // channel B, gain 32
      default:  gainPulses_ = 1; break;
    }
  }

  int32_t lastRaw() const { return lastRaw_; }

 private:
  uint8_t dout_ = 0, sck_ = 0;
  uint8_t gainPulses_ = 1; // default A gain 128
  int32_t offset_ = 0;     // tare
  float   scale_  = 1.0f;  // counts per unit (e.g., counts per gram)
  int32_t lastRaw_ = 0;
};

