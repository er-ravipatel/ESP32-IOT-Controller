#include "portal.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include "wifi_config.h"

namespace {
  WebServer server(80);
  bool g_running = false;
  bool g_newCreds = false;

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
    if (!serveFile("/wifi.html")) {
      // Fallback inline page
      server.send(200, "text/html",
        "<!doctype html><html><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>ESP32 WiFi Setup</title><body style='font-family:sans-serif;margin:2rem'>"
        "<h2>ESP32 Wi‑Fi Setup</h2>"
        "<form method='POST' action='/save'>"
        "<label>SSID<br><input name='ssid' required></label><br><br>"
        "<label>Password<br><input type='password' name='pass'></label><br><br>"
        "<button type='submit'>Save & Connect</button>"
        "</form>"
        "</body></html>");
    }
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
    if (!LittleFS.begin(true)) {
      // Continue anyway; inline page will be used
    }
    WiFi.mode(WIFI_AP_STA); // allow scanning while AP is up
    String ssid = apSsid ? String(apSsid) : apName();
    const char* pass = apPass ? apPass : "12345678";
    WiFi.softAP(ssid.c_str(), pass);
    delay(100);
    Serial.print("Config portal AP IP: "); Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/wifi", handleRoot);
    server.on("/scan", handleScan);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleRoot);
    server.begin();
    g_running = true; g_newCreds = false;
  }

  void loop() {
    if (g_running) server.handleClient();
  }

  void stop() {
    if (!g_running) return;
    server.stop();
    WiFi.softAPdisconnect(true);
    g_running = false;
  }

  bool running() { return g_running; }
  bool hasNewCredentials() { bool v = g_newCreds; g_newCreds = false; return v; }
}

