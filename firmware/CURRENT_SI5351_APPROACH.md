# Current SI5351 approach

## Overview

The firmware now uses the same fixed-divider PLL retune method across all amateur bands for RX quadrature generation. This is the method that originally performed best on the lower bands and produced less audible step noise while tuning.

Two Si5351 devices are supported:

- RX at I2C address `0x60`
- TX at I2C address `0x61`

The main implementation is in [lib/SI5351Control/src/SI5351Control.cpp](lib/SI5351Control/src/SI5351Control.cpp).

## Why this method was chosen

Earlier builds used two different strategies:

1. direct in-chip phase-register quadrature on higher bands
2. fixed multisynth divider with PLL retune on lower bands

The fixed-divider method was quieter during live tuning and gave good quadrature behavior, especially on the lower bands. After explicitly disabling spread-spectrum clocking, the fixed-divider method could be used across all bands without the previously observed fixed-offset sidebands.

## Current quadrature method

For each amateur band, the firmware selects a fixed multisynth divider that keeps the PLL inside the valid Si5351 VCO range.

On 6m, the divider is chosen so the PLL remains in-range at 50 to 54 MHz; using the old 10m-style divider would push the PLL above the Si5351 VCO limit and prevent retuning.

- CLK0 and CLK1 are both sourced from PLLA.
- The output frequency stays at the requested RF frequency.
- The multisynth divider stays fixed while tuning within a band.
- Tuning is done by changing the PLL frequency only.

This gives smoother tuning because the divider does not jump around during normal in-band retunes.

### Band entry behavior

When first entering a band, or when changing `USB`/`LSB` polarity:

1. outputs are disabled
2. CLK0 and CLK1 are programmed to the same frequency from the same PLL
3. PLLA is reset for deterministic alignment
4. outputs are re-enabled
5. a short temporary frequency offset is applied to CLK1
6. CLK1 is restored to the target frequency

That brief offset acts as a phase kick and establishes the effective quadrature relationship.

### Live tuning behavior

While tuning within the same band:

- no PLL reset is performed
- outputs stay enabled
- both outputs are updated with `set_freq_manual()`
- only PLL N changes

This is the low-step-noise behavior that was preferred in testing.

## Mode behavior

Mode handling lives in [src/main.cpp](src/main.cpp).

- `LSB`, `USB`, `FT8`, and `WSPR` use the dial frequency as the LO frequency.
- `CW` offsets the LO by the sidetone pitch.
- `USB`/`FT8`/`WSPR` versus `LSB` are selected by reversing the quadrature sense, not by moving the LO to the other side.

This means the carrier should sit on the dial frequency for `LSB`, `USB`, `FT8`, and `WSPR`.

## Spread-spectrum handling

The Etherkit library does not manage Si5351 spread-spectrum clocking.

To prevent the fixed sidebands that were observed during testing, the firmware explicitly clears the Si5351 SSC parameter registers during device initialization.

Relevant code is in [lib/SI5351Control/src/SI5351Control.cpp](lib/SI5351Control/src/SI5351Control.cpp).

## Drive level and unused outputs

To reduce unnecessary output activity:

- CLK0 and CLK1 drive strength are set to 4 mA
- unused outputs are powered down
- RX/TX output enabling is managed explicitly by the firmware

## Calibration approach

Calibration is handled with two terms in [src/main.cpp](src/main.cpp):

- `SI5351_CORRECTION_PPB`: original base correction
- `SI5351_CORRECTION_TRIM_PPB`: fine trim after the spread-spectrum change

The current trim is set to the midpoint of the last two measurements so the residual error is centered near zero rather than chasing very small drift.

For digital modes such as `FT8` and `WSPR`, any remaining small residual should be smaller than normal crystal temperature drift and module-to-module variation.

## Practical summary

The present firmware strategy is:

- disable Si5351 spread-spectrum parameters at startup
- use fixed-divider PLL retune quadrature on all bands
- keep the LO on the dial frequency for `LSB`, `USB`, `FT8`, and `WSPR`
- use a small empirical calibration trim for absolute frequency accuracy

This combination currently gives the best balance of:

- low tuning noise
- acceptable quadrature behavior
- no fixed SSC-related sidebands
- good absolute frequency accuracy for narrow digital modes