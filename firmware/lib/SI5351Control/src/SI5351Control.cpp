#include "SI5351Control.h"

#include <Arduino.h>
#include <Wire.h>
#include <si5351.h>

namespace SI5351Control {

struct DeviceState {
  Si5351* si5351;
  bool ready;
  bool outputsEnabled;
  uint8_t lowBandProfile;
  bool lastReversePhase;
  uint64_t vfoHz;
  uint32_t xtalHz;
  uint8_t i2cAddress;
};

static Si5351 s_si5351Addr60(SI5351_BUS_BASE_ADDR);
static Si5351 s_si5351Addr61(SI5351_BUS_BASE_ADDR + 1);
static DeviceState s_deviceStates[] = {
  {&s_si5351Addr60, false, true, 0, 0, 25000000UL, SI5351_BUS_BASE_ADDR},
  {&s_si5351Addr61, false, false, 0, 0, 25000000UL, static_cast<uint8_t>(SI5351_BUS_BASE_ADDR + 1)}
};

// Low-band (below 7 MHz) empirical phase trims in phase-word steps
// for the 8x-intermediate /8-R-divider method.
// Keep 160m as reference from scope work; tune 80m by +/-1 if needed.
static constexpr int8_t kLowBandPhaseTrim160m = 4;
static constexpr int8_t kLowBandPhaseTrim80m = 0;
// Diagnostic switch: force CLK1 off to isolate single-output spur behavior.
static constexpr bool kDebugDisableClk1 = false;
// Diagnostic switch: bypass quadrature/manual PLL path and run CLK0 only
// through the library's set_freq() path.
static constexpr bool kDebugSingleClockOnly = false;

struct BandProfile {
  Band band;
  uint32_t pllA;
  uint32_t pllB;
  uint32_t pllC;
  uint16_t divider;
  uint8_t phaseClk0;
  uint8_t phaseClk1;
  const char* name;
};

static const BandProfile kBandProfiles[] = {
    {Band::B160M, 28, 0, 62500, 400, 0x9B, 0x64, "160m"},
    {Band::B80M,  28, 3000, 12500, 200, 0x9B, 0x64, "80m"},
    {Band::B40M,  28, 100, 250000, 100, 0x00, 0x64, "40m"},
    {Band::B30M,  32, 137000, 312500, 80, 0x00, 0x50, "30m"},
    {Band::B20M,  28, 0, 500000, 50, 0x00, 0x32, "20m"},
    {Band::B17M,  28, 0, 625000, 40, 0x00, 0x28, "17m"},
    {Band::B15M,  33, 0, 625000, 40, 0x00, 0x28, "15m"},
    {Band::B12M,  28, 0, 833333, 30, 0x00, 0x1E, "12m"},
    {Band::B10M,  28, 0, 1000000, 25, 0x00, 0x19, "10m"},
  {Band::B6M,   28, 0, 1000000, 14, 0x00, 0x0E, "6m"},
};

static const BandProfile* findBandProfile(Band band) {
  for (const auto& p : kBandProfiles) {
    if (p.band == band) return &p;
  }
  return nullptr;
}

static bool bandProfileForFrequencyHz(uint64_t rfHz, uint8_t& profileOut, uint16_t& dividerOut) {
  if (rfHz >= 1800000ULL && rfHz <= 2000000ULL) {
    profileOut = 1;
    dividerOut = 334;
    return true;
  }
  if (rfHz >= 3500000ULL && rfHz <= 3800000ULL) {
    profileOut = 2;
    dividerOut = 172;
    return true;
  }
  if (rfHz >= 7000000ULL && rfHz <= 7300000ULL) {
    profileOut = 3;
    dividerOut = 100;
    return true;
  }
  if (rfHz >= 10100000ULL && rfHz <= 10150000ULL) {
    profileOut = 4;
    dividerOut = 80;
    return true;
  }
  if (rfHz >= 14000000ULL && rfHz <= 14350000ULL) {
    profileOut = 5;
    dividerOut = 50;
    return true;
  }
  if (rfHz >= 18068000ULL && rfHz <= 18168000ULL) {
    profileOut = 6;
    dividerOut = 40;
    return true;
  }
  if (rfHz >= 21000000ULL && rfHz <= 21450000ULL) {
    profileOut = 7;
    dividerOut = 40;
    return true;
  }
  if (rfHz >= 24890000ULL && rfHz <= 24990000ULL) {
    profileOut = 8;
    dividerOut = 30;
    return true;
  }
  if (rfHz >= 28000000ULL && rfHz <= 29700000ULL) {
    profileOut = 9;
    dividerOut = 25;
    return true;
  }
  if (rfHz >= 50000000ULL && rfHz <= 54000000ULL) {
    profileOut = 10;
    dividerOut = 14;
    return true;
  }

  return false;
}

static uint64_t constrainTxFrequencyHz(uint64_t requestedHz) {
  uint64_t nearestHz = kTxBandLimitsHz[0].low;
  uint64_t nearestDelta = (requestedHz > nearestHz) ? (requestedHz - nearestHz) : (nearestHz - requestedHz);

  for (const auto& range : kTxBandLimitsHz) {
    if (requestedHz >= range.low && requestedHz <= range.high) {
      return requestedHz;
    }

    const uint64_t lowDelta = (requestedHz > range.low) ? (requestedHz - range.low) : (range.low - requestedHz);
    if (lowDelta < nearestDelta) {
      nearestDelta = lowDelta;
      nearestHz = range.low;
    }

    const uint64_t highDelta = (requestedHz > range.high) ? (requestedHz - range.high) : (range.high - requestedHz);
    if (highDelta < nearestDelta) {
      nearestDelta = highDelta;
      nearestHz = range.high;
    }
  }

  return nearestHz;
}

static DeviceState& stateForDevice(Device device) {
  switch (device) {
    case Device::Tx:
      return s_deviceStates[1];
    case Device::Rx:
    default:
      return s_deviceStates[0];
  }
}

static Si5351* chipForAddress(uint8_t address) {
  if (address == SI5351_BUS_BASE_ADDR) {
    return &s_si5351Addr60;
  }
  if (address == static_cast<uint8_t>(SI5351_BUS_BASE_ADDR + 1)) {
    return &s_si5351Addr61;
  }
  return nullptr;
}

static uint64_t pllFreqCentiHz(const BandProfile& p, uint32_t xtalHz) {
  // PLL = XTAL * (a + b/c), returned in 0.01 Hz units.
  const uint64_t num = static_cast<uint64_t>(p.pllA) * p.pllC + p.pllB;
  return (static_cast<uint64_t>(xtalHz) * 100ULL * num) / p.pllC;
}

bool begin(const Config& cfg) {
  return beginDevice(Device::Rx, {SI5351_BUS_BASE_ADDR, cfg.xtalFreqHz, cfg.correctionPpb});
}

bool beginDevice(Device device, const DeviceConfig& cfg) {
  Wire.begin();
  DeviceState& state = stateForDevice(device);
  Si5351* chip = chipForAddress(cfg.i2cAddress);
  if (!chip) {
    state.ready = false;
    return false;
  }

  state.si5351 = chip;
  state.i2cAddress = cfg.i2cAddress;
  state.xtalHz = cfg.xtalFreqHz;

  // Etherkit SI5351 expects frequency in hundredths of Hz for set_freq().
  if (!state.si5351->init(SI5351_CRYSTAL_LOAD_8PF, cfg.xtalFreqHz, cfg.correctionPpb)) {
    state.ready = false;
    return false;
  }

  // Explicitly disable spread-spectrum clocking (SSC).
  // Some modules may power up with non-zero SSC registers; clear all params.
  for (uint8_t reg = SI5351_SSC_PARAM0; reg <= SI5351_SSC_PARAM12; ++reg) {
    state.si5351->si5351_write(reg, 0x00);
  }

  // Lower drive strength to reduce output-stage current steps that can couple
  // into fixed-offset sidebands on some builds/layouts.
  state.si5351->drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  state.si5351->drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  state.si5351->set_clock_pwr(SI5351_CLK0, 1);
  state.si5351->set_clock_pwr(SI5351_CLK1, 1);
  state.si5351->set_clock_pwr(SI5351_CLK3, 0);
  state.si5351->set_clock_pwr(SI5351_CLK4, 0);
  state.si5351->set_clock_pwr(SI5351_CLK5, 0);
  state.si5351->set_clock_pwr(SI5351_CLK6, 0);
  state.si5351->set_clock_pwr(SI5351_CLK7, 0);
  state.si5351->output_enable(SI5351_CLK0, 1);
  state.si5351->output_enable(SI5351_CLK1, 0);
  state.si5351->output_enable(SI5351_CLK2, 0);
  state.si5351->set_clock_pwr(SI5351_CLK2, 0);
  state.outputsEnabled = (device != Device::Tx);
  state.lowBandProfile = 0;
  state.lastReversePhase = false;
  state.ready = true;
  return true;
}

void setVFO(uint64_t rfHz) {
  setVFO(Device::Rx, rfHz);
}

void setVFO(Device device, uint64_t rfHz) {
  if (device == Device::Tx) {
    rfHz = constrainTxFrequencyHz(rfHz);
  }

  DeviceState& state = stateForDevice(device);
  state.vfoHz = rfHz;
  if (!state.ready) return;

  const uint64_t freqCentiHz = rfHz * 100ULL;
  state.si5351->set_freq(freqCentiHz, SI5351_CLK0);
}

bool setQuadrature90(uint64_t rfHz, bool reversePhase, bool liveRetune) {
  return setQuadrature90(Device::Rx, rfHz, reversePhase, liveRetune);
}

bool setQuadrature90(Device device, uint64_t rfHz, bool reversePhase, bool liveRetune) {
  if (device == Device::Tx) {
    rfHz = constrainTxFrequencyHz(rfHz);
  }

  DeviceState& state = stateForDevice(device);
  if (!state.ready || rfHz == 0) return false;

  if (kDebugSingleClockOnly) {
    const uint64_t outCentiHz = rfHz * 100ULL;
    state.si5351->output_enable(SI5351_CLK1, 0);
    state.si5351->set_clock_pwr(SI5351_CLK1, 0);
    state.si5351->set_clock_pwr(SI5351_CLK0, 1);
    state.si5351->output_enable(SI5351_CLK0, state.outputsEnabled ? 1 : 0);
    state.si5351->set_freq(outCentiHz, SI5351_CLK0);
    state.lowBandProfile = 0;
    state.lastReversePhase = reversePhase;
    state.vfoHz = rfHz;
    return true;
  }

  // Preferred quadrature path on all bands: keep the multisynth divider fixed
  // per amateur band and retune by changing PLL (N) only. This proved quieter
  // during tuning than the direct phase-register path once SSC was disabled.
  const uint64_t outCentiHz = rfHz * 100ULL;
  uint8_t nextLowBandProfile = 3;
  uint16_t fixedDivider = 120;
  if (!bandProfileForFrequencyHz(rfHz, nextLowBandProfile, fixedDivider)) {
    return false;
  }
  const uint64_t pllCentiHz = rfHz * static_cast<uint64_t>(fixedDivider) * 100ULL;
  if (pllCentiHz < 60000000000ULL || pllCentiHz > 90000000000ULL) return false;
  const bool firstSelectInBand = (state.lowBandProfile != nextLowBandProfile) ||
                                  (state.lastReversePhase != reversePhase);

  // Keep both clocks powered and on a single PLLA.
  state.si5351->set_clock_pwr(SI5351_CLK0, 1);
  state.si5351->set_clock_pwr(SI5351_CLK1, 1);
  state.si5351->set_ms_source(SI5351_CLK0, SI5351_PLLA);
  state.si5351->set_ms_source(SI5351_CLK1, SI5351_PLLA);

  if (firstSelectInBand) {
    // Entry routine (once per band/mode entry):
    // disable outputs, sync, then run a brief +1 Hz phase-kick routine.
    state.si5351->output_enable(SI5351_CLK0, 0);
    state.si5351->output_enable(SI5351_CLK1, 0);

    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK0);
    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);
    state.si5351->pll_reset(SI5351_PLLA);

    if (state.outputsEnabled) {
      state.si5351->output_enable(SI5351_CLK0, 1);
      state.si5351->output_enable(SI5351_CLK1, kDebugDisableClk1 ? 0 : 1);
      if (kDebugDisableClk1) {
        state.si5351->set_clock_pwr(SI5351_CLK1, 0);
      }

      // Frequency-shift phase kick: move CLK1 by +25 Hz briefly, then restore.
      // Phase accumulates at 360°/s per Hz of offset, so 25 Hz for 10 ms = 90°.
      // Adjust kPhaseKickMs to trim the final phase angle (more ms = more phase).
      constexpr uint64_t kPhaseKickDeltaCentiHz = 2410ULL; // 24.70 Hz × 6 ms ≈ 90°
      constexpr uint16_t kPhaseKickMs = 6;
      // Negate the kick for USB vs LSB to produce +90° or -90°.
      const uint64_t kickFreq = reversePhase ? (outCentiHz - kPhaseKickDeltaCentiHz)
                                             : (outCentiHz + kPhaseKickDeltaCentiHz);
      state.si5351->set_freq_manual(kickFreq, pllCentiHz, SI5351_CLK1);
      delay(kPhaseKickMs);
      state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);
    }

    state.lowBandProfile = nextLowBandProfile;
    state.lastReversePhase = reversePhase;
  } else {
    // Live tuning updates in-band: no PLL reset, no power-down.
    // Divider remains fixed for this band; PLL N changes with rfHz.
    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK0);
    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);
  }

  state.vfoHz = rfHz;
  return true;
}

bool setupBandQuadrature(Band band) {
  return setupBandQuadrature(Device::Rx, band);
}

bool setupBandQuadrature(Device device, Band band) {
  DeviceState& state = stateForDevice(device);
  if (!state.ready) return false;

  const BandProfile* p = findBandProfile(band);
  if (!p) return false;

  const uint64_t pllCentiHz = pllFreqCentiHz(*p, state.xtalHz);
  const uint64_t outCentiHz = pllCentiHz / p->divider;

  state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK0);
  state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);

  // Preserve legacy band phase behavior from prior project.
  state.si5351->si5351_write(SI5351_CLK0_PHASE_OFFSET, p->phaseClk0);
  state.si5351->si5351_write(SI5351_CLK1_PHASE_OFFSET, p->phaseClk1);
  state.si5351->pll_reset(SI5351_PLLA);

  state.si5351->output_enable(SI5351_CLK0, 1);
  state.si5351->output_enable(SI5351_CLK1, 1);
  state.vfoHz = outCentiHz / 100ULL;
  return true;
}

const char* bandName(Band band) {
  const BandProfile* p = findBandProfile(band);
  return p ? p->name : "?";
}

void enableOutput(bool enable) {
  enableOutput(Device::Rx, enable);
}

void enableOutput(Device device, bool enable) {
  DeviceState& state = stateForDevice(device);
  if (!state.ready) return;
  state.si5351->output_enable(SI5351_CLK0, enable ? 1 : 0);
  state.si5351->output_enable(SI5351_CLK1, (enable && !kDebugDisableClk1) ? 1 : 0);
  state.si5351->set_clock_pwr(SI5351_CLK0, enable ? 1 : 0);
  state.si5351->set_clock_pwr(SI5351_CLK1, (enable && !kDebugDisableClk1) ? 1 : 0);
  state.outputsEnabled = enable;
}

bool isReady() {
  return isReady(Device::Rx);
}

bool isReady(Device device) {
  return stateForDevice(device).ready;
}

uint64_t currentVFO() {
  return currentVFO(Device::Rx);
}

uint64_t currentVFO(Device device) {
  return stateForDevice(device).vfoHz;
}

} // namespace SI5351Control
