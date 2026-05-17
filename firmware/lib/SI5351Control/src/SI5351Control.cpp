#include "SI5351Control.h"

#include <Arduino.h>
#include <Wire.h>
#include <si5351.h>

namespace SI5351Control {

struct DeviceState {
  Si5351* si5351;
  bool ready;
  uint64_t vfoHz;
  uint32_t xtalHz;
  uint8_t i2cAddress;
};

static Si5351 s_si5351Addr60(SI5351_BUS_BASE_ADDR);
static Si5351 s_si5351Addr61(SI5351_BUS_BASE_ADDR + 1);
static DeviceState s_deviceStates[] = {
  {&s_si5351Addr60, false, 0, 25000000UL, SI5351_BUS_BASE_ADDR},
  {&s_si5351Addr61, false, 0, 25000000UL, static_cast<uint8_t>(SI5351_BUS_BASE_ADDR + 1)}
};

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
  {Band::B6M,   30, 0, 1000000, 25, 0x00, 0x19, "6m"},
};

static const BandProfile* findBandProfile(Band band) {
  for (const auto& p : kBandProfiles) {
    if (p.band == band) return &p;
  }
  return nullptr;
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

static uint8_t rDivEnumForValue(uint16_t rDiv) {
  switch (rDiv) {
    case 1: return SI5351_OUTPUT_CLK_DIV_1;
    case 2: return SI5351_OUTPUT_CLK_DIV_2;
    case 4: return SI5351_OUTPUT_CLK_DIV_4;
    case 8: return SI5351_OUTPUT_CLK_DIV_8;
    case 16: return SI5351_OUTPUT_CLK_DIV_16;
    case 32: return SI5351_OUTPUT_CLK_DIV_32;
    case 64: return SI5351_OUTPUT_CLK_DIV_64;
    case 128: return SI5351_OUTPUT_CLK_DIV_128;
    default: return SI5351_OUTPUT_CLK_DIV_1;
  }
}

static void programRDivForClock(Si5351& dev, enum si5351_clock clk, uint16_t rDiv) {
  uint8_t regAddr = 0;
  switch (clk) {
    case SI5351_CLK0: regAddr = SI5351_CLK0_PARAMETERS + 2; break;
    case SI5351_CLK1: regAddr = SI5351_CLK1_PARAMETERS + 2; break;
    case SI5351_CLK2: regAddr = SI5351_CLK2_PARAMETERS + 2; break;
    case SI5351_CLK3: regAddr = SI5351_CLK3_PARAMETERS + 2; break;
    case SI5351_CLK4: regAddr = SI5351_CLK4_PARAMETERS + 2; break;
    case SI5351_CLK5: regAddr = SI5351_CLK5_PARAMETERS + 2; break;
    default: return;
  }

  uint8_t regVal = dev.si5351_read(regAddr);
  regVal &= ~(0x7c); // Clear R divider and DIVBY4 bits
  regVal |= static_cast<uint8_t>(rDivEnumForValue(rDiv) << SI5351_OUTPUT_CLK_DIV_SHIFT);
  dev.si5351_write(regAddr, regVal);
}

static bool pickQuadratureDivider(uint64_t rfHz, uint16_t& dividerOut) {
  // Need an even divider so phase register value == divider for +90 deg.
  // Limit divider to <=126 so phase register fits 7 bits.
  if (rfHz == 0) return false;

  uint16_t best = 0;
  uint64_t bestErr = UINT64_MAX;
  for (uint16_t d = 8; d <= 126; d += 2) {
    const uint64_t pll = rfHz * static_cast<uint64_t>(d);
    if (pll < 600000000ULL || pll > 900000000ULL) continue;

    // Prefer PLL close to 750 MHz.
    const uint64_t err = (pll > 750000000ULL) ? (pll - 750000000ULL) : (750000000ULL - pll);
    if (err < bestErr) {
      bestErr = err;
      best = d;
    }
  }

  if (best == 0) return false;
  dividerOut = best;
  return true;
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

  state.si5351->drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  state.si5351->drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  state.si5351->output_enable(SI5351_CLK0, 1);
  state.si5351->output_enable(SI5351_CLK1, 0);
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

bool setQuadrature90(uint64_t rfHz, bool reversePhase) {
  return setQuadrature90(Device::Rx, rfHz, reversePhase);
}

bool setQuadrature90(Device device, uint64_t rfHz, bool reversePhase) {
  if (device == Device::Tx) {
    rfHz = constrainTxFrequencyHz(rfHz);
  }

  DeviceState& state = stateForDevice(device);
  if (!state.ready || rfHz == 0) return false;

  // Preferred path: direct in-chip 90° quadrature at RF output frequency.
  uint16_t ratio = 0;
  if (pickQuadratureDivider(rfHz, ratio)) {
    const uint64_t pllCentiHz = rfHz * static_cast<uint64_t>(ratio) * 100ULL;
    const uint64_t outCentiHz = rfHz * 100ULL;

    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK0);
    state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);

    state.si5351->set_clock_invert(SI5351_CLK0, 0);
    state.si5351->set_clock_invert(SI5351_CLK1, 0);
    state.si5351->set_phase(SI5351_CLK0, reversePhase ? static_cast<uint8_t>(ratio) : 0);
    state.si5351->set_phase(SI5351_CLK1, reversePhase ? 0 : static_cast<uint8_t>(ratio));
    state.si5351->pll_reset(SI5351_PLLA);

    state.si5351->output_enable(SI5351_CLK0, 1);
    state.si5351->output_enable(SI5351_CLK1, 1);
    state.vfoHz = rfHz;
    return true;
  }

  // Low-band fallback: use same PLL for both outputs at 8x RF.
  // Even higher intermediate frequency for finer phase granularity before R divider.
  // Strategy: output 8x RF from same PLL, apply phase word for 180°,
  // then R divider /8 brings frequency down and converts 180° → 90°.
  const uint64_t lo8xHz = rfHz * 8ULL;
  if (lo8xHz < 2000000ULL) return false;

  // Single PLL at 720 MHz for both clocks.
  const uint64_t pllCentiHz = 72000000000ULL; // 720 MHz
  const uint64_t outCentiHz = lo8xHz * 100ULL;

  // Both CLK0 and CLK1 use PLLA for phase coherence.
  state.si5351->set_ms_source(SI5351_CLK0, SI5351_PLLA);
  state.si5351->set_ms_source(SI5351_CLK1, SI5351_PLLA);

  state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK0);
  state.si5351->set_freq_manual(outCentiHz, pllCentiHz, SI5351_CLK1);

  // For 8x RF with 720 MHz PLL:
  // MS ratio = 720 / (8*RF) = 90 / RF
  // We want 90° at the final RF output.
  // Phase word for 90° at 8x intermediate = (MS+1)/4 (empirical fine-tuning)
  // 8x intermediate gives 4x finer phase resolution vs 4x intermediate.
  // Example: 1.8 MHz RF -> MS = 50 -> phase = 12-13 for 90°
  const uint16_t msRatio = (static_cast<uint64_t>(720000000ULL) / (8ULL * rfHz)) & 0xFFFF;
  const uint8_t phaseWord90 = static_cast<uint8_t>(((msRatio + 1) / 4) & 0x7F);

  state.si5351->set_clock_invert(SI5351_CLK0, 0);
  state.si5351->set_clock_invert(SI5351_CLK1, 0);
  state.si5351->set_phase(SI5351_CLK0, reversePhase ? phaseWord90 : 0);
  state.si5351->set_phase(SI5351_CLK1, reversePhase ? 0 : phaseWord90);
  
  // Single PLL reset for phase alignment.
  state.si5351->pll_reset(SI5351_PLLA);

  // Apply R divider /8 to both clocks to reach target RF frequency.
  programRDivForClock(*state.si5351, SI5351_CLK0, 8);
  programRDivForClock(*state.si5351, SI5351_CLK1, 8);

  state.si5351->output_enable(SI5351_CLK0, 1);
  state.si5351->output_enable(SI5351_CLK1, 1);
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
  state.si5351->output_enable(SI5351_CLK1, enable ? 1 : 0);
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
