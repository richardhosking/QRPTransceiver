# QRPTransceiver Bring-Up Guide

## 1. Purpose

This document describes a practical first-power-up and validation sequence for QRPTransceiver. The aim is to reduce troubleshooting time by testing the system in controlled stages, starting with power and digital control, then moving into oscillator, audio, and RF/phasing behavior.

## 2. Preconditions

Before powering the board, confirm:

- the current KiCad 8 project files are the intended hardware baseline
- the board has been visually inspected for solder bridges, polarity mistakes, and missing parts
- test equipment is available as needed
- the current firmware builds successfully in PlatformIO
- the Raspberry Pi Pico can be programmed successfully

Recommended minimum equipment:

- bench supply with current limit
- digital multimeter
- oscilloscope
- frequency counter or calibrated receiver if available
- audio monitor, speaker, or headphones as appropriate
- RF signal source if available

## 3. Firmware / Toolchain Baseline

The repository firmware is configured for PlatformIO with Raspberry Pi Pico Arduino support. The default environment is `pico_bootsel`, with a `pico` environment also defined.

Important baseline details:

- framework: Arduino
- board: pico
- default environment: `pico_bootsel`
- alternate upload method: `picotool`
- serial monitor speed: 115200

Before hardware bring-up, confirm that a clean firmware build completes.

## 4. Known Firmware-Controlled Interfaces

The current firmware defines these visible control connections:

### Display
- TFT CS: GP17
- TFT DC: GP20
- TFT RST: GP21
- TFT MOSI: GP19
- TFT SCK: GP18
- TFT MISO: GP16
- TFT backlight: GP22

### Rotary encoder
- encoder A: GP2
- encoder B: GP3
- encoder button: GP6

### Push buttons
- MODE: GP7
- BAND: GP8
- STEP: GP9
- FN/SAVE: GP10

These should be used as the default expected front-panel pin mapping unless the hardware is intentionally wired differently.

## 5. Bring-Up Sequence Overview

Recommended validation order:

1. visual inspection
2. continuity and shorts check
3. power rail verification
4. Pico boot and firmware upload
5. display bring-up
6. button and encoder verification
7. SI5351 / LO verification
8. audio-stage sanity checks
9. mixer / phasing checks
10. end-to-end receive-path validation

Do not skip earlier stages. Most mixed-signal bring-up problems become harder to diagnose if RF testing starts before power and control behavior are confirmed.

## 6. Step 1: Visual Inspection

Before first power:

- inspect all IC orientation and pin-1 markings
- verify polarized capacitors and diodes
- inspect fine-pitch solder joints under magnification
- confirm no clipped leads, solder balls, or whiskers remain
- verify connectors and headers are installed in the intended orientation
- inspect RF path components for wrong values or placement swaps
- inspect the Pico and display wiring or connectors carefully

If the board contains optional population areas, confirm which configuration is currently assembled.

## 7. Step 2: Unpowered Electrical Checks

With power removed:

- measure resistance from main supply input to ground
- check for unexpected shorts on digital rails
- check for unexpected shorts on analog rails
- confirm key connector pins are not shorted together
- spot-check continuity for ground distribution and major supply rails

A low resistance is not always a fault, but anything close to a hard short should be resolved before power-up.

## 8. Step 3: First Power Application

Use a bench supply with current limiting for the first power-on.

Recommended approach:

- start at the intended input voltage
- set a conservative current limit
- power with display and external accessories disconnected if needed
- watch current draw immediately
- remove power immediately if current exceeds expectations or any part heats unexpectedly

During this stage verify:

- main input voltage reaches the board correctly
- any regulated rails are present and stable
- Pico supply is correct
- no device overheats

## 9. Step 4: Pico Boot and Firmware Upload

Once power rails are confirmed:

1. connect the Pico through USB or the intended programming path
2. build firmware in PlatformIO
3. upload using the configured method
4. open the serial monitor at 115200 baud

Expected firmware behavior includes status messages during initialization, including synthesizer startup and input-device readiness.

Useful signs of life include:

- serial output appears
- the built-in LED toggles periodically
- repeated `alive` messages appear approximately every 5 seconds

If firmware does not start:

- verify the correct PlatformIO environment is selected
- verify USB/programming access to the Pico
- confirm no hardware wiring conflict is preventing boot
- reduce attached peripherals if needed and retest

## 10. Step 5: Display Bring-Up

After firmware is running, verify the TFT display.

Expected behavior from the current firmware:

- splash screen appears
- main screen is drawn after initialization
- frequency, mode, and status regions become visible

If the backlight is wired correctly but no image appears:

- verify SPI wiring
- verify CS/DC/RST mapping
- verify display controller compatibility with the library
- confirm the display is powered at the correct voltage
- inspect for solder faults on display connections

If the display remains blank, treat this as a digital interface issue before moving to RF debugging.

## 11. Step 6: Encoder and Button Verification

Once the display and serial console are alive, validate the controls.

### Encoder
Rotate the encoder and confirm:

- displayed frequency changes
- tuning increments match the current step size
- direction is correct
- no excessive jitter or skipped counts is observed

### Encoder push button
Press the encoder button and confirm:

- mode changes as expected

### Front-panel buttons
Press MODE, BAND, STEP, and FN/SAVE in turn and confirm:

- MODE changes operating mode
- BAND changes band selection
- STEP changes tuning resolution
- FN/SAVE stores settings when they are dirty

If controls are unreliable:

- verify pin wiring against the documented GPIO mapping
- inspect switch grounding and pull-up behavior
- check debounce-related symptoms
- verify no pin conflicts exist with external hardware

## 12. Step 7: SI5351 / LO Verification

After control input is working, verify the oscillator subsystem.

Expected firmware behavior:

- SI5351 initialization is attempted at boot
- quadrature setup is attempted first
- fallback to CLK0-only output is used if quadrature setup fails

Validation steps:

1. probe SI5351 output with oscilloscope or counter
2. confirm output frequency tracks the displayed VFO frequency
3. rotate encoder and verify output changes appropriately
4. change bands and observe expected frequency behavior
5. confirm outputs are stable and present after boot

If output frequency is offset but stable, this may be a calibration issue rather than a wiring issue.

If no output is present:

- check SI5351 supply voltage
- verify I2C wiring
- inspect crystal or reference connections
- confirm the device address and library assumptions match the assembled hardware

## 13. Step 8: Audio-Stage Sanity Checks

Before full RF testing, verify the receiver audio section at a basic level.

Checks may include:

- DC bias points are reasonable
- no op-amp output is railed unexpectedly
- no stage is oscillating or clipping at idle
- audio path has no obvious short to ground or supply

Where practical:

- inject a low-level test signal at an appropriate stage
- observe expected gain/filter response
- monitor output for hiss, tone, or recoverable signal activity

## 14. Step 9: Mixer and Phasing Checks

The mixer and polyphase sections are central to this design, so test them carefully.

Suggested checks:

- verify LO reaches the intended mixer input points
- confirm expected phase-related signals exist where applicable
- compare amplitudes between corresponding paths
- look for major imbalance or missing paths
- verify behavior changes logically with tuning and band changes

Because this is a phasing-based architecture, sideband rejection depends on analog balance. If receive performance is poor, compare the corresponding I/Q or phased paths before changing firmware.

## 15. Step 10: End-to-End Receive Validation

Once power, firmware, display, input, and LO behavior are verified, test the complete receive chain.

Suggested methods:

- connect an antenna or controlled RF source
- tune to a known strong HF signal
- verify tuning changes move the receive response
- compare band behavior across multiple amateur bands
- monitor audio quality and any obvious image/sideband issues

Document observations such as:

- dead bands
- weak or distorted reception
- poor sideband suppression
- tuning discontinuities
- frequency offset from known signals

## 16. Calibration Notes

The firmware already includes an SI5351 correction term, so frequency alignment is expected to be adjustable in software.

Calibration topics that should be addressed after basic bring-up:

- absolute frequency correction
- quadrature phase balance
- gain matching between paths
- sideband suppression tuning
- band-to-band consistency

These belong in a dedicated `docs/calibration.md` document and should not be improvised repeatedly during bring-up.

## 17. Troubleshooting Priorities

When the board does not function, debug in this order:

1. power
2. Pico boot and firmware execution
3. display and serial diagnostics
4. control inputs
5. SI5351 output
6. audio stages
7. mixer/phasing network
8. end-to-end RF behavior

This order avoids chasing RF symptoms that are actually caused by a missing rail or failed digital initialization.

## 18. Bring-Up Log Template

It is useful to keep a per-board record including:

- board revision
- assembly date
- firmware commit or tag
- supply voltage and current at first power-on
- regulator measurements
- display result
- encoder/button result
- SI5351 output frequency result
- first receive-path observations
- calibration notes
- known defects or rework performed

## 19. Summary

The safest bring-up path for QRPTransceiver is to validate the design as a sequence of subsystems:

- power first
- then Pico and firmware
- then display and controls
- then SI5351 frequency generation
- then audio and phasing sections
- then end-to-end receive performance

Following this order makes faults easier to isolate and reduces the risk of misdiagnosing calibration issues as hardware failures.
