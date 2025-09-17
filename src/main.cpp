// Serial LED Controller for ESP32 (PlatformIO + Arduino)                            // File purpose
#include <Arduino.h>                                                                 // Arduino core APIs
#include <ctype.h>                                                                   // C char classification (isspace)

// Adjust if your board differs
constexpr uint8_t LED_PIN = 2;          // GPIO pin for the LED (2 is onboard on many ESP32 devkits)
constexpr bool LED_ACTIVE_HIGH = true;  // true: HIGH turns LED on; false: LOW turns LED on (active‑low boards)

// LEDC (PWM) for brightness control
constexpr uint8_t LEDC_CHANNEL = 0;     // LEDC channel index (0..15 on ESP32)
constexpr uint32_t LEDC_FREQ = 5000;    // PWM frequency in Hz (5 kHz works well for LEDs)
constexpr uint8_t LEDC_RES_BITS = 8;    // PWM resolution in bits (8 → duty 0..255)

// State
uint8_t led_brightness = 0;             // Current brightness level 0..255

// Prototypes (forward declarations of functions used later)
static uint8_t applyPolarity(uint8_t duty);      // Adjust duty for active‑high/low LED wiring
static void setLedBrightness(uint8_t value);     // Set LED brightness via PWM
static void setLedOn(bool on);                   // Convenience: full on or off
static void setLedOff();                         // Convenience: turn LED fully off
static void printHelp();                         // Print command help to Serial
static String trimSpaces(const String &s);       // Trim leading/trailing whitespace
static bool parseUint(const String &s, uint16_t &out); // Parse unsigned integer from String

static uint8_t applyPolarity(uint8_t duty) {     // Map logical duty to physical signal level
  return LED_ACTIVE_HIGH ? duty : (uint8_t)(255 - duty); // Invert if LED is active‑low
}

static void setLedBrightness(uint8_t value) {    // Apply a new brightness value
  led_brightness = value;                        // Remember logical brightness (0..255)
  ledcWrite(LEDC_CHANNEL, applyPolarity(led_brightness)); // Write PWM duty to LEDC channel
}

static void setLedOn(bool on) { setLedBrightness(on ? 255 : 0); } // Full on or off using brightness helper
static void setLedOff() { setLedBrightness(0); }                   // Explicit off helper

static void printHelp() {                                          // Emit command list over Serial
  Serial.println();                                                // Blank line for readability
  Serial.println(F("Commands:"));                                  // Header
  Serial.println(F("  ON              -> LED full on"));           // Turn LED fully on
  Serial.println(F("  OFF             -> LED off"));               // Turn LED off
  Serial.println(F("  TOGGLE          -> Toggle on/off"));         // Toggle current on/off state
  Serial.println(F("  0..255          -> Set brightness (0=off, 255=max)")); // Set brightness directly
  Serial.println(F("  BRIGHT <0..255> -> Same as above"));         // Command with explicit keyword
  Serial.println(F("  HELP            -> Show this help"));        // Show this help again
  Serial.println();                                                // Trailing blank line
}

static String trimSpaces(const String &s) {                         // Remove leading/trailing whitespace
  int start = 0;                                                    // Left index
  while (start < (int)s.length() && isspace((unsigned char)s[start])) start++; // Skip left spaces
  int end = s.length() - 1;                                         // Right index
  while (end >= start && isspace((unsigned char)s[end])) end--;      // Skip right spaces
  return s.substring(start, end + 1);                                // Return trimmed substring
}

static bool parseUint(const String &s, uint16_t &out) {              // Parse unsigned integer String→number
  if (s.length() == 0) return false;                                 // Empty string is invalid
  for (size_t i = 0; i < s.length(); ++i) {                          // Check every character
    if (!isDigit((unsigned char)s[i])) return false;                 // Reject if any non-digit
  }
  out = (uint16_t)s.toInt();                                         // Convert to integer
  return true;                                                        // Parsing ok
}

void setup() {                                                       // Arduino setup runs once on boot
  Serial.begin(115200);                                              // Start serial port at 115200 baud
  delay(50);                                                         // Brief delay so monitor can attach

  // Configure LEDC for PWM control
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RES_BITS);                 // Initialize LEDC channel
  ledcAttachPin(LED_PIN, LEDC_CHANNEL);                              // Bind GPIO pin to LEDC channel
  setLedOff();                                                       // Start with LED off

  Serial.println();                                                  // Welcome banner
  Serial.println(F("ESP32 Serial LED Controller"));                  // Title
  Serial.println(F("Type HELP for commands."));                     // Quick hint
}

void loop() {                                                        // Main loop runs repeatedly
  if (Serial.available()) {                                          // If there is input waiting
    String line = Serial.readStringUntil('\n');                     // Read a line up to newline
    line = trimSpaces(line);                                         // Trim spaces and newlines
    if (line.length() == 0) return;                                  // Ignore empty lines

    String u = line; u.toUpperCase();                                // Uppercase copy for case-insensitive compare

    if (u == F("ON")) {                                             // Command: ON
      setLedOn(true);                                                // Turn LED fully on
      Serial.println(F("LED: ON"));                                  // Acknowledge
      return;                                                        // Done handling this line
    }
    if (u == F("OFF")) {                                            // Command: OFF
      setLedOn(false);                                               // Turn LED off
      Serial.println(F("LED: OFF"));                                 // Acknowledge
      return;                                                        // Done
    }
    if (u == F("TOGGLE")) {                                         // Command: TOGGLE
      setLedOn(led_brightness == 0);                                 // If off -> on, else -> off
      Serial.printf("LED: %s\n", led_brightness ? "ON" : "OFF");     // Report new state
      return;                                                        // Done
    }
    if (u == F("HELP")) {                                           // Command: HELP
      printHelp();                                                   // Show help text
      return;                                                        // Done
    }

    if (u.startsWith(F("BRIGHT "))) {                               // Command: BRIGHT <value>
      String arg = trimSpaces(line.substring(7));                    // Extract text after keyword
      uint16_t v;                                                    // Temporary parsed value
      if (parseUint(arg, v) && v <= 255) {                           // Validate 0..255 number
        setLedBrightness((uint8_t)v);                                // Apply brightness
        Serial.printf("Brightness: %u/255\n", (unsigned)v);          // Acknowledge
      } else {                                                       // Invalid value
        Serial.println(F("ERR: Use BRIGHT 0..255"));                 // Error message
      }
      return;                                                        // Done
    }

    // Bare number -> brightness
    {
      uint16_t v;                                                   // Temporary parsed value
      if (parseUint(u, v) && v <= 255) {                             // If whole line is a number 0..255
        setLedBrightness((uint8_t)v);                                // Apply brightness
        Serial.printf("Brightness: %u/255\n", (unsigned)v);          // Acknowledge
        return;                                                       // Done
      }
    }

    Serial.println(F("Unknown command. Type HELP."));               // Fallback for unrecognized input
  }
}
