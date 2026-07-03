#include "BoardControl.h"

#include <Arduino.h>
#include <math.h>
#include <PushButtons.h>
#include <RotaryInput.h>

namespace BoardControl {

namespace {

constexpr uint8_t kQueueCapacity = 8;
constexpr uint16_t kPowerToggleHoldMs = 1500;

Config s_cfg{
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
};
Command s_queue[kQueueCapacity]{};
uint8_t s_head = 0;
uint8_t s_tail = 0;
uint8_t s_count = 0;
bool s_muted[3] = {false, false, false};
uint32_t s_sMeterAccum = 0;
uint16_t s_sMeterSampleCount = 0;
uint16_t s_sMeterLatestRaw = 0;
uint16_t s_sMeterAverageRaw = 0;
uint16_t s_sMeterPeak5sRaw = 0;
unsigned long s_sMeterAvgStartMs = 0;
unsigned long s_sMeterPeakStartMs = 0;
bool s_powerWasDown = false;
bool s_powerLongPressHandled = false;
unsigned long s_powerPressStartMs = 0;

constexpr uint8_t maskFor(Target target) {
  return static_cast<uint8_t>(target);
}

static int8_t pinFor(MuteOutput output) {
  switch (output) {
    case MuteOutput::RxMute:  return s_cfg.rxMutePin;
    case MuteOutput::SsbMute: return s_cfg.ssbMutePin;
    case MuteOutput::CwMute:  return s_cfg.cwMutePin;
    default:                  return -1;
  }
}

bool enqueue(CommandType type, uint8_t targetMask, int32_t value = 0) {
  if (s_count >= kQueueCapacity) {
    return false;
  }

  s_queue[s_tail] = {type, targetMask, value};
  s_tail = (s_tail + 1) % kQueueCapacity;
  ++s_count;
  return true;
}

void updateSMeter() {
  if (s_cfg.sMeterPin < 0) {
    return;
  }

  const unsigned long now = millis();
  const uint16_t rawSample = static_cast<uint16_t>(analogRead(static_cast<uint8_t>(s_cfg.sMeterPin)));
  uint32_t sample = static_cast<uint16_t>(4095U - rawSample);

  if (sample <= s_cfg.sMeterZeroRaw) {
    sample = 0;
  } else {
    sample -= s_cfg.sMeterZeroRaw;
  }

  sample = (sample * s_cfg.sMeterGainPermille) / 1000U;
  if (sample > 4095U) {
    sample = 4095U;
  }

  if (s_cfg.sMeterUseLogResponse) {
    // Quasi-log response: expand low-level changes and compress high levels.
    // sqrt(norm) approximates the visual response expected for S-meter behavior.
    const float norm = static_cast<float>(sample) / 4095.0f;
    sample = static_cast<uint32_t>(sqrtf(norm) * 4095.0f + 0.5f);
  }

  s_sMeterLatestRaw = sample;
  s_sMeterAccum += sample;
  ++s_sMeterSampleCount;

  if ((now - s_sMeterAvgStartMs) >= s_cfg.sMeterAveragePeriodMs) {
    if (s_sMeterSampleCount > 0) {
      s_sMeterAverageRaw = static_cast<uint16_t>(s_sMeterAccum / s_sMeterSampleCount);
      if (s_sMeterAverageRaw > s_sMeterPeak5sRaw) {
        s_sMeterPeak5sRaw = s_sMeterAverageRaw;
      }
    }

    s_sMeterAccum = 0;
    s_sMeterSampleCount = 0;
    s_sMeterAvgStartMs = now;
  }

  if ((now - s_sMeterPeakStartMs) >= s_cfg.sMeterPeakPeriodMs) {
    s_sMeterPeakStartMs = now;
    s_sMeterPeak5sRaw = s_sMeterAverageRaw;
  }
}

} // namespace

void begin(const Config& cfg) {
  s_cfg = cfg;
  if (s_cfg.sMeterAveragePeriodMs == 0) {
    s_cfg.sMeterAveragePeriodMs = 1;
  }
  if (s_cfg.sMeterPeakPeriodMs < s_cfg.sMeterAveragePeriodMs) {
    s_cfg.sMeterPeakPeriodMs = s_cfg.sMeterAveragePeriodMs;
  }
  s_head = 0;
  s_tail = 0;
  s_count = 0;
  s_sMeterAccum = 0;
  s_sMeterSampleCount = 0;
  s_sMeterLatestRaw = 0;
  s_sMeterAverageRaw = 0;
  s_sMeterPeak5sRaw = 0;
  s_sMeterAvgStartMs = millis();
  s_sMeterPeakStartMs = s_sMeterAvgStartMs;
  s_powerWasDown = false;
  s_powerLongPressHandled = false;
  s_powerPressStartMs = 0;

  if (s_cfg.sMeterPin >= 0) {
    pinMode(static_cast<uint8_t>(s_cfg.sMeterPin), INPUT);
  }

  // Initialize mute pins
  for (uint8_t i = 0; i < static_cast<uint8_t>(MuteOutput::Count); ++i) {
    const auto output = static_cast<MuteOutput>(i);
    const int8_t pin = pinFor(output);
    if (pin >= 0) {
      pinMode(static_cast<uint8_t>(pin), OUTPUT);
      digitalWrite(static_cast<uint8_t>(pin), LOW);
      s_muted[i] = false;
    }
  }
}

void update() {
  updateSMeter();
  PushButtons::update();

  const int32_t deltaSteps = RotaryInput::readDeltaSteps();
  if (deltaSteps != 0) {
    enqueue(CommandType::TuneDelta, maskFor(Target::AllBoards), deltaSteps);
  }

  // Use encoder push as a STEP fallback control.
  // MODE still has a dedicated front-panel button.
  if (RotaryInput::buttonPressed()) {
    enqueue(CommandType::CycleStep, maskFor(Target::AllBoards));
  }

  if (PushButtons::pressed(PushButtons::ButtonId::Mode)) {
    enqueue(CommandType::CycleMode, maskFor(Target::AllBoards));
  }
  if (PushButtons::pressed(PushButtons::ButtonId::Band)) {
    enqueue(CommandType::CycleBand, maskFor(Target::AllBoards));
  }
  if (PushButtons::pressed(PushButtons::ButtonId::Step)) {
    enqueue(CommandType::CycleStep, maskFor(Target::AllBoards));
  }
  if (PushButtons::pressed(PushButtons::ButtonId::Fn)) {
    enqueue(CommandType::CycleMode, maskFor(Target::AllBoards));
  }

  const unsigned long now = millis();
  const bool powerDown = PushButtons::isDown(PushButtons::ButtonId::Power);
  if (powerDown && !s_powerWasDown) {
    s_powerWasDown = true;
    s_powerLongPressHandled = false;
    s_powerPressStartMs = now;
  }

  if (powerDown && s_powerWasDown && !s_powerLongPressHandled) {
    if ((now - s_powerPressStartMs) >= kPowerToggleHoldMs) {
      enqueue(CommandType::PowerLongPress, maskFor(Target::Controller));
      s_powerLongPressHandled = true;
    }
  }

  if (!powerDown && s_powerWasDown) {
    if (!s_powerLongPressHandled) {
      enqueue(CommandType::PowerShortPress, maskFor(Target::Controller));
    }
    s_powerWasDown = false;
    s_powerLongPressHandled = false;
    s_powerPressStartMs = 0;
  }

  if (PushButtons::pressed(PushButtons::ButtonId::TxRx)) {
    enqueue(CommandType::ToggleTxRx, maskFor(Target::AllBoards));
  }
}

bool read(Command& command) {
  if (s_count == 0) {
    return false;
  }

  command = s_queue[s_head];
  s_head = (s_head + 1) % kQueueCapacity;
  --s_count;
  return true;
}

bool targetsBoard(const Command& command, Target target) {
  return (command.targetMask & maskFor(target)) != 0;
}

const char* commandName(CommandType type) {
  switch (type) {
    case CommandType::TuneDelta: return "TuneDelta";
    case CommandType::CycleMode: return "CycleMode";
    case CommandType::CycleBand: return "CycleBand";
    case CommandType::CycleStep: return "CycleStep";
    case CommandType::SaveSettings: return "SaveSettings";
    case CommandType::ToggleTxRx: return "ToggleTxRx";
    case CommandType::PowerShortPress: return "PowerShortPress";
    case CommandType::PowerLongPress: return "PowerLongPress";
    case CommandType::None:
    default:
      return "None";
  }
}

void setMute(MuteOutput output, bool muted) {
  const uint8_t idx = static_cast<uint8_t>(output);
  const int8_t pin = pinFor(output);

  if (pin < 0) return;

  s_muted[idx] = muted;
  digitalWrite(static_cast<uint8_t>(pin), muted ? HIGH : LOW);
}

bool isMuted(MuteOutput output) {
  const uint8_t idx = static_cast<uint8_t>(output);
  return s_muted[idx];
}

uint16_t readSMeterRaw() {
  return s_sMeterLatestRaw;
}

uint16_t readSMeterAveragedRaw() {
  return s_sMeterAverageRaw;
}

uint16_t readSMeterPeak5sRaw() {
  return s_sMeterPeak5sRaw;
}

} // namespace BoardControl