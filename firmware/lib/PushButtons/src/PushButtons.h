#pragma once

#include <stdint.h>

namespace PushButtons {

enum class ButtonId : uint8_t {
  Mode = 0,
  Band,
  Step,
  Fn,
  TxRx,
  Power,
  Count
};

struct Config {
  int8_t modePin;
  int8_t bandPin;
  int8_t stepPin;
  int8_t fnPin;
  int8_t txRxPin;
  int8_t powerPin;
  bool activeLow;
  uint16_t debounceMs;
};

void begin(const Config& cfg = {7, 8, 9, -1, 11, 15, true, 25});
void update();
bool pressed(ButtonId id);
bool isDown(ButtonId id);
uint32_t downDurationMs(ButtonId id);

} // namespace PushButtons
