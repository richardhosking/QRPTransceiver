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

  // Frequency display area
  s_tft.drawRect(0, 0, 320, 60, ILI9341_CYAN);

  // Mode label
  s_tft.setTextColor(ILI9341_WHITE);
  s_tft.setTextSize(2);
  s_tft.setCursor(10, 70);
  s_tft.print("Mode: ");

  // Band label
  s_tft.setCursor(10, 95);
  s_tft.print("Band: ");

  // S-meter placeholder
  s_tft.setTextColor(ILI9341_WHITE);
  s_tft.setTextSize(1);
  s_tft.setCursor(10, 130);
  s_tft.println("S-Meter:");
  s_tft.drawRect(10, 142, 200, 16, ILI9341_WHITE);

  // Status bar
  s_tft.drawFastHLine(0, 225, 320, ILI9341_CYAN);
  s_tft.setTextColor(ILI9341_CYAN);
  s_tft.setCursor(4, 228);
  s_tft.println("RX  |  AGC: ON  |  ATT: OFF");
}

void updateFrequencyDisplay(uint64_t vfoFreq, Mode currentMode) {
  // Clear frequency and mode fields
  s_tft.fillRect(1, 1, 318, 58, ILI9341_BLACK);
  s_tft.fillRect(70, 70, 200, 18, ILI9341_BLACK);
  s_tft.fillRect(70, 95, 200, 18, ILI9341_BLACK);

  // Format: MHz.kHz.Hz  e.g. 7.100.000
  uint32_t f    = static_cast<uint32_t>(vfoFreq);
  uint16_t mhz  = f / 1000000;
  uint16_t khz  = (f % 1000000) / 1000;
  uint16_t hz   = f % 1000;
  char buf[20];
  snprintf(buf, sizeof(buf), "%u.%03u.%03u Hz", mhz, khz, hz);

  s_tft.setTextColor(ILI9341_GREEN);
  s_tft.setTextSize(3);
  s_tft.setCursor(10, 15);
  s_tft.print(buf);

  // Mode
  s_tft.setTextColor(ILI9341_YELLOW);
  s_tft.setTextSize(2);
  s_tft.setCursor(70, 70);
  s_tft.println(modeName(currentMode));

  // Band (simple lookup)
  s_tft.setCursor(70, 95);
  if      (vfoFreq >= 1800000  && vfoFreq <= 2000000)  s_tft.println("160m");
  else if (vfoFreq >= 3500000  && vfoFreq <= 4000000)  s_tft.println("80m");
  else if (vfoFreq >= 7000000  && vfoFreq <= 7300000)  s_tft.println("40m");
  else if (vfoFreq >= 10100000 && vfoFreq <= 10150000) s_tft.println("30m");
  else if (vfoFreq >= 14000000 && vfoFreq <= 14350000) s_tft.println("20m");
  else if (vfoFreq >= 18068000 && vfoFreq <= 18168000) s_tft.println("17m");
  else if (vfoFreq >= 21000000 && vfoFreq <= 21450000) s_tft.println("15m");
  else if (vfoFreq >= 28000000 && vfoFreq <= 29700000) s_tft.println("10m");
  else if (vfoFreq >= 50000000 && vfoFreq <= 54000000) s_tft.println("6m");
  else                                                  s_tft.println("--");
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
