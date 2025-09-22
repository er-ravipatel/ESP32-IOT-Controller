// Minimal MQTT helper (subscribe to LED command topic, publish state)
#pragma once

namespace MqttMin {
  void begin(); // set server + callback; does not block
  void loop();  // keep MQTT connected, handle messages, publish heartbeat

  // Lightweight helpers for other modules
  bool connected();
  bool publish(const char* topic, const char* payload, bool retained=false);
}
