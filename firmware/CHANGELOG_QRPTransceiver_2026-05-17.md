# QRP Transceiver Firmware Change Log

Date: 2026-05-17
Project: QRPTransceiver firmware

## Overview
This document summarizes the implemented firmware changes during the recent integration session.

## New Libraries Added

### BoardControl
Path: lib/BoardControl/src/
- Added a command routing layer between physical inputs and control actions.
- Added command queue and typed commands for tune/mode/band/step/save/TX-RX toggle.
- Added mute output control APIs and state tracking.
- Added S-meter analog input support (default GP26 / ADC0).
- Added S-meter signal processing:
  - averaged value over configurable period
  - 5-second peak hold value
- Exposed configurable S-meter timing in header config.

### BandFilterControl
Path: lib/BandFilterControl/src/
- Added per-band filter selection control.
- Supports two output interfaces:
  - PCF8574 on I2C (default, address 0x20)
  - 4-bit direct GPIO fallback
- Added per-band filter mapping and apply-on-band-change behavior.

## Existing Libraries Updated

### PushButtons
Path: lib/PushButtons/src/
- Added TX/RX button support.
- Extended button enum and config to include TxRx input (default GP11).

### SI5351Control
Path: lib/SI5351Control/src/
- Refactored from single-device to multi-device support:
  - RX device
  - TX device
- Added per-device APIs for begin/tune/output control.
- Supports same I2C bus with separate SI5351 addresses:
  - RX default: 0x60
  - TX default: 0x61
- Added optional quadrature phase reversal (used for sideband handling).
- Added explicit 6m profile in band profile table.
- Added TX frequency limiting against AU amateur ranges.
- Exposed TX limit ranges in header as editable constants.

### DisplayUI
Path: lib/DisplayUI/src/
- Reworked main screen layout:
  - top small heading: VK3BFX QRP Transceiver
  - large frequency field in KHz.Hz format
  - mode row below frequency
  - S-meter row below mode
- Added live S-meter renderer:
  - solid filled bar for averaged signal
  - thin red line for 5-second peak
  - boxed display area
- Added calibration scale labels: 1, 2, 4, 6, 9, +10, +20.
- Exposed S-meter calibration raw points in header for easy tuning.

## Main Firmware Updates
Path: src/main.cpp
- Migrated input processing to BoardControl event consumption.
- Added TX/RX state handling with mute policy:
  - TX mode: CWMUTE, RXMUTE, SSBMUTE all HIGH.
  - RX mode: RXMUTE LOW in all modes.
- Updated mode-based mute mapping:
  - LSB/USB/FT8/WSPR: CWMUTE HIGH, SSBMUTE LOW
  - CW: CWMUTE LOW, SSBMUTE HIGH
- Added CW offset support (700 Hz, CW-L convention).
- Added sideband quadrature mapping:
  - USB/FT8/WSPR use reversed quadrature
  - LSB/CW use normal quadrature
- Extended step tuning cycle:
  - 1, 10, 100, 1k, 10k, 100k, 1M Hz
- Extended settings persistence to remember per-band mode and frequency.
- Added startup TX SI5351 initialization scaffold.
- Added startup and band-change filter selection calls.
- Added periodic S-meter UI updates from averaged + peak values.

## Settings Persistence Changes
Path: src/main.cpp
- Settings version bumped to v2.
- Flash record now stores:
  - per-band frequency
  - per-band mode
  - current band, current mode, step index
- Existing v1 saved records are ignored by design after upgrade.

## Pin and Bus Allocation Summary
- Rotary encoder: GP2, GP3, button GP6
- Function buttons: GP7, GP8, GP9, GP10, TX/RX GP11
- Mute outputs: GP12 RXMUTE, GP13 SSBMUTE, GP14 CWMUTE
- S-meter analog input: GP26
- SI5351 RX: I2C address 0x60
- SI5351 TX: I2C address 0x61
- Filter expander default: PCF8574 at I2C address 0x20

## Build/Validation
- PlatformIO builds were repeatedly run after major changes.
- Final state reported successful compilation.

## Documentation Added
- Added power latch circuit documentation for hardware power control:
  - POWER_SWITCH_CIRCUIT.txt
  - Covers short-press power-on, long-press power-off, and firmware POWER_HOLD behavior.

## Recommended Bench Bring-Up Order
1. Verify I2C device detection at 0x60, 0x61, and 0x20.
2. Verify RX/TX mute logic on physical lines.
3. Verify per-band filter code switching.
4. Verify CW offset and sideband direction on-air/with signal generator.
5. Calibrate S-meter raw thresholds in DisplayUI header.

## Key Files
- src/main.cpp
- lib/BoardControl/src/BoardControl.h
- lib/BoardControl/src/BoardControl.cpp
- lib/BandFilterControl/src/BandFilterControl.h
- lib/BandFilterControl/src/BandFilterControl.cpp
- lib/SI5351Control/src/SI5351Control.h
- lib/SI5351Control/src/SI5351Control.cpp
- lib/DisplayUI/src/DisplayUI.h
- lib/DisplayUI/src/DisplayUI.cpp
- lib/PushButtons/src/PushButtons.h
- lib/PushButtons/src/PushButtons.cpp
