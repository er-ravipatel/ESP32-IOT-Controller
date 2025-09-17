// Minimal Serial LED control (ESP32 + Arduino)
#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Fallback for typical ESP32 DevKit boards
#endif

bool ledOn = false; // Track LED state

void printHelp() {
  Serial.println();
  Serial.println(F("Commands:"));
  Serial.println(F("  ON       -> LED on"));
  Serial.println(F("  OFF      -> LED off"));
  Serial.println(F("  TOGGLE   -> Switch state"));
  Serial.println(F("  STATE    -> Print current state"));
  Serial.println(F("  1 / 0    -> ON / OFF"));
  Serial.println();
}

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_BUILTIN, on ? HIGH : LOW); // If your LED is active-low, swap HIGH/LOW
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Configure LED pin as output
  setLed(false);                    // Start LED off

  Serial.begin(115200);             // Matches monitor_speed in platformio.ini
  delay(50);
  Serial.println(F("ESP32 Serial LED"));
  Serial.println(F("Type HELP for commands."));
}

void loop() {
  if (!Serial.available()) return;              // Nothing to read

  String cmd = Serial.readStringUntil('\n');   // Read a line (press Enter)
  cmd.trim();                                   // Strip spaces and CR/LF
  if (cmd.length() == 0) return;                // Ignore empty

  String u = cmd; u.toUpperCase();              // Case-insensitive

  if (u == F("ON") || u == F("1"))          { setLed(true);  Serial.println(F("OK: ON"));  return; }
  if (u == F("OFF")|| u == F("0"))          { setLed(false); Serial.println(F("OK: OFF")); return; }
  if (u == F("TOGGLE") || u == F("T"))      { setLed(!ledOn); Serial.println(ledOn ? F("OK: ON") : F("OK: OFF")); return; }
  if (u == F("STATE") || u == F("STATUS"))  { Serial.println(ledOn ? F("STATUS: ON") : F("STATUS: OFF")); return; }
  if (u == F("HELP"))                         { printHelp(); return; }

  Serial.println(F("Unknown command. Try: ON / OFF / TOGGLE / STATE"));
}

