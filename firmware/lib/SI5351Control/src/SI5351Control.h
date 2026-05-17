#pragma once

#include <stddef.h>
#include <stdint.h>

namespace SI5351Control {

struct FrequencyRangeHz {
  uint64_t low;
  uint64_t high;
};

// Australian amateur HF/VHF allocations used to constrain TX frequency programming.
static constexpr FrequencyRangeHz kTxBandLimitsHz[] = {
  {1800000ULL, 1875000ULL},   // 160m
  {3500000ULL, 3800000ULL},   // 80m
  {7000000ULL, 7300000ULL},   // 40m
  {10100000ULL, 10150000ULL}, // 30m
  {14000000ULL, 14350000ULL}, // 20m
  {18068000ULL, 18168000ULL}, // 17m
  {21000000ULL, 21450000ULL}, // 15m
  {24890000ULL, 24990000ULL}, // 12m
  {28000000ULL, 29700000ULL}, // 10m
  {50000000ULL, 54000000ULL}  // 6m
};

static constexpr size_t kTxBandLimitCount = sizeof(kTxBandLimitsHz) / sizeof(kTxBandLimitsHz[0]);

enum class Device : uint8_t {
  Rx = 0,
  Tx
};

enum class Band : uint8_t {
  B160M,
  B80M,
  B40M,
  B30M,
  B20M,
  B17M,
  B15M,
  B12M,
  B10M,
  B6M
};

struct Config {
  uint32_t xtalFreqHz;
  int32_t correctionPpb;
};

struct DeviceConfig {
  uint8_t i2cAddress;
  uint32_t xtalFreqHz;
  int32_t correctionPpb;
};

bool begin(const Config& cfg = {25000000UL, 0});
bool beginDevice(Device device, const DeviceConfig& cfg);
void setVFO(uint64_t rfHz);
void setVFO(Device device, uint64_t rfHz);
bool setQuadrature90(uint64_t rfHz, bool reversePhase = false);
bool setQuadrature90(Device device, uint64_t rfHz, bool reversePhase = false);
bool setupBandQuadrature(Band band);
bool setupBandQuadrature(Device device, Band band);
const char* bandName(Band band);

void enableOutput(bool enable);
void enableOutput(Device device, bool enable);
bool isReady();
bool isReady(Device device);
uint64_t currentVFO();
uint64_t currentVFO(Device device);

} // namespace SI5351Control
