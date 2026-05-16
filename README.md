# QRPTransceiver

QRPTransceiver is an HF transceiver project using phasing techniques, surface-mount components, KiCad hardware design files, and Raspberry Pi Pico firmware for control and user interface.

## Overview

The repository combines:

- **hardware design** in KiCad
- **firmware** for Raspberry Pi Pico using PlatformIO/Arduino
- **local oscillator control** using an SI5351
- **front-panel UI** using a rotary encoder, push buttons, and an ILI9341 TFT display

The design is centered on a phasing-based architecture for HF operation and is being developed as an integrated hardware/firmware system.

## Repository Structure

```text
.
├── src/                # Main firmware application
├── lib/                # Firmware libraries (UI, input, SI5351 control, etc.)
├── include/            # Shared firmware headers
├── test/               # Firmware tests
├── scripts/            # Utility scripts
├── Datasheets/         # Reference component datasheets
├── Gerber/             # Manufacturing outputs
├── pdf/                # Exported documents
├── QRPTransceiver.kicad_pro
├── QRPTransceiver.kicad_sch
├── QRPTransceiver.kicad_pcb
└── platformio.ini
```

## Main Subsystems

### Hardware
- main transceiver schematic and PCB
- hierarchical sheets for mixer, polyphase, and receiver audio sections
- local symbol and footprint libraries
- manufacturing outputs and design reports

### Firmware
- `src/main.cpp` as the control entry point
- `lib/DisplayUI` for the TFT display layer
- `lib/RotaryInput` for tuning encoder input
- `lib/PushButtons` for front-panel button handling
- `lib/SI5351Control` for synthesizer control and band-specific quadrature setup

## Documentation

Project documentation is organized under `docs/`:

- `docs/architecture.md` — overall system architecture
- `docs/hardware-overview.md` — hardware design structure and subsystem roles
- `docs/firmware-overview.md` — firmware organization and responsibilities

## Current Status

The repository currently contains:
- active KiCad project files
- firmware for Pico-based control and display
- generated outputs and reference materials
- some legacy, backup, and intermediate design artifacts

As the project evolves, the intent is to keep the current KiCad 8 project files and PlatformIO firmware as the primary editable sources.

## Toolchain

### Hardware
- KiCad 8 project files are present in the repository

### Firmware
- PlatformIO
- Arduino framework for Raspberry Pi Pico
- SI5351 library
- Adafruit GFX / ILI9341 display libraries

## Notes

This project is an active mixed-domain design spanning:

- RF/analog circuit design
- phasing-based transceiver architecture
- embedded firmware
- user interface development
- calibration and layout refinement

For architectural details, start with `docs/architecture.md`.
