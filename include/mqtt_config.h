// Minimal MQTT configuration (hardcoded values)
// --------------------------------------------
// Edit values in src/mqtt_config.cpp to match your broker.
#pragma once
#include <Arduino.h>

extern const char* MQTT_HOST;   // e.g. "broker.hivemq.com"
extern const uint16_t MQTT_PORT; // e.g. 1883
extern const char* MQTT_USER;   // optional, may be ""
extern const char* MQTT_PASS;   // optional, may be ""
extern const char* MQTT_CLIENT_ID; // e.g. "esp32-abcdef"

// Topics
extern const char* MQTT_TOPIC_CMD;   // subscribe here: expects "ON"/"OFF"/"TOGGLE"/"1"/"0"
extern const char* MQTT_TOPIC_STATE; // publish state JSON {"on":true|false}
// HX711 publish topic (JSON: {"value":<float>,"raw":<int>,"unit":"units"})
extern const char* MQTT_TOPIC_HX711;
