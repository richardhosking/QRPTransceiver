#pragma once

#include <stdint.h>

namespace SI5351Control {

enum class Band : uint8_t {
  B160M,
  B80M,
  B40M,
  B30M,
  B20M,
  B17M,
  B15M,
  B12M,
  B10M
};

struct Config {
  uint32_t xtalFreqHz;
  int32_t correctionPpb;
};

bool begin(const Config& cfg = {25000000UL, 0});
void setVFO(uint64_t rfHz);
bool setQuadrature90(uint64_t rfHz);
bool setupBandQuadrature(Band band);
const char* bandName(Band band);

void enableOutput(bool enable);
bool isReady();
uint64_t currentVFO();

} // namespace SI5351Control
