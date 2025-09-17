// Minimal Web LED control (ESP32 + Arduino) with static HTML from LittleFS
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for typical ESP32 DevKit boards
#endif

// Create a simple Access Point so no Wi‑Fi credentials are needed
static const char* AP_SSID = "ESP32-LED";
static const char* AP_PASS = "12345678"; // Min 8 chars; change if desired

WebServer server(80);
bool ledOn = false;

static String contentTypeFor(const String &path) {
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

static bool serveFile(const String &path) {
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  server.streamFile(f, contentTypeFor(path));
  f.close();
  return true;
}

static void handleOn()  { digitalWrite(LED_BUILTIN, HIGH); ledOn = true;  server.send(200, "application/json", "{\"on\":true}"); }
static void handleOff() { digitalWrite(LED_BUILTIN, LOW);  ledOn = false; server.send(200, "application/json", "{\"on\":false}"); }

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Start OFF (set HIGH if your LED is active‑low)

  Serial.begin(115200);
  delay(50);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // Typically 192.168.4.1

  // Mount LittleFS (format on first run if needed)
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  // Serve static index and simple JSON API endpoints
  server.on("/", HTTP_GET, [] {
    if (!serveFile("/index.html")) server.send(500, "text/plain", "Missing /index.html");
  });
  server.on("/on", HTTP_GET, handleOn);
  server.on("/off", HTTP_GET, handleOff);
  server.on("/state", HTTP_GET, [] {
    server.send(200, "application/json", String("{\"on\":") + (ledOn ? "true" : "false") + "}");
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
