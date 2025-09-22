// Minimal MQTT implementation using PubSubClient
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mqtt_config.h"
#include "certs.h"
#include "client_certs.h"
#include <time.h>
#include "relay.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// LED polarity (match main.cpp if needed)
static const bool LED_ACTIVE_HIGH = true;
static inline void writeLed(bool on) { digitalWrite(LED_BUILTIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW); }
static inline bool isLedOn() { int lvl = digitalRead(LED_BUILTIN); return LED_ACTIVE_HIGH ? (lvl==HIGH):(lvl==LOW); }

namespace {
  WiFiClientSecure g_net;
  PubSubClient g_mqtt(g_net);
  uint32_t g_lastTry = 0;          // last attempted connect time
  uint32_t g_nextTryDelay = 5000;  // backoff, starts at 5s up to 60s
  uint32_t g_lastStatePub = 0;     // last state publish time
  uint32_t g_lastRelayStatePub = 0;
  uint32_t g_wifiUpSince = 0;      // when Wi‑Fi became connected
  bool g_timeReady = false;        // SNTP time set
  uint32_t g_timeStart = 0;        // when SNTP was started

  // Buffer for an auto-generated MQTT client ID (stable per device)
  // Example: "esp32-A1B2C3D4E5F6"
  char g_clientId[32] = {0};

  static void ensureClientIdGenerated() {
    if (MQTT_CLIENT_ID && MQTT_CLIENT_ID[0] != '\\0') return; // user provided
    uint64_t mac = ESP.getEfuseMac();
    uint8_t b0 = (mac >> 40) & 0xFF;
    uint8_t b1 = (mac >> 32) & 0xFF;
    uint8_t b2 = (mac >> 24) & 0xFF;
    uint8_t b3 = (mac >> 16) & 0xFF;
    uint8_t b4 = (mac >>  8) & 0xFF;
    uint8_t b5 = (mac >>  0) & 0xFF;
    snprintf(g_clientId, sizeof(g_clientId), "esp32-RAVI-%02X%02X%02X%02X%02X%02X", b0,b1,b2,b3,b4,b5);
    MQTT_CLIENT_ID = g_clientId;
  }

  static bool timeIsSet() {
    time_t now = time(nullptr);
    return now > 1609459200; // > 2021-01-01
  }

  void publishState(bool force=false) {
    if (!g_mqtt.connected()) return;
    uint32_t now = millis();
    if (!force && (now - g_lastStatePub) < 30000) return; // heartbeat 30s
    String payload = String("{\"on\":") + (isLedOn()?"true":"false") + "}";
    bool ok = g_mqtt.publish(MQTT_TOPIC_STATE, payload.c_str(), true);
    Serial.print(F("MQTT: publish ")); Serial.print(MQTT_TOPIC_STATE);
    Serial.print(F(" payload=")); Serial.print(payload);
    Serial.print(F(" result=")); Serial.println(ok ? F("OK") : F("FAIL"));
    g_lastStatePub = now;
  }

  void publishRelayState(bool force=false) {
    if (!g_mqtt.connected()) return;
    uint32_t now = millis();
    if (!force && (now - g_lastRelayStatePub) < 30000) return; // heartbeat 30s
    String payload = String("{\"on\":") + (Relay::get()?"true":"false") + "}";
    bool ok = g_mqtt.publish(MQTT_TOPIC_RELAY_STATE, payload.c_str(), true);
    Serial.print(F("MQTT: publish ")); Serial.print(MQTT_TOPIC_RELAY_STATE);
    Serial.print(F(" payload=")); Serial.print(payload);
    Serial.print(F(" result=")); Serial.println(ok ? F("OK") : F("FAIL"));
    g_lastRelayStatePub = now;
  }

  void onMsg(char* topic, byte* payload, unsigned int len) {
    String t = String(topic);
    String msg; msg.reserve(len);
    for (unsigned int i=0;i<len;i++) msg += (char)payload[i];
    msg.trim(); String u = msg; u.toUpperCase();
    Serial.print(F("MQTT: message topic=")); Serial.print(t);
    Serial.print(F(" payload=")); Serial.println(msg);
    if (t == MQTT_TOPIC_CMD) {
      if (u == "ON" || u == "1") { writeLed(true);  Serial.println(F("LED: ON (via MQTT)")); }
      else if (u == "OFF" || u == "0") { writeLed(false); Serial.println(F("LED: OFF (via MQTT)")); }
      else if (u == "TOGGLE" || u == "T") { writeLed(!isLedOn()); Serial.println(F("LED: TOGGLE (via MQTT)")); }
      else { Serial.println(F("MQTT: unknown LED command")); }
      publishState(true);
    } else if (t == MQTT_TOPIC_RELAY_CMD) {
      if (u == "ON" || u == "1") { Relay::set(true);  Serial.println(F("Relay: ON (via MQTT)")); }
      else if (u == "OFF" || u == "0") { Relay::set(false); Serial.println(F("Relay: OFF (via MQTT)")); }
      else if (u == "TOGGLE" || u == "T") { Relay::toggle(); Serial.println(F("Relay: TOGGLE (via MQTT)")); }
      else { Serial.println(F("MQTT: unknown RELAY command")); }
      publishRelayState(true);
    }
  }

  void ensureConnected() {
    // Only try after Wi‑Fi is connected and has been stable a few seconds
    if (WiFi.status() != WL_CONNECTED) {
      g_wifiUpSince = 0;
      g_timeReady = false; g_timeStart = 0;
      return;
    }
    uint32_t now = millis();
    if (g_wifiUpSince == 0) g_wifiUpSince = now;
    if (now - g_wifiUpSince < 3000) return; // 3s settle time

    // If building with CA and/or client certs, make sure SNTP time is set
#if defined(USE_MQTT_CA) || defined(USE_MQTT_CLIENT_CERT)
    if (!g_timeReady) {
      if (g_timeStart == 0) {
        Serial.println(F("SNTP: starting time sync..."));
        configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        g_timeStart = now;
      }
      if (timeIsSet()) {
        g_timeReady = true;
        Serial.println(F("SNTP: time set OK"));
      } else {
        // Wait up to 15s for time to set
        if (now - g_timeStart < 15000) return; else return; // keep waiting, try again next loop
      }
    }
#endif

    if (g_mqtt.connected()) return;
    if (now - g_lastTry < g_nextTryDelay) return; // respect backoff
    g_lastTry = now;

    // Configure TLS (with CA if provided) + server using hostname
#ifdef USE_MQTT_CA
    g_net.setCACert(MQTT_ROOT_CA_PEM);
    Serial.println(F("TLS: using Root CA validation"));
#else
    g_net.setInsecure();
    Serial.println(F("TLS: insecure connection (no CA)"));
#endif

#ifdef USE_MQTT_CLIENT_CERT
    if (MQTT_CLIENT_CERT_PEM[0] != '\0' && MQTT_CLIENT_KEY_PEM[0] != '\0') {
      g_net.setCertificate(MQTT_CLIENT_CERT_PEM);
      g_net.setPrivateKey(MQTT_CLIENT_KEY_PEM);
      Serial.println(F("TLS: client certificate configured"));
    } else {
      Serial.println(F("TLS: USE_MQTT_CLIENT_CERT set but no cert/key provided"));
    }
#endif
    g_mqtt.setServer(MQTT_HOST, MQTT_PORT);
    g_mqtt.setSocketTimeout(5);
#ifdef USE_MQTT_CA
    Serial.print(F("MQTT: connecting (TLS, CA-validated) to "));
#else
    Serial.print(F("MQTT: connecting (TLS, insecure) to "));
#endif
    Serial.print(MQTT_HOST); Serial.print(':'); Serial.println(MQTT_PORT);
    g_mqtt.setCallback(onMsg);
    Serial.print(F("MQTT: clientId=")); Serial.println(MQTT_CLIENT_ID);
    if (strlen(MQTT_USER)) { Serial.print(F("MQTT: user=")); Serial.println(MQTT_USER); }

    bool ok = strlen(MQTT_USER) ? g_mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)
                                : g_mqtt.connect(MQTT_CLIENT_ID);
    if (ok) {
      Serial.println(F("MQTT: connected"));
      g_mqtt.subscribe(MQTT_TOPIC_CMD);
      Serial.print(F("MQTT: subscribed to ")); Serial.println(MQTT_TOPIC_CMD);
      g_mqtt.subscribe(MQTT_TOPIC_RELAY_CMD);
      Serial.print(F("MQTT: subscribed to ")); Serial.println(MQTT_TOPIC_RELAY_CMD);
      publishState(true);
      publishRelayState(true);
      g_nextTryDelay = 5000; // reset backoff on success
    } else {
      int rc = g_mqtt.state();
      Serial.print(F("MQTT: connect failed, rc=")); Serial.println(rc);
      // Exponential-ish backoff up to 60s
      g_nextTryDelay = (g_nextTryDelay < 60000) ? (g_nextTryDelay + 5000) : 60000;
    }
  }
}

namespace MqttMin {
  void begin() {
    pinMode(LED_BUILTIN, OUTPUT);
    // Build a device-unique MQTT client ID if none supplied
    ensureClientIdGenerated();
    // Log configuration summary once
    Serial.println(F("MQTT: configured (will attempt after Wi‑Fi is up):"));
    Serial.print(F("  host=")); Serial.print(MQTT_HOST); Serial.print(F(" port=")); Serial.println(MQTT_PORT);
    Serial.print(F("  clientId=")); Serial.println(MQTT_CLIENT_ID);
    Serial.print(F("  cmdTopic=")); Serial.println(MQTT_TOPIC_CMD);
    Serial.print(F("  stateTopic=")); Serial.println(MQTT_TOPIC_STATE);
    // Server and callback assigned on first ensureConnected()
  }

  void loop() {
    ensureConnected();
    if (g_mqtt.connected()) g_mqtt.loop();
    publishState();
  }

  bool connected() { return g_mqtt.connected(); }

  bool publish(const char* topic, const char* payload, bool retained) {
    if (!g_mqtt.connected() || !topic || !payload) return false;
    return g_mqtt.publish(topic, payload, retained);
  }
}



