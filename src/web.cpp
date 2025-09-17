#include "web.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include "led.h"

namespace {
  WebServer server(80);

  String contentTypeFor(const String &path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png"))  return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".svg"))  return "image/svg+xml";
    if (path.endsWith(".ico"))  return "image/x-icon";
    return "text/plain";
  }

  bool serveFile(const String &path) {
    if (!LittleFS.exists(path)) return false;
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    // Avoid browser caching so latest edits are visible without hard refresh
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.streamFile(f, contentTypeFor(path));
    f.close();
    return true;
  }
}

namespace Web {
  void beginAp(const char* ssid, const char* pass) {
    // Start Wi‑Fi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, pass);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP()); // usually 192.168.4.1

    // Mount LittleFS (format if first time); try label "littlefs", then fallback to "spiffs"
    bool mounted = LittleFS.begin(true, "/littlefs", 5, "littlefs");
    if (!mounted) {
      Serial.println("LittleFS: trying fallback label 'spiffs'");
      mounted = LittleFS.begin(true, "/littlefs", 5, "spiffs");
    }
    if (!mounted) {
      Serial.println("LittleFS mount failed (even after format)");
    } else {
      Serial.printf("LittleFS OK: used %u / %u bytes\n",
                    (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
      if (!LittleFS.exists("/index.html")) {
        Serial.println("/index.html not found in LittleFS (did you Upload File System Image?)");
      }
    }

    // Routes
    server.on("/", HTTP_GET, [] {
      if (!serveFile("/index.html")) server.send(500, "text/plain", "Missing /index.html");
    });

    server.on("/state", HTTP_GET, [] {
      server.send(200, "application/json", String("{\"on\":") + (Led::isOn() ? "true" : "false") + "}");
    });

    server.on("/on", HTTP_GET, [] {
      Led::on();
      server.send(200, "application/json", "{\"on\":true}");
    });

    server.on("/off", HTTP_GET, [] {
      Led::off();
      server.send(200, "application/json", "{\"on\":false}");
    });

    server.onNotFound([] {
      String path = server.uri();
      if (!serveFile(path)) server.send(404, "text/plain", "Not found");
    });

    server.begin();
  }

  void loop() {
    server.handleClient();
  }
}

