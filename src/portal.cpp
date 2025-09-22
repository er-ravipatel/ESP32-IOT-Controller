// Setup portal (SoftAP + captive DNS) and minimal web API
// --------------------------------------------------------
// Responsibilities
// - Start/stop SoftAP + captive DNS for onboarding
// - Serve static files from LittleFS (setup.html, localcontroller.html)
// - Provide tiny JSON endpoints: /status, /state, /on, /off
// - Keep HTTP server running on STA even after AP stops
//
// This module avoids heavy/blocking work in request handlers to keep
// the page responsive while polling.
#include "portal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include <DNSServer.h>
#include "wifi_config.h"
#include "hx711_service.h"

namespace {
  WebServer server(80);
  DNSServer dns;
  bool g_running = false;
  bool g_newCreds = false;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
  // Active-high by default; change here if your board LED is active-low
  static const bool LED_ACTIVE_HIGH = true;
  static bool ledOn = false;
  static inline void writeLed(bool on){
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW);
    ledOn = on;
  }

  String apName() {
    uint32_t chip = (uint32_t)ESP.getEfuseMac();
    char buf[32];
    snprintf(buf, sizeof(buf), "ESP32-Setup-%04X", (unsigned)(chip & 0xFFFF));
    return String(buf);
  }

  String contentTypeFor(const String &path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    return "text/plain";
  }

  bool serveFile(const String &path) {
    if (!LittleFS.exists(path)) return false;
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.streamFile(f, contentTypeFor(path));
    f.close();
    return true;
  }

  void handleRoot() {
    if (serveFile("/setup.html")) return;
    if (serveFile("/wifi.html")) return;
    // Fallback inline page
    server.send(200, "text/html",
      "<!doctype html><html><meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>ESP32 WiFi Setup - Fallback</title><body style='font-family:sans-serif;margin:2rem'>"
      "<h2>ESP32 Wi - Fi Setup - Fallback</h2>"
      "<form method='POST' action='/save'>"
      "<label>SSID<br><input name='ssid' required></label><br><br>"
      "<label>Password<br><input type='password' name='pass'></label><br><br>"
      "<button type='submit'>Save & Connect</button>"
      "</form>"
      "</body></html>");
  }

  void handleScan() {
    // Ensure STA scanning is enabled
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
      if (i) json += ',';
      json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + 
              ",\"enc\":" + String(WiFi.encryptionType(i)) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  }

  void handleSave() {
    if (!server.hasArg("ssid")) { server.send(400, "text/plain", "Missing ssid"); return; }
    String ssid = server.arg("ssid");
    String pass = server.hasArg("pass") ? server.arg("pass") : String();
    WifiConfig::save(ssid, pass);
    g_newCreds = true;
    server.send(200, "application/json", "{\"ok\":true}");
  }
}

namespace Portal {
  void begin(const char* apSsid, const char* apPass) {
    // Mount LittleFS explicitly on label "littlefs"; fallback to "spiffs" for compatibility
    bool mounted = LittleFS.begin(true, "/littlefs", 5, "littlefs");
    if (!mounted) {
      mounted = LittleFS.begin(true, "/littlefs", 5, "spiffs");
    }
    if (!mounted) {
      Serial.println("LittleFS mount failed (portal will use inline page)");
    }
    WiFi.mode(WIFI_AP_STA); // allow scanning while AP is up
    String ssid = apSsid ? String(apSsid) : apName();
    const char* pass = apPass ? apPass : "12345678";
    WiFi.softAP(ssid.c_str(), pass);
    delay(100);
    IPAddress apIp = WiFi.softAPIP();
    Serial.println();
    Serial.println(F("==== ESP32 Setup Portal ===="));
    Serial.print (F("AP SSID: ")); Serial.println(ssid);
    Serial.print (F("AP Pass: ")); Serial.println(pass);
    Serial.print (F("AP IP:   ")); Serial.println(apIp);
    // Captive DNS: answer all domains to our AP IP
    dns.start(53, "*", apIp);
    Serial.println(F("Captive DNS: active (all domains resolve to AP IP)"));
    Serial.println(F("Open one of these in your browser after joining the AP:"));
    Serial.println(F("  • http://192.168.4.1/"));
    Serial.println(F("  • http://192.168.4.1/setup.html"));
    Serial.println(F("  • http://esp32.setup/  (any host should redirect)"));

    // Root -> redirect to setup.html if present, else handleRoot fallback
    server.on("/", [] {
      if (LittleFS.exists("/setup.html")) {
        server.sendHeader("Location", "/setup.html", true);
        server.send(302, "text/plain", "");
      } else {
        handleRoot();
      }
    });

    server.on("/wifi", handleRoot);
    server.on("/setup", [] { handleRoot(); });
    server.on("/setup.html", [] { handleRoot(); });
    // Common captive endpoints for OS captive portal detection
    server.on("/generate_204", [] { handleRoot(); });
    server.on("/gen_204", [] { handleRoot(); });
    server.on("/hotspot-detect.html", [] { handleRoot(); });
    server.on("/library/test/success.html", [] { handleRoot(); });
    server.on("/ncsi.txt", [] { handleRoot(); });
    server.on("/fwlink", [] { handleRoot(); });
    server.on("/scan", handleScan);
    server.on("/save", HTTP_POST, handleSave);
    // HX711 endpoints
    server.on("/hx", HTTP_GET, []{
      float u; int32_t raw; uint32_t age; HxService::get(u, raw, age);
      String json = String("{\"value\":") + String(u, 3) +
                    ",\"raw\":" + String(raw) +
                    ",\"age_ms\":" + (age==UINT32_MAX?String(-1):String(age)) +
                    ",\"scale\":" + String(HxService::currentScale(), 6) +
                    "}";
      server.send(200, "application/json", json);
    });
    server.on("/hx/tare", HTTP_GET, []{
      bool ok = HxService::tare(8);
      server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}");
    });
    server.on("/hx/scale", HTTP_GET, []{
      if (!server.hasArg("k")) { server.send(400, "text/plain", "Missing k"); return; }
      float k = server.arg("k").toFloat(); if (k <= 0.f) { server.send(400, "text/plain", "k must be > 0"); return; }
      HxService::setScale(k);
      server.send(200, "application/json", String("{\"ok\":true,\"scale\":") + String(HxService::currentScale(),6) + "}");
    });
    server.on("/hx/config", HTTP_GET, []{
      String json = String("{\"scale\":") + String(HxService::currentScale(),6) +
                    ",\"offset\":" + String(HxService::currentOffset()) + "}";
      server.send(200, "application/json", json);
    });
    // Device status: Wi‑Fi + Internet reachability
    server.on("/status", HTTP_GET, []{
      bool connected = (WiFi.status() == WL_CONNECTED);
      IPAddress ip = connected ? WiFi.localIP() : IPAddress(0,0,0,0);
      String ssid = connected ? WiFi.SSID() : String();
      int rssi = connected ? WiFi.RSSI() : 0;
      bool internet = false;
      if (connected) {
        IPAddress probe;
        internet = (WiFi.hostByName("example.com", probe) == 1);
      }
      String json = String("{\"connected\":") + (connected?"true":"false") +
                    ",\"ssid\":\"" + ssid + "\"" +
                    ",\"ip\":\"" + ip.toString() + "\"" +
                    ",\"rssi\":" + String(rssi) +
                    ",\"internet\":" + (internet?"true":"false") +
                    "}";
      server.send(200, "application/json", json);
    });
    // LED control endpoints (work both before/after STA connect)
    server.on("/state", HTTP_GET, []{
      server.send(200, "application/json", String("{\"on\":") + (ledOn?"true":"false") + "}");
    });
    server.on("/on", HTTP_GET, []{ writeLed(true); server.send(200, "application/json", "{\"on\":true}"); });
    server.on("/off", HTTP_GET, []{ writeLed(false); server.send(200, "application/json", "{\"on\":false}"); });
    server.on("/toggle", HTTP_GET, []{
      int level = digitalRead(LED_BUILTIN);
      bool on = LED_ACTIVE_HIGH ? (level == HIGH) : (level == LOW);
      bool next = !on;
      writeLed(next);
      server.send(200, "application/json", String("{\"on\":") + (next?"true":"false") + "}");
    });
    server.onNotFound([]{
      String path = server.uri();
      if (!serveFile(path)) handleRoot();
    });
    server.begin();
    g_running = true; g_newCreds = false;
  }

  void loop() {
    if (g_running) { dns.processNextRequest(); server.handleClient(); }
  }

  void stop() {
    if (!g_running) return;
    server.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
    g_running = false;
  }

  bool running() { return g_running; }
  bool hasNewCredentials() { bool v = g_newCreds; g_newCreds = false; return v; }
}
