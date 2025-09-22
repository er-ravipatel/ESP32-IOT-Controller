// 4x4 Keypad service: maps keys 1..4 to Relays 1..4
#include <Arduino.h>
#include <Keypad.h>
#include "relays.h"
#include "mqtt_min.h"
#include "mqtt_config.h"
#include "portal.h"

// Default wiring (override via build_flags)
#ifndef KEYPAD_ROWS
#define KEYPAD_ROWS 4
#endif
#ifndef KEYPAD_COLS
#define KEYPAD_COLS 4
#endif

// Default pins chosen to avoid strapping pins and common peripherals
#ifndef KEYPAD_ROW_PINS
#define KEYPAD_ROW_PINS { 15, 04, 05, 18 }
#endif
#ifndef KEYPAD_COL_PINS
#define KEYPAD_COL_PINS { 19, 21, 22, 23 }
#endif

namespace {
  const byte kRows = KEYPAD_ROWS;
  const byte kCols = KEYPAD_COLS;
  const char kKeymap[kRows][kCols] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };

  // Expand macros to arrays
  const byte rowPins[kRows] = KEYPAD_ROW_PINS;
  const byte colPins[kCols] = KEYPAD_COL_PINS;

  Keypad g_keypad = Keypad(makeKeymap((char*)kKeymap), (byte*)rowPins, (byte*)colPins, kRows, kCols);

  volatile char g_lastKey = 0;
  volatile uint32_t g_lastAt = 0; // millis when last key arrived
  volatile uint32_t g_seq = 0;    // increments on '##' (newline)
  // Capture buffer for a sequence of keys
  #ifndef KEYPAD_BUF_SIZE
  #define KEYPAD_BUF_SIZE 64
  #endif
  char g_buf[KEYPAD_BUF_SIZE] = {0};
  size_t g_len = 0;
  char g_prevKey = 0;

  String replaceSuffix(const String& base, const char* oldSuffix, const String& replacement) {
    int p = base.lastIndexOf(oldSuffix);
    if (p < 0) return base + '/' + replacement;
    return base.substring(0, p) + '/' + replacement;
  }
  String relayStateTopicFor(size_t idx1) {
    return replaceSuffix(String(MQTT_TOPIC_RELAY_STATE), "/state", String(idx1) + "/state");
  }

  void publishRelayState(size_t idx0) {
    if (!MqttMin::connected()) return;
    String topic = relayStateTopicFor(idx0 + 1);
    String payload = String("{\"on\":") + (Relays::get(idx0)?"true":"false") + "}";
    MqttMin::publish(topic.c_str(), payload.c_str(), true);
  }
}

namespace KeypadService {
  void begin() {
    // Set a modest debounce time
    g_keypad.setDebounceTime(25);
    // Announce keypad capture semantics to Serial Monitor
    Serial.println(F("Keypad: capture on. Rules: '##' = newline+clear, '**' = backspace one char."));
  }

  void loop() {
    char key = g_keypad.getKey();
    if (!key) return;
    g_lastKey = key; g_lastAt = millis();
    // Simple input capture + Serial echo of all input
    // - Append every key, allow repeats
    // - '##' prints newline and clears buffer (start fresh on next line)
    // - '**' backspaces one character (press again to delete another)
    if (key == '#') {
      if (g_prevKey == '#') {
        Serial.println();            // move to next line
        g_len = 0; g_buf[0] = '\0'; // clear buffer
        g_seq++;                     // mark newline event
        g_prevKey = 0;
      } else {
        g_prevKey = '#';
      }
      return;
    }
    if (key == '*') {
      if (g_prevKey == '*') {
        if (g_len > 0) {
          // erase last char from buffer
          g_len--; g_buf[g_len] = '\0';
          // and visually from Serial (BS, space, BS pattern)
          Serial.print("\b \b");
        }
      } else {
        g_prevKey = '*'; // arm backspace mode
      }
      return;
    }
    // Normal key: append + echo
    g_prevKey = key;
    if (g_len + 1 < KEYPAD_BUF_SIZE) {
      g_buf[g_len++] = key;
      g_buf[g_len] = '\0';
      Serial.print(key);
    }

    // Pattern actions (non-destructive): check last 4 keys
    if (g_len >= 4) {
      char *p = &g_buf[g_len-4];
      if (p[0]=='5' && p[1]=='5' && p[2]=='9' && p[3]=='9') {
        Portal::setLed(true);
        Serial.println(F("  => LED ON (pattern 5599)"));
      } else if (p[0]=='9' && p[1]=='9' && p[2]=='5' && p[3]=='5') {
        Portal::setLed(false);
        Serial.println(F("  => LED OFF (pattern 9955)"));
      }
    }
  }

  bool getLast(char &key, uint32_t &ageMs) {
    char k = g_lastKey;
    if (!k) return false;
    key = k;
    uint32_t at = g_lastAt;
    ageMs = at ? (millis() - at) : UINT32_MAX;
    return true;
  }

  bool getInput(String &out, uint32_t &ageMs) {
    if (g_len == 0) { out = String(); ageMs = UINT32_MAX; return false; }
    out = String(g_buf);
    ageMs = g_lastAt ? (millis() - g_lastAt) : UINT32_MAX;
    return true;
  }

  void clear() { g_len = 0; g_buf[0] = '\0'; g_prevKey = 0; }

  uint32_t sequence() { return g_seq; }
}
