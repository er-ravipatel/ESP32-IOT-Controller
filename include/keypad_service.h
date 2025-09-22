// 4x4 Keypad service for ESP32
#pragma once

namespace KeypadService {
  // Initialize keypad (pins from macros or defaults) and start scanning
  void begin();
  // Poll keypad; handle key presses and map to relay actions
  void loop();

  // Get last pressed key and its age (ms). Returns true if a key has been pressed.
  bool getLast(char &key, uint32_t &ageMs);

  // Retrieve current input buffer as a string and its age (ms since last key).
  // Returns true if any keys have been captured; 'out' receives the full sequence.
  bool getInput(String &out, uint32_t &ageMs);

  // Clear the current input buffer (same as pressing '##').
  void clear();

  // Monotonically increases each time user presses '##' (newline event).
  // Useful for UIs to detect end-of-line and render a new line.
  uint32_t sequence();
}
