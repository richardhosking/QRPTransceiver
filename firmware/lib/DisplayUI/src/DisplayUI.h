#pragma once

#include <stddef.h>
#include <stdint.h>
#include <Adafruit_ILI9341.h>

namespace DisplayUI {

extern const uint8_t TFT_CS;   // GP17 – Chip Select
extern const uint8_t TFT_DC;   // GP20 – Data/Command
extern const uint8_t TFT_RST;  // GP21 – Reset
extern const uint8_t TFT_MOSI; // GP19 – SPI0 TX
extern const uint8_t TFT_SCK;  // GP18 – SPI0 SCK
extern const uint8_t TFT_BL;   // GP22 – Backlight
extern const int8_t  TFT_MISO; // GP16 – SPI0 RX

enum class Mode : uint8_t { LSB, USB, CW, FT8, WSPR };

// S-meter calibration points (ADC raw values on 0..4095 scale) for labels:
// 1, 2, 4, 6, 9, +10, +20
static constexpr uint16_t kSMeterCalRaw[] = {350, 700, 1200, 1800, 2500, 3200, 3900};
static constexpr size_t kSMeterCalCount = sizeof(kSMeterCalRaw) / sizeof(kSMeterCalRaw[0]);

struct ProbeData {
  uint8_t madctl;
  uint8_t pixfmt;
  uint8_t id1;
  uint8_t id2;
  uint8_t id3;
};

Adafruit_ILI9341& tft();
void begin(uint32_t spiHz = 12000000);
ProbeData readProbeData();
void configureUi();
void drawSplashScreen();
void drawMainScreen();
void updateFrequencyDisplay(uint64_t vfoFreq, Mode currentMode);
void updateSMeterDisplay(uint16_t averageRaw, uint16_t peakRaw);
void enterLowPower();
void exitLowPower();
const char* modeName(Mode m);

} // namespace DisplayUI
