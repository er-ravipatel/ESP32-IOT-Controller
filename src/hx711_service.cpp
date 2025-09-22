// HX711 background sampler + simple smoothing and MQTT publishing
#include <Arduino.h>
#include "hx711_min.h"
#include "hx711_service.h"
#include "mqtt_min.h"
#include "mqtt_config.h"

namespace {
  HX711Min g_hx;
  bool     g_inited = false;
  uint32_t g_lastUpdateMs = 0;
  int32_t  g_lastRaw = 0;
  float    g_lastUnits = 0.0f;

  // Smoothing factor (EMA): 0..1. Higher = faster response, lower = smoother.
  const float kAlpha = 0.20f;
  bool     g_hasSmoothed = false;

  // MQTT publish cadence (ms)
  const uint32_t kPubEveryMs = 800; // ~1.25 Hz
  uint32_t g_lastPubMs = 0;

  // Utility: build compact JSON without ArduinoJson to keep deps minimal
  static String jsonReading(float units, int32_t raw) {
    String s = "{";
    s += "\"value\":"; s += String(units, 3);
    s += ",\"raw\":";   s += String(raw);
    s += ",\"unit\":\""; s += "units"; s += "\""; // rename on UI if you set scale to grams, etc.
    s += "}";
    return s;
  }
}

namespace HxService {
  void begin(uint8_t dout, uint8_t sck, uint8_t gain, float scale) {
    g_hx.begin(dout, sck, gain);
    g_hx.setScale(scale <= 0.f ? 1.f : scale);
    g_inited = true;
    g_lastUpdateMs = 0; g_lastPubMs = 0; g_hasSmoothed = false;
  }

  void loop() {
    if (!g_inited) return;
    int32_t r;
    if (g_hx.readRaw(r, 2)) { // quick read if ready
      g_lastRaw = r;
      float u = g_hx.toUnits(r);
      if (g_hasSmoothed) {
        g_lastUnits = (1.0f - kAlpha) * g_lastUnits + kAlpha * u;
      } else {
        g_lastUnits = u; g_hasSmoothed = true;
      }
      g_lastUpdateMs = millis();
    }

    // MQTT publish periodically when connected and we have a reading
    if (MqttMin::connected() && g_hasSmoothed) {
      uint32_t now = millis();
      if (now - g_lastPubMs >= kPubEveryMs) {
        String payload = jsonReading(g_lastUnits, g_lastRaw);
        MqttMin::publish(MQTT_TOPIC_HX711, payload.c_str(), /*retained=*/false);
        g_lastPubMs = now;
      }
    }
  }

  bool tare(uint8_t samples) { return g_hx.tare(samples); }

  void setScale(float scale) { g_hx.setScale(scale <= 0.f ? 1.f : scale); }

  void get(float& units, int32_t& raw, uint32_t& ageMs) {
    units = g_lastUnits; raw = g_lastRaw;
    ageMs = g_lastUpdateMs ? (millis() - g_lastUpdateMs) : UINT32_MAX;
  }

  float currentScale() { return g_hx.scale(); }
  int32_t currentOffset() { return g_hx.offset(); }
}

