// HX711 service: periodic sampling + simple smoothing + web/MQTT hooks
// --------------------------------------------------------------------
#pragma once

#include <Arduino.h>

namespace HxService {
  // Default pins (change via build flags or define before include)
  #ifndef HX711_DOUT_PIN
  #define HX711_DOUT_PIN 32
  #endif
  #ifndef HX711_SCK_PIN
  #define HX711_SCK_PIN 33
  #endif

  // Initialize HX711 on given pins; optional gain (128 default) and initial scale (counts per unit)
  void begin(uint8_t dout = HX711_DOUT_PIN, uint8_t sck = HX711_SCK_PIN, uint8_t gain = 128, float scale = 1.0f);

  // Call from loop(); reads when data ready; publishes MQTT at an interval if connected.
  void loop();

  // Set tare to current reading (average N samples quickly)
  bool tare(uint8_t samples = 5);

  // Update scale (counts per unit; e.g., counts per gram). Use positive, non-zero value.
  void setScale(float scale);

  // Return latest values; ageMs is milliseconds since last update (UINT32_MAX if never).
  void get(float& units, int32_t& raw, uint32_t& ageMs);

  // Retrieve current configuration
  float currentScale();
  int32_t currentOffset();
}

