// Optional client certificate + private key for MQTT mTLS
// -------------------------------------------------------
// If your broker requires mutual TLS (client authentication),
// define USE_MQTT_CLIENT_CERT in your build flags and paste your
// PEM-encoded certificate and key below.
//
// Example (PEM format):
// static const char MQTT_CLIENT_CERT_PEM[] PROGMEM = R"PEM(
// -----BEGIN CERTIFICATE-----
// ...
// -----END CERTIFICATE-----
// )PEM";
// static const char MQTT_CLIENT_KEY_PEM[] PROGMEM = R"PEM(
// -----BEGIN PRIVATE KEY-----
// ...
// -----END PRIVATE KEY-----
// )PEM";
//
// SECURITY: Keep private keys out of source control. Consider using
// environment-based injection or LittleFS deployment in production.

#pragma once
#include <pgmspace.h>

// Defaults: empty strings keep flash usage tiny and are ignored at runtime
static const char MQTT_CLIENT_CERT_PEM[] PROGMEM = R"PEM()PEM";
static const char MQTT_CLIENT_KEY_PEM[]  PROGMEM = R"PEM()PEM";

