/*
main.cpp for QRP transceiver V3 with Raspberry Pi Pico,
 SI5351 local oscillator and ILI9341 SPI display.
Using VSCode IDE with PlatformIO extension for development and upload.
Rotary encoder tuning with push button, plus 3 additional push buttons for mode/band/step.
Display, rotary, push buttons and SI5351 code abstracyted into libraries
*/

#include <Arduino.h>
#include <stddef.h>
#include <string.h>

#include <BoardControl.h>
#include <BandFilterControl.h>
#include <DisplayUI.h>
#include <PushButtons.h>
#include <RotaryInput.h>
#include <SI5351Control.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

// SerialUSB = USB CDC (/dev/ttyACM0) on Arduino-Mbed Pico core
#if defined(SERIAL_PORT_USBVIRTUAL)
#define DBG_PORT SERIAL_PORT_USBVIRTUAL
#else
#define DBG_PORT SerialUSB
#endif

#define FW_TAG "HW_SPI_18_19_16_UI_V4"
using DisplayUI::Mode;

// SI5351 calibration (ppb):
// fine trim from measured 7,100,900 Hz at 7,100,000 Hz setpoint
// error = +900 Hz (~+126.8 ppm), so reduce magnitude of negative correction.
static constexpr int32_t SI5351_CORRECTION_PPB = -4680000;
static constexpr uint8_t SI5351_RX_I2C_ADDR = 0x60;
static constexpr uint8_t SI5351_TX_I2C_ADDR = 0x61;
static constexpr int32_t CW_PITCH_HZ = 700;

// ── VFO state ────────────────────────────────────────────────────────────────
uint64_t vfoFreq     = 1825000UL;   // current VFO frequency in Hz
Mode     currentMode = Mode::LSB;
static constexpr uint64_t VFO_MIN_HZ = 1000000ULL;
static constexpr uint64_t VFO_MAX_HZ = 54000000ULL;

static const int32_t kStepTableHz[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
static constexpr size_t kStepCount = sizeof(kStepTableHz) / sizeof(kStepTableHz[0]);
static uint8_t g_stepIndex = 2; // default 100 Hz

static const uint64_t kBandDefaultHz[] = {
  1900000ULL,   // 160m
  3575000ULL,   // 80m
  7067000ULL,   // 40m
  10136000ULL,  // 30m
  14074000ULL,  // 20m
  18100000ULL,  // 17m
  21074000ULL,  // 15m
  24915000ULL,  // 12m
  28074000ULL,  // 10m
  50313000ULL   // 6m
};
static constexpr size_t kBandCount = sizeof(kBandDefaultHz) / sizeof(kBandDefaultHz[0]);
static const uint8_t kBandFilterCode[kBandCount] = {
  0, // 160m
  1, // 80m
  2, // 40m
  3, // 30m
  4, // 20m
  5, // 17m
  6, // 15m
  7, // 12m
  8, // 10m
  9  // 6m
};
static const Mode kBandDefaultMode[kBandCount] = {
  Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB,
  Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB
};
static uint64_t g_bandFreqHz[kBandCount] = {
  1900000ULL, 3575000ULL, 7067000ULL, 10136000ULL, 14074000ULL,
  18100000ULL, 21074000ULL, 24915000ULL, 28074000ULL, 50313000ULL
};
static Mode g_bandMode[kBandCount] = {
  Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB,
  Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB, Mode::LSB
};
static uint8_t g_bandIndex = 1;
static bool g_settingsDirty = false;
static bool g_txModeActive = false;
static bool g_softPowerOff = false;

// ── Flash-backed settings ────────────────────────────────────────────────────
static constexpr uint32_t kSettingsMagic = 0x51525033UL; // "QRP3"
static constexpr uint16_t kSettingsVersion = 2;
static constexpr uint32_t kSettingsSlotAOffset = PICO_FLASH_SIZE_BYTES - (2 * FLASH_SECTOR_SIZE);
static constexpr uint32_t kSettingsSlotBOffset = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;

enum class SettingsSlot : uint8_t { None, A, B };

struct SettingsRecord {
  uint32_t magic;
  uint16_t version;
  uint16_t length;
  uint32_t sequence;
  uint8_t currentBand;
  uint8_t currentMode;
  uint8_t stepIndex;
  uint8_t reserved0;
  uint64_t bandFreqHz[kBandCount];
  uint8_t bandMode[kBandCount];
  uint32_t checksum;
  uint8_t padding[FLASH_PAGE_SIZE - (4 + 2 + 2 + 4 + 1 + 1 + 1 + 1 + (8 * kBandCount) + (1 * kBandCount) + 2 + 4)];
};

static_assert(sizeof(SettingsRecord) == FLASH_PAGE_SIZE, "SettingsRecord must fit exactly in one flash page");

static SettingsSlot g_activeSettingsSlot = SettingsSlot::None;
static uint32_t g_settingsSequence = 0;

static uint64_t clampFrequency(uint64_t hz) {
  if (hz < VFO_MIN_HZ) return VFO_MIN_HZ;
  if (hz > VFO_MAX_HZ) return VFO_MAX_HZ;
  return hz;
}

static bool isValidModeValue(uint8_t modeValue) {
  switch (static_cast<Mode>(modeValue)) {
    case Mode::LSB:
    case Mode::USB:
    case Mode::CW:
    case Mode::FT8:
    case Mode::WSPR:
      return true;
    default:
      return false;
  }
}

static uint32_t fnv1a32(const uint8_t* data, size_t len) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

static uint32_t checksumForRecord(const SettingsRecord& record) {
  return fnv1a32(reinterpret_cast<const uint8_t*>(&record), offsetof(SettingsRecord, checksum));
}

static const SettingsRecord& settingsRecordAt(uint32_t flashOffset) {
  return *reinterpret_cast<const SettingsRecord*>(XIP_BASE + flashOffset);
}

static bool isRecordValid(const SettingsRecord& record) {
  if (record.magic != kSettingsMagic) return false;
  if (record.version != kSettingsVersion) return false;
  if (record.length != sizeof(SettingsRecord)) return false;
  if (record.currentBand >= kBandCount) return false;
  if (record.stepIndex >= kStepCount) return false;
  if (!isValidModeValue(record.currentMode)) return false;
  for (size_t i = 0; i < kBandCount; ++i) {
    if (!isValidModeValue(record.bandMode[i])) return false;
  }
  if (record.checksum != checksumForRecord(record)) return false;
  return true;
}

static void markSettingsDirty() {
  g_bandFreqHz[g_bandIndex] = clampFrequency(vfoFreq);
  g_bandMode[g_bandIndex] = currentMode;
  g_settingsDirty = true;
}

static void loadRecordIntoRuntime(const SettingsRecord& record) {
  for (size_t i = 0; i < kBandCount; ++i) {
    const uint64_t candidate = record.bandFreqHz[i];
    g_bandFreqHz[i] = (candidate >= VFO_MIN_HZ && candidate <= VFO_MAX_HZ)
      ? candidate
      : kBandDefaultHz[i];

    g_bandMode[i] = isValidModeValue(record.bandMode[i])
      ? static_cast<Mode>(record.bandMode[i])
      : kBandDefaultMode[i];
  }

  g_bandIndex = record.currentBand;
  g_stepIndex = record.stepIndex;
  currentMode = g_bandMode[g_bandIndex];
  vfoFreq = g_bandFreqHz[g_bandIndex];
}

static void loadSettingsFromFlash() {
  const SettingsRecord& slotA = settingsRecordAt(kSettingsSlotAOffset);
  const SettingsRecord& slotB = settingsRecordAt(kSettingsSlotBOffset);
  const bool validA = isRecordValid(slotA);
  const bool validB = isRecordValid(slotB);

  if (!validA && !validB) {
    DBG_PORT.println("[SAVE] No valid saved settings found; using defaults");
    g_activeSettingsSlot = SettingsSlot::None;
    g_settingsSequence = 0;
    g_settingsDirty = false;
    return;
  }

  const SettingsRecord* chosen = &slotA;
  g_activeSettingsSlot = SettingsSlot::A;
  if (!validA || (validB && slotB.sequence > slotA.sequence)) {
    chosen = &slotB;
    g_activeSettingsSlot = SettingsSlot::B;
  }

  loadRecordIntoRuntime(*chosen);
  g_settingsSequence = chosen->sequence;
  g_settingsDirty = false;

  DBG_PORT.print("[SAVE] Restored slot ");
  DBG_PORT.print((g_activeSettingsSlot == SettingsSlot::A) ? 'A' : 'B');
  DBG_PORT.print(" seq=");
  DBG_PORT.print(g_settingsSequence);
  DBG_PORT.print(" band=");
  DBG_PORT.print(g_bandIndex);
  DBG_PORT.print(" freq=");
  DBG_PORT.print((uint32_t)vfoFreq);
  DBG_PORT.print(" mode=");
  DBG_PORT.println(DisplayUI::modeName(currentMode));
}

static SettingsRecord makeSettingsRecord(uint32_t nextSequence) {
  SettingsRecord record{};
  record.magic = kSettingsMagic;
  record.version = kSettingsVersion;
  record.length = sizeof(SettingsRecord);
  record.sequence = nextSequence;
  record.currentBand = g_bandIndex;
  record.currentMode = static_cast<uint8_t>(currentMode);
  record.stepIndex = g_stepIndex;
  for (size_t i = 0; i < kBandCount; ++i) {
    record.bandFreqHz[i] = g_bandFreqHz[i];
    record.bandMode[i] = static_cast<uint8_t>(g_bandMode[i]);
  }
  record.checksum = checksumForRecord(record);
  return record;
}

static bool saveSettingsIfDirty(const char* reason) {
  if (!g_settingsDirty) {
    DBG_PORT.print("[SAVE] Skipped (");
    DBG_PORT.print(reason);
    DBG_PORT.println("): no changes");
    return true;
  }

  markSettingsDirty();
  const SettingsSlot targetSlot = (g_activeSettingsSlot == SettingsSlot::A) ? SettingsSlot::B : SettingsSlot::A;
  const uint32_t targetOffset = (targetSlot == SettingsSlot::A) ? kSettingsSlotAOffset : kSettingsSlotBOffset;
  const SettingsRecord record = makeSettingsRecord(g_settingsSequence + 1);

  DBG_PORT.print("[SAVE] Writing slot ");
  DBG_PORT.print((targetSlot == SettingsSlot::A) ? 'A' : 'B');
  DBG_PORT.print(" seq=");
  DBG_PORT.print(record.sequence);
  DBG_PORT.print(" reason=");
  DBG_PORT.println(reason);
  DBG_PORT.flush();

  const uint32_t savedInterrupts = save_and_disable_interrupts();
  flash_range_erase(targetOffset, FLASH_SECTOR_SIZE);
  flash_range_program(targetOffset, reinterpret_cast<const uint8_t*>(&record), sizeof(SettingsRecord));
  restore_interrupts(savedInterrupts);

  const SettingsRecord& verify = settingsRecordAt(targetOffset);
  if (!isRecordValid(verify) || verify.sequence != record.sequence) {
    DBG_PORT.println("[SAVE] Verify failed");
    return false;
  }

  g_activeSettingsSlot = targetSlot;
  g_settingsSequence = record.sequence;
  g_settingsDirty = false;
  DBG_PORT.println("[SAVE] Complete");
  return true;
}

static bool reverseQuadratureForMode(Mode mode) {
  switch (mode) {
    case Mode::USB:
    case Mode::FT8:
    case Mode::WSPR:
      return true;
    case Mode::LSB:
    case Mode::CW:
    default:
      return false;
  }
}

static uint64_t loFrequencyForMode(uint64_t dialHz, Mode mode) {
  if (mode != Mode::CW) {
    return dialHz;
  }

  // CW-L convention: LO is below dial frequency by audio pitch.
  if (dialHz > static_cast<uint64_t>(CW_PITCH_HZ)) {
    return dialHz - static_cast<uint64_t>(CW_PITCH_HZ);
  }
  return VFO_MIN_HZ;
}

static void applyMuteOutputs() {
  if (g_txModeActive) {
    BoardControl::setMute(BoardControl::MuteOutput::RxMute, true);
    BoardControl::setMute(BoardControl::MuteOutput::SsbMute, true);
    BoardControl::setMute(BoardControl::MuteOutput::CwMute, true);
    return;
  }

  bool cwMute = false;
  bool ssbMute = true;
  if (currentMode == Mode::LSB ||
      currentMode == Mode::USB ||
      currentMode == Mode::FT8 ||
      currentMode == Mode::WSPR) {
    cwMute = true;
    ssbMute = false;
  } else if (currentMode == Mode::CW) {
    cwMute = false;
    ssbMute = true;
  }

  BoardControl::setMute(BoardControl::MuteOutput::RxMute, false);
  BoardControl::setMute(BoardControl::MuteOutput::CwMute, cwMute);
  BoardControl::setMute(BoardControl::MuteOutput::SsbMute, ssbMute);
}

static void applyBandFilter() {
  if (!BandFilterControl::applyForBand(g_bandIndex)) {
    DBG_PORT.println("[FLT] Failed to apply band filter selection");
    return;
  }

  DBG_PORT.print("[FLT] Band ");
  DBG_PORT.print(g_bandIndex);
  DBG_PORT.print(" -> filter ");
  DBG_PORT.println(BandFilterControl::currentFilterIndex());
}

static void applyVfo() {
  const uint64_t loHz = loFrequencyForMode(vfoFreq, currentMode);
  SI5351Control::setQuadrature90(SI5351Control::Device::Rx, loHz, reverseQuadratureForMode(currentMode));
  DisplayUI::updateFrequencyDisplay(vfoFreq, currentMode);
}

static void setDisplayBacklight(bool on) {
  digitalWrite(DisplayUI::TFT_BL, on ? HIGH : LOW);
}

static void enterSoftPowerOff() {
  if (g_softPowerOff) {
    return;
  }

  DBG_PORT.println("[PWR] Shutdown requested");
  saveSettingsIfDirty("Power down");

  g_txModeActive = false;
  BoardControl::setMute(BoardControl::MuteOutput::RxMute, true);
  BoardControl::setMute(BoardControl::MuteOutput::SsbMute, true);
  BoardControl::setMute(BoardControl::MuteOutput::CwMute, true);

  SI5351Control::enableOutput(SI5351Control::Device::Rx, false);
  SI5351Control::enableOutput(SI5351Control::Device::Tx, false);

  BandFilterControl::setFilterIndex(0);
  DisplayUI::enterLowPower();
  setDisplayBacklight(false);
  digitalWrite(LED_BUILTIN, LOW);

  g_softPowerOff = true;
  DBG_PORT.println("[PWR] Soft power OFF (short POWER press to wake)");
}

static void exitSoftPowerOff() {
  if (!g_softPowerOff) {
    return;
  }

  DBG_PORT.println("[PWR] Wake requested");
  setDisplayBacklight(true);
  DisplayUI::exitLowPower();
  DisplayUI::drawMainScreen();

  SI5351Control::enableOutput(SI5351Control::Device::Rx, true);
  SI5351Control::enableOutput(SI5351Control::Device::Tx, false);
  applyBandFilter();
  applyMuteOutputs();
  applyVfo();

  g_softPowerOff = false;
  DBG_PORT.println("[PWR] Soft power ON");
}

static void cycleMode() {
  switch (currentMode) {
    case Mode::LSB:  currentMode = Mode::USB;  break;
    case Mode::USB:  currentMode = Mode::CW;   break;
    case Mode::CW:   currentMode = Mode::FT8;  break;
    case Mode::FT8:  currentMode = Mode::WSPR; break;
    case Mode::WSPR: currentMode = Mode::LSB;  break;
    default:         currentMode = Mode::LSB;  break;
  }
  applyVfo();
  markSettingsDirty();
  applyMuteOutputs();

  const bool cwMute = BoardControl::isMuted(BoardControl::MuteOutput::CwMute);
  const bool ssbMute = BoardControl::isMuted(BoardControl::MuteOutput::SsbMute);
  DBG_PORT.print("[MUTE] Mode=");
  DBG_PORT.print(DisplayUI::modeName(currentMode));
  DBG_PORT.print(" CWMUTE=");
  DBG_PORT.print(cwMute);
  DBG_PORT.print(" SSB=");
  DBG_PORT.println(ssbMute);
}

static void cycleBand() {
  g_bandFreqHz[g_bandIndex] = clampFrequency(vfoFreq);
  g_bandMode[g_bandIndex] = currentMode;
  g_bandIndex = (g_bandIndex + 1) % kBandCount;
  vfoFreq = g_bandFreqHz[g_bandIndex];
  currentMode = g_bandMode[g_bandIndex];
  applyBandFilter();
  applyVfo();
  markSettingsDirty();
  saveSettingsIfDirty("Band change");
}

static void cycleStep() {
  g_stepIndex = (g_stepIndex + 1) % kStepCount;
  RotaryInput::setStepHz(kStepTableHz[g_stepIndex]);
  markSettingsDirty();
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  DBG_PORT.begin(115200);
  unsigned long serialWaitStart = millis();
  while (!DBG_PORT && (millis() - serialWaitStart < 5000)) {
    delay(10);
  }
  delay(1000);
  DBG_PORT.println("\n\n=== QRP Transceiver V3 starting ===");
  DBG_PORT.print("[FW] Built: ");
  DBG_PORT.print(__DATE__);
  DBG_PORT.print(" ");
  DBG_PORT.println(__TIME__);
  DBG_PORT.print("[FW] Tag: ");
  DBG_PORT.println(FW_TAG);
  pinMode(LED_BUILTIN, OUTPUT);

  loadSettingsFromFlash();

  // ── Display init with debug ──────────────────────────────────────────────
  DBG_PORT.println("[TFT] Configuring pins...");
  DBG_PORT.print  ("  CS=GP");   DBG_PORT.println(DisplayUI::TFT_CS);
  DBG_PORT.print  ("  DC=GP");   DBG_PORT.println(DisplayUI::TFT_DC);
  DBG_PORT.print  ("  RST=GP");  DBG_PORT.println(DisplayUI::TFT_RST);
  DBG_PORT.print  ("  MOSI=GP"); DBG_PORT.println(DisplayUI::TFT_MOSI);
  DBG_PORT.print  ("  SCK=GP");  DBG_PORT.println(DisplayUI::TFT_SCK);
  DBG_PORT.print  ("  MISO=GP"); DBG_PORT.println(DisplayUI::TFT_MISO);
  DBG_PORT.print  ("  BL=GP");   DBG_PORT.println(DisplayUI::TFT_BL);

  // Control pins
  pinMode(DisplayUI::TFT_CS, OUTPUT);
  pinMode(DisplayUI::TFT_DC, OUTPUT);
  pinMode(DisplayUI::TFT_RST, OUTPUT);
  if (DisplayUI::TFT_MISO >= 0) {
    // Leave MISO high-impedance; pull-down can mask valid readback.
    pinMode(DisplayUI::TFT_MISO, INPUT);
  }

  // Safe idle levels before init
  digitalWrite(DisplayUI::TFT_CS, HIGH);

  // Backlight on before init so we can tell if the panel lights up
  pinMode(DisplayUI::TFT_BL, OUTPUT);
  digitalWrite(DisplayUI::TFT_BL, HIGH);
  DBG_PORT.println("[TFT] Backlight HIGH");

  // Manual reset pulse
  DBG_PORT.println("[TFT] Asserting RST low...");
  pinMode(DisplayUI::TFT_RST, OUTPUT);
  digitalWrite(DisplayUI::TFT_RST, LOW);
  delay(20);
  digitalWrite(DisplayUI::TFT_RST, HIGH);
  delay(150);
  DBG_PORT.println("[TFT] RST released");

  DBG_PORT.println("[TFT] Calling tft.begin(12MHz)...");
  DisplayUI::begin(12000000);  // normal run speed (stable on Pico + short wires)
  DBG_PORT.println("[TFT] tft.begin() returned");

  // Readback only works if display MISO/SDO is actually wired.
  if (DisplayUI::TFT_MISO >= 0) {
    DisplayUI::ProbeData probe = DisplayUI::readProbeData();
    DBG_PORT.print("[TFT] MADCTL = 0x");
    DBG_PORT.println(probe.madctl, HEX);
    DBG_PORT.print("[TFT] PIXFMT = 0x");
    DBG_PORT.println(probe.pixfmt, HEX);
    DBG_PORT.print("[TFT] ID4 bytes: 0x");
    DBG_PORT.print(probe.id1, HEX);
    DBG_PORT.print(" 0x");
    DBG_PORT.print(probe.id2, HEX);
    DBG_PORT.print(" 0x");
    DBG_PORT.println(probe.id3, HEX);
  } else {
    DBG_PORT.println("[TFT] MISO not wired -> register reads will return 0x00 (expected)");
  }

  // Normal UI bring-up (ILI9341 confirmed by ID4 = 0x93 0x41)
  DisplayUI::configureUi();
  DisplayUI::drawSplashScreen();
  delay(1200);
  DisplayUI::drawMainScreen();
  DisplayUI::updateFrequencyDisplay(vfoFreq, currentMode);
  DBG_PORT.println("[TFT] UI initialized");

  DBG_PORT.println("[SI5351] Initializing RX...\n");
  if (SI5351Control::beginDevice(SI5351Control::Device::Rx,
                                 {SI5351_RX_I2C_ADDR, 25000000UL, SI5351_CORRECTION_PPB})) {
    if (SI5351Control::setQuadrature90(SI5351Control::Device::Rx,
                       loFrequencyForMode(vfoFreq, currentMode),
                       reverseQuadratureForMode(currentMode))) {
      DBG_PORT.print("[SI5351-RX] Quadrature set @ ");
      DBG_PORT.print((uint32_t)vfoFreq);
      DBG_PORT.println(" Hz");
    } else {
      SI5351Control::setVFO(SI5351Control::Device::Rx, vfoFreq);
      DBG_PORT.print("[SI5351-RX] CLK0-only fallback @ ");
      DBG_PORT.print((uint32_t)vfoFreq);
      DBG_PORT.println(" Hz");
    }
  } else {
    DBG_PORT.println("[SI5351-RX] Init failed");
  }

  DBG_PORT.println("[SI5351] Initializing TX (shared I2C, addr 0x61)...");
  if (SI5351Control::beginDevice(SI5351Control::Device::Tx,
                                 {SI5351_TX_I2C_ADDR, 25000000UL, SI5351_CORRECTION_PPB})) {
    SI5351Control::setVFO(SI5351Control::Device::Tx, vfoFreq);
    SI5351Control::enableOutput(SI5351Control::Device::Tx, false);
    DBG_PORT.println("[SI5351-TX] Ready on same I2C bus");
  } else {
    DBG_PORT.println("[SI5351-TX] Not detected at 0x61");
  }

  // Rotary encoder input (default pins: A=GP2, B=GP3, BTN=GP6)
  RotaryInput::begin({2, 3, 6, false, 100});
  RotaryInput::setStepHz(kStepTableHz[g_stepIndex]);
  DBG_PORT.println("[ENC] Ready (A=GP2 B=GP3 BTN=GP6)");

  // Push buttons for functions (MODE=GP7, BAND=GP8, STEP=GP9, FN/SAVE=GP10, TX/RX=GP11, POWER=GP15)
  PushButtons::begin({7, 8, 9, 10, 11, 15, true, 25});
  DBG_PORT.println("[BTN] Ready (MODE=GP7 BAND=GP8 STEP=GP9 FN/SAVE=GP10 TX/RX=GP11 POWER=GP15)");
  DBG_PORT.println("[SAVE] Press FN/SAVE to store current band/frequency/mode/step");

  BoardControl::begin();
  DBG_PORT.println("[CTRL] Board command routing ready (RxMute=GP12 SsbMute=GP13 CwMute=GP14 SMeter=GP26)");

  // Band filter selection via I2C GPIO expander (default PCF8574 at 0x20).
  BandFilterControl::begin({BandFilterControl::InterfaceType::Pcf8574I2C, 0x20, -1, -1, -1, -1, true});
  BandFilterControl::setBandMapping(kBandFilterCode, kBandCount);
  applyBandFilter();

  applyMuteOutputs();

  DBG_PORT.println("Ready");
}

// ── Main loop ────────────────────────────────────────────────────────────────
void loop() {
  BoardControl::update();

  if (g_softPowerOff) {
    BoardControl::Command offCommand{};
    while (BoardControl::read(offCommand)) {
      if (offCommand.type == BoardControl::CommandType::PowerShortPress) {
        exitSoftPowerOff();
      }
    }

#if defined(__arm__) || defined(__thumb__)
    __WFI();
#else
    delay(20);
#endif
    return;
  }

  static unsigned long lastBeat = 0;
  if (millis() - lastBeat >= 1000) {
    lastBeat = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 5000) {
    lastLog = millis();
    DBG_PORT.print("alive Sraw=");
    DBG_PORT.print(BoardControl::readSMeterRaw());
    DBG_PORT.print(" Savg100ms=");
    DBG_PORT.print(BoardControl::readSMeterAveragedRaw());
    DBG_PORT.print(" Speak5s=");
    DBG_PORT.println(BoardControl::readSMeterPeak5sRaw());
  }

  static unsigned long lastSMeterUiMs = 0;
  if (millis() - lastSMeterUiMs >= 100) {
    lastSMeterUiMs = millis();
    DisplayUI::updateSMeterDisplay(BoardControl::readSMeterAveragedRaw(),
                                   BoardControl::readSMeterPeak5sRaw());
  }

  BoardControl::Command command{};
  while (BoardControl::read(command)) {
    switch (command.type) {
      case BoardControl::CommandType::TuneDelta: {
        int64_t next = static_cast<int64_t>(vfoFreq)
          + static_cast<int64_t>(command.value) * RotaryInput::stepHz();
        if (next < static_cast<int64_t>(VFO_MIN_HZ)) next = static_cast<int64_t>(VFO_MIN_HZ);
        if (next > static_cast<int64_t>(VFO_MAX_HZ)) next = static_cast<int64_t>(VFO_MAX_HZ);

        vfoFreq = static_cast<uint64_t>(next);
        g_bandFreqHz[g_bandIndex] = vfoFreq;
        applyVfo();
        g_settingsDirty = true;
        break;
      }
      case BoardControl::CommandType::CycleMode:
        cycleMode();
        break;
      case BoardControl::CommandType::CycleBand:
        cycleBand();
        break;
      case BoardControl::CommandType::CycleStep:
        cycleStep();
        break;
      case BoardControl::CommandType::SaveSettings:
        saveSettingsIfDirty("Fn button");
        break;
      case BoardControl::CommandType::ToggleTxRx: {
        g_txModeActive = !g_txModeActive;
        applyMuteOutputs();
        DBG_PORT.print("[MUTE] TX mode ");
        DBG_PORT.println(g_txModeActive ? "ON (CWMUTE/RXMUTE/SSBMUTE all HIGH)"
                                        : "OFF (RX mode mutes restored)");
        break;
      }
      case BoardControl::CommandType::PowerLongPress:
        enterSoftPowerOff();
        break;
      case BoardControl::CommandType::PowerShortPress:
        // Short POWER presses are only used for wake while in soft-off state.
        break;
      case BoardControl::CommandType::None:
      default:
        break;
    }
  }
}