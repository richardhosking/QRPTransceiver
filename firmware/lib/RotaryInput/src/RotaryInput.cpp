#include "RotaryInput.h"

#include <Arduino.h>

namespace RotaryInput {

static Config s_cfg{2, 3, -1, false, 100};
static uint8_t s_prevNextCode = 0;
static uint16_t s_store = 0;
static bool s_lastBtn = true;
static bool s_btnEdge = false;

// State-machine transition validity table for Gray code
// 1 = valid transition, 0 = invalid/noise
static const uint8_t kRotEncTable[16] = {
  0, 1, 1, 0,
  1, 0, 0, 1,
  1, 0, 0, 1,
  0, 1, 1, 0
};

void begin(const Config& cfg) {
  s_cfg = cfg;

  pinMode(s_cfg.pinA, INPUT_PULLUP);
  pinMode(s_cfg.pinB, INPUT_PULLUP);
  if (s_cfg.pinButton >= 0) {
    pinMode(static_cast<uint8_t>(s_cfg.pinButton), INPUT_PULLUP);
    s_lastBtn = digitalRead(static_cast<uint8_t>(s_cfg.pinButton));
  }

  s_prevNextCode = 0;
  s_store = 0;
  s_btnEdge = false;
}

int32_t readDeltaSteps() {
  // Shift in previous state and append current pin state into lower bits
  s_prevNextCode <<= 2;
  if (digitalRead(s_cfg.pinB)) s_prevNextCode |= 0x02;
  if (digitalRead(s_cfg.pinA)) s_prevNextCode |= 0x01;
  s_prevNextCode &= 0x0F;

  // Validate transition against Gray-code state table
  if (kRotEncTable[s_prevNextCode]) {
    s_store <<= 4;
    s_store |= s_prevNextCode;

    // Full valid cycle signatures
    // 0x17 = CW, 0x2B = CCW
    if ((s_store & 0xFF) == 0x17) {
      return s_cfg.invertDirection ? -1 : +1;
    }
    if ((s_store & 0xFF) == 0x2B) {
      return s_cfg.invertDirection ? +1 : -1;
    }
  }

  return 0;
}

bool buttonPressed() {
  if (s_cfg.pinButton < 0) return false;

  const bool now = digitalRead(static_cast<uint8_t>(s_cfg.pinButton));
  if (s_lastBtn && !now) {
    s_btnEdge = true; // falling edge
  }
  s_lastBtn = now;

  const bool ret = s_btnEdge;
  s_btnEdge = false;
  return ret;
}

int32_t stepHz() {
  return s_cfg.stepHz;
}

void setStepHz(int32_t step) {
  if (step < 1) step = 1;
  s_cfg.stepHz = step;
}

} // namespace RotaryInput
