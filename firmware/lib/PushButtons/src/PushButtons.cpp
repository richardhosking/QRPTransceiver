#include "PushButtons.h"

#include <Arduino.h>

namespace PushButtons {

static Config s_cfg{7, 8, 9, -1, true, 25};
static bool s_raw[4] = {false, false, false, false};
static bool s_stable[4] = {false, false, false, false};
static bool s_edgePress[4] = {false, false, false, false};
static unsigned long s_lastChangeMs[4] = {0, 0, 0, 0};

static int8_t pinFor(ButtonId id) {
  switch (id) {
    case ButtonId::Mode: return s_cfg.modePin;
    case ButtonId::Band: return s_cfg.bandPin;
    case ButtonId::Step: return s_cfg.stepPin;
    case ButtonId::Fn:   return s_cfg.fnPin;
    default:             return -1;
  }
}

static bool readPhysical(int8_t pin) {
  if (pin < 0) return false;
  const bool level = digitalRead(static_cast<uint8_t>(pin));
  return s_cfg.activeLow ? !level : level;
}

void begin(const Config& cfg) {
  s_cfg = cfg;

  for (uint8_t i = 0; i < static_cast<uint8_t>(ButtonId::Count); ++i) {
    const auto id = static_cast<ButtonId>(i);
    const int8_t pin = pinFor(id);
    if (pin >= 0) {
      pinMode(static_cast<uint8_t>(pin), s_cfg.activeLow ? INPUT_PULLUP : INPUT);
      const bool v = readPhysical(pin);
      s_raw[i] = v;
      s_stable[i] = v;
    } else {
      s_raw[i] = false;
      s_stable[i] = false;
    }
    s_edgePress[i] = false;
    s_lastChangeMs[i] = millis();
  }
}

void update() {
  const unsigned long now = millis();
  for (uint8_t i = 0; i < static_cast<uint8_t>(ButtonId::Count); ++i) {
    const auto id = static_cast<ButtonId>(i);
    const int8_t pin = pinFor(id);
    if (pin < 0) continue;

    const bool current = readPhysical(pin);
    if (current != s_raw[i]) {
      s_raw[i] = current;
      s_lastChangeMs[i] = now;
    }

    if ((now - s_lastChangeMs[i]) >= s_cfg.debounceMs && s_stable[i] != s_raw[i]) {
      s_stable[i] = s_raw[i];
      if (s_stable[i]) {
        s_edgePress[i] = true;
      }
    }
  }
}

bool pressed(ButtonId id) {
  const uint8_t idx = static_cast<uint8_t>(id);
  const bool hit = s_edgePress[idx];
  s_edgePress[idx] = false;
  return hit;
}

} // namespace PushButtons
