#include "BandFilterControl.h"

#include <Arduino.h>
#include <Wire.h>

namespace BandFilterControl {

namespace {

Config s_cfg{InterfaceType::Pcf8574I2C, 0x20, -1, -1, -1, -1, true};
uint8_t s_currentFilter = 0;
const uint8_t* s_bandMapping = nullptr;
size_t s_bandMapCount = 0;

void writePin(int8_t pin, bool value) {
  if (pin < 0) {
    return;
  }
  digitalWrite(static_cast<uint8_t>(pin), value ? HIGH : LOW);
}

void applyAsGpio4Bit(uint8_t filterIndex) {
  const bool b0 = (filterIndex & 0x01U) != 0;
  const bool b1 = (filterIndex & 0x02U) != 0;
  const bool b2 = (filterIndex & 0x04U) != 0;
  const bool b3 = (filterIndex & 0x08U) != 0;

  writePin(s_cfg.gpioBit0, s_cfg.activeHigh ? b0 : !b0);
  writePin(s_cfg.gpioBit1, s_cfg.activeHigh ? b1 : !b1);
  writePin(s_cfg.gpioBit2, s_cfg.activeHigh ? b2 : !b2);
  writePin(s_cfg.gpioBit3, s_cfg.activeHigh ? b3 : !b3);
}

bool applyAsPcf8574(uint8_t filterIndex) {
  uint8_t busValue = 0xFFU;
  const uint8_t nibble = static_cast<uint8_t>(filterIndex & 0x0FU);

  if (s_cfg.activeHigh) {
    busValue = static_cast<uint8_t>((busValue & 0xF0U) | nibble);
  } else {
    busValue = static_cast<uint8_t>((busValue & 0xF0U) | (static_cast<uint8_t>(~nibble) & 0x0FU));
  }

  Wire.beginTransmission(s_cfg.i2cAddress);
  Wire.write(busValue);
  return Wire.endTransmission() == 0;
}

} // namespace

void begin(const Config& cfg) {
  s_cfg = cfg;
  s_currentFilter = 0;

  if (s_cfg.interfaceType == InterfaceType::Gpio4Bit) {
    if (s_cfg.gpioBit0 >= 0) pinMode(static_cast<uint8_t>(s_cfg.gpioBit0), OUTPUT);
    if (s_cfg.gpioBit1 >= 0) pinMode(static_cast<uint8_t>(s_cfg.gpioBit1), OUTPUT);
    if (s_cfg.gpioBit2 >= 0) pinMode(static_cast<uint8_t>(s_cfg.gpioBit2), OUTPUT);
    if (s_cfg.gpioBit3 >= 0) pinMode(static_cast<uint8_t>(s_cfg.gpioBit3), OUTPUT);
    applyAsGpio4Bit(0);
    return;
  }

  Wire.begin();
  static_cast<void>(applyAsPcf8574(0));
}

bool setFilterIndex(uint8_t filterIndex) {
  filterIndex &= 0x0FU;

  if (s_cfg.interfaceType == InterfaceType::Gpio4Bit) {
    applyAsGpio4Bit(filterIndex);
    s_currentFilter = filterIndex;
    return true;
  }

  if (!applyAsPcf8574(filterIndex)) {
    return false;
  }

  s_currentFilter = filterIndex;
  return true;
}

bool setBandMapping(const uint8_t* mapping, size_t count) {
  if (mapping == nullptr || count == 0) {
    s_bandMapping = nullptr;
    s_bandMapCount = 0;
    return false;
  }

  s_bandMapping = mapping;
  s_bandMapCount = count;
  return true;
}

bool applyForBand(uint8_t bandIndex) {
  if (s_bandMapping == nullptr || bandIndex >= s_bandMapCount) {
    return false;
  }

  return setFilterIndex(s_bandMapping[bandIndex]);
}

uint8_t currentFilterIndex() {
  return s_currentFilter;
}

} // namespace BandFilterControl