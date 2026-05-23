#include "PushButtons.h"

#include <Arduino.h>

namespace PushButtons {

static constexpr uint8_t kButtonCount = static_cast<uint8_t>(ButtonId::Count);
static Config s_cfg{7, 8, 9, -1, 11, 15, true, 25};
static bool s_raw[kButtonCount] = {false, false, false, false, false, false};
static bool s_stable[kButtonCount] = {false, false, false, false, false, false};
static bool s_edgePress[kButtonCount] = {false, false, false, false, false, false};
static unsigned long s_lastChangeMs[kButtonCount] = {0, 0, 0, 0, 0, 0};
static unsigned long s_pressStartMs[kButtonCount] = {0, 0, 0, 0, 0, 0};

static int8_t pinFor(ButtonId id) {
  switch (id) {
    case ButtonId::Mode: return s_cfg.modePin;
    case ButtonId::Band: return s_cfg.bandPin;
    case ButtonId::Step: return s_cfg.stepPin;
    case ButtonId::Fn:   return s_cfg.fnPin;
    case ButtonId::TxRx: return s_cfg.txRxPin;
    case ButtonId::Power:return s_cfg.powerPin;
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
      s_pressStartMs[i] = v ? millis() : 0;
    } else {
      s_raw[i] = false;
      s_stable[i] = false;
      s_pressStartMs[i] = 0;
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
        s_pressStartMs[i] = now;
        s_edgePress[i] = true;
      } else {
        s_pressStartMs[i] = 0;
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

bool isDown(ButtonId id) {
  const uint8_t idx = static_cast<uint8_t>(id);
  return s_stable[idx];
}

uint32_t downDurationMs(ButtonId id) {
  const uint8_t idx = static_cast<uint8_t>(id);
  if (!s_stable[idx] || s_pressStartMs[idx] == 0) {
    return 0;
  }

  const unsigned long now = millis();
  return static_cast<uint32_t>(now - s_pressStartMs[idx]);
}

} // namespace PushButtons
