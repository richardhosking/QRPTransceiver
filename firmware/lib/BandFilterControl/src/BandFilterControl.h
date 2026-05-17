#pragma once

#include <stddef.h>
#include <stdint.h>

namespace BandFilterControl {

enum class InterfaceType : uint8_t {
  Pcf8574I2C,
  Gpio4Bit
};

struct Config {
  InterfaceType interfaceType;
  uint8_t i2cAddress;
  int8_t gpioBit0;
  int8_t gpioBit1;
  int8_t gpioBit2;
  int8_t gpioBit3;
  bool activeHigh;
};

void begin(const Config& cfg = {InterfaceType::Pcf8574I2C, 0x20, -1, -1, -1, -1, true});
bool setFilterIndex(uint8_t filterIndex);
bool setBandMapping(const uint8_t* mapping, size_t count);
bool applyForBand(uint8_t bandIndex);
uint8_t currentFilterIndex();

} // namespace BandFilterControl