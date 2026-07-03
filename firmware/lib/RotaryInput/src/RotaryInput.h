#pragma once

#include <stdint.h>

namespace RotaryInput {

struct Config {
  uint8_t pinA;
  uint8_t pinB;
  int8_t pinButton;      // -1 if no push button used
  bool invertDirection;
  int32_t stepHz;        // Hz change per detent
};

void begin(const Config& cfg = {4, 5, 6, false, 1000});
int32_t readDeltaSteps();
bool buttonPressed();
int32_t stepHz();
void setStepHz(int32_t step);

} // namespace RotaryInput
