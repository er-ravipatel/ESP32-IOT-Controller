// Provide your MQTT server configuration here (hardcoded)
#include "mqtt_config.h"

// Broker connection
const char* MQTT_HOST      = "y2d012bd.ala.eu-central-1.emqxsl.com"; // CHANGE if using your own broker
const uint16_t MQTT_PORT   = 8883;                // 1883 (TCP) or 8883 (TLS - not covered here)
const char* MQTT_USER      = "ESP32-LOCK-VIK";                  // optional
const char* MQTT_PASS      = "lkj082A%SD524";                  // optional
const char* MQTT_CLIENT_ID = "esp32-Pune";    // change to a unique ID if using shared broker

// Topics
const char* MQTT_TOPIC_CMD   = "esp32/led/cmd";   // publish ON/OFF/TOGGLE (or 1/0) to this topic
const char* MQTT_TOPIC_STATE = "esp32/led/state"; // device publishes retained state here
const char* MQTT_TOPIC_HX711 = "esp32/hx711/weight"; // device publishes JSON readings here
