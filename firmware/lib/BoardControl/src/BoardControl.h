#pragma once

#include <stdint.h>

namespace BoardControl {

static constexpr uint16_t kDefaultSMeterAveragePeriodMs = 100;
static constexpr uint16_t kDefaultSMeterPeakPeriodMs = 5000;
static constexpr uint16_t kDefaultSMeterZeroRaw = 0;
static constexpr uint16_t kDefaultSMeterGainPermille = 1000;
static constexpr bool kDefaultSMeterUseLogResponse = false;

enum class Target : uint8_t {
  None = 0,
  Receiver = 1 << 0,
  Transmitter = 1 << 1,
  Controller = 1 << 2,
  AllBoards = Receiver | Transmitter
};

enum class CommandType : uint8_t {
  None = 0,
  TuneDelta,
  CycleMode,
  CycleBand,
  CycleStep,
  SaveSettings,
  ToggleTxRx,
  PowerShortPress,
  PowerLongPress
};

enum class MuteOutput : uint8_t {
  RxMute = 0,
  SsbMute,
  CwMute,
  Count
};

struct Command {
  CommandType type;
  uint8_t targetMask;
  int32_t value;
};

struct Config {
  bool mapEncoderButtonToMode;
  int8_t rxMutePin;
  int8_t ssbMutePin;
  int8_t cwMutePin;
  int8_t sMeterPin;
  uint16_t sMeterAveragePeriodMs;
  uint16_t sMeterPeakPeriodMs;
  uint16_t sMeterZeroRaw;
  uint16_t sMeterGainPermille;
  bool sMeterUseLogResponse;
};

void begin(const Config& cfg = {
  true,
  12,
  13,
  14,
  26,
  kDefaultSMeterAveragePeriodMs,
  kDefaultSMeterPeakPeriodMs,
  kDefaultSMeterZeroRaw,
  kDefaultSMeterGainPermille,
  kDefaultSMeterUseLogResponse
});
void update();
bool read(Command& command);
bool targetsBoard(const Command& command, Target target);
const char* commandName(CommandType type);
void setMute(MuteOutput output, bool muted);
bool isMuted(MuteOutput output);
uint16_t readSMeterRaw();
uint16_t readSMeterAveragedRaw();
uint16_t readSMeterPeak5sRaw();

} // namespace BoardControl