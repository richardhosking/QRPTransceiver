#include "DisplayUI.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

namespace DisplayUI {

const uint8_t TFT_CS   = 17;
const uint8_t TFT_DC   = 20;
const uint8_t TFT_RST  = 21;
const uint8_t TFT_MOSI = 19;
const uint8_t TFT_SCK  = 18;
const uint8_t TFT_BL   = 22;
const int8_t  TFT_MISO = 16;

static Adafruit_ILI9341 s_tft(TFT_CS, TFT_DC, TFT_RST);

static constexpr int16_t kSMeterBarX = 8;
static constexpr int16_t kSMeterBarY = 164;
static constexpr int16_t kSMeterBarW = 304;
static constexpr int16_t kSMeterBarH = 18;

static constexpr int16_t kSMeterCalX[7] = {44, 82, 120, 158, 196, 234, 272};
static const char* const kSMeterLabels[7] = {"1", "2", "4", "6", "9", "+10", "+20"};

static int16_t mapRawToSMeterX(uint16_t raw) {
  if (raw <= kSMeterCalRaw[0]) {
    const int32_t x = kSMeterBarX + ((int32_t)(kSMeterCalX[0] - kSMeterBarX) * raw) / kSMeterCalRaw[0];
    return static_cast<int16_t>(x);
  }

  for (size_t i = 1; i < 7; ++i) {
    if (raw <= kSMeterCalRaw[i]) {
      const uint16_t spanRaw = kSMeterCalRaw[i] - kSMeterCalRaw[i - 1];
      const uint16_t deltaRaw = raw - kSMeterCalRaw[i - 1];
      const int16_t spanX = kSMeterCalX[i] - kSMeterCalX[i - 1];
      const int32_t x = kSMeterCalX[i - 1] + (static_cast<int32_t>(spanX) * deltaRaw) / spanRaw;
      return static_cast<int16_t>(x);
    }
  }

  const uint16_t extraRaw = (raw > kSMeterCalRaw[6]) ? (raw - kSMeterCalRaw[6]) : 0;
  const uint16_t tailRawSpan = 4095U - kSMeterCalRaw[6];
  const int16_t tailXSpan = (kSMeterBarX + kSMeterBarW - 1) - kSMeterCalX[6];
  const int32_t x = kSMeterCalX[6] + (static_cast<int32_t>(tailXSpan) * extraRaw) / tailRawSpan;
  if (x > (kSMeterBarX + kSMeterBarW - 1)) {
    return kSMeterBarX + kSMeterBarW - 1;
  }
  return static_cast<int16_t>(x);
}

Adafruit_ILI9341& tft() {
  return s_tft;
}

void begin(uint32_t spiHz) {
  s_tft.begin(spiHz);
}

ProbeData readProbeData() {
  ProbeData data{};
  data.madctl = s_tft.readcommand8(ILI9341_MADCTL);
  data.pixfmt = s_tft.readcommand8(ILI9341_PIXFMT);
  data.id1 = s_tft.readcommand8(0xD3, 1);
  data.id2 = s_tft.readcommand8(0xD3, 2);
  data.id3 = s_tft.readcommand8(0xD3, 3);
  return data;
}

void configureUi() {
  s_tft.setRotation(1); // 320x240 landscape
  s_tft.invertDisplay(false);
}

void drawSplashScreen() {
  s_tft.fillScreen(ILI9341_BLACK);

  s_tft.setTextColor(ILI9341_CYAN);
  s_tft.setTextSize(3);
  s_tft.setCursor(40, 60);
  s_tft.println("QRP Transceiver");

  s_tft.setTextColor(ILI9341_WHITE);
  s_tft.setTextSize(2);
  s_tft.setCursor(90, 110);
  s_tft.println("Version 3.0");

  s_tft.setTextColor(ILI9341_YELLOW);
  s_tft.setTextSize(1);
  s_tft.setCursor(60, 150);
  s_tft.println("Raspberry Pi Pico  |  Arduino");
}

void drawMainScreen() {
  s_tft.fillScreen(ILI9341_BLACK);

  // Header
  s_tft.setTextColor(ILI9341_CYAN);
  s_tft.setTextSize(1);
  s_tft.setCursor(8, 6);
  s_tft.print("VK3BFX QRP Transceiver");

  // Large frequency lane
  s_tft.drawRoundRect(4, 20, 312, 86, 6, ILI9341_CYAN);

  // Mode and S-meter section
  s_tft.setTextColor(ILI9341_WHITE);
  s_tft.setTextSize(2);
  s_tft.setCursor(8, 116);
  s_tft.print("Mode:");

  s_tft.setTextSize(1);
  s_tft.setCursor(8, 150);
  s_tft.print("S meter");
  s_tft.drawRect(kSMeterBarX, kSMeterBarY, kSMeterBarW, kSMeterBarH, ILI9341_WHITE);
  for (size_t i = 0; i < 7; ++i) {
    s_tft.drawFastVLine(kSMeterCalX[i], kSMeterBarY + 1, kSMeterBarH - 2, ILI9341_DARKGREY);
    s_tft.setCursor(kSMeterCalX[i] - ((i >= 5) ? 8 : 3), 186);
    s_tft.print(kSMeterLabels[i]);
  }
}

void updateFrequencyDisplay(uint64_t vfoFreq, Mode currentMode) {
  // Clear dynamic fields
  s_tft.fillRect(8, 28, 304, 72, ILI9341_BLACK);
  s_tft.fillRect(86, 116, 224, 18, ILI9341_BLACK);

  // Format: KHz.Hz  e.g. 7100.000
  uint32_t f    = static_cast<uint32_t>(vfoFreq);
  uint32_t khz  = f / 1000;
  uint16_t hz   = f % 1000;
  char buf[24];
  snprintf(buf, sizeof(buf), "%lu.%03u", static_cast<unsigned long>(khz), hz);

  s_tft.setTextColor(ILI9341_GREEN);
  s_tft.setTextSize(5);
  s_tft.setCursor(10, 40);
  s_tft.print(buf);

  // Mode
  s_tft.setTextColor(ILI9341_YELLOW);
  s_tft.setTextSize(2);
  s_tft.setCursor(86, 116);
  s_tft.println(modeName(currentMode));
}

void updateSMeterDisplay(uint16_t averageRaw, uint16_t peakRaw) {
  if (averageRaw > 4095U) averageRaw = 4095U;
  if (peakRaw > 4095U) peakRaw = 4095U;

  const int16_t avgX = mapRawToSMeterX(averageRaw);
  const int16_t peakX = mapRawToSMeterX(peakRaw);

  // Keep a clear box around the S-meter while refreshing the inside.
  s_tft.drawRect(kSMeterBarX, kSMeterBarY, kSMeterBarW, kSMeterBarH, ILI9341_WHITE);
  s_tft.fillRect(kSMeterBarX + 1, kSMeterBarY + 1, kSMeterBarW - 2, kSMeterBarH - 2, ILI9341_BLACK);

  const int16_t barWidth = avgX - (kSMeterBarX + 1);
  if (barWidth > 0) {
    s_tft.fillRect(kSMeterBarX + 1, kSMeterBarY + 1, barWidth, kSMeterBarH - 2, ILI9341_GREEN);
  }

  // Peak hold marker (5-second peak) as a thin red line.
  s_tft.drawFastVLine(peakX, kSMeterBarY + 1, kSMeterBarH - 2, ILI9341_RED);
}

const char* modeName(Mode m) {
  switch (m) {
    case Mode::LSB: return "LSB";
    case Mode::USB: return "USB";
    case Mode::CW:  return "CW";
    case Mode::FT8: return "FT8";
    case Mode::WSPR:return "WSPR";
    default:        return "?";
  }
}

} // namespace DisplayUI
