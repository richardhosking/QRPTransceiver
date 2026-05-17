#pragma once

#include <stdint.h>

namespace PushButtons {

enum class ButtonId : uint8_t {
  Mode = 0,
  Band,
  Step,
  Fn,
  TxRx,
  Count
};

struct Config {
  int8_t modePin;
  int8_t bandPin;
  int8_t stepPin;
  int8_t fnPin;
  int8_t txRxPin;
  bool activeLow;
  uint16_t debounceMs;
};

void begin(const Config& cfg = {7, 8, 9, -1, 11, true, 25});
void update();
bool pressed(ButtonId id);

} // namespace PushButtons
