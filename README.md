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
├── firmware/
│   ├── src/                # Main firmware application
│   ├── lib/                # Firmware libraries (UI, input, SI5351 control, etc.)
│   ├── include/            # Shared firmware headers
│   ├── test/               # Firmware tests
│   ├── scripts/            # Utility scripts
│   └── platformio.ini      # PlatformIO project file
├── hardware/
│   ├── kicad/
│   │   ├── QRPTransceiver.kicad_pro
│   │   ├── QRPTransceiver.kicad_sch
│   │   ├── QRPTransceiver.kicad_pcb
│   │   ├── QRPTransceiver.pretty/   # Local footprint library
│   │   └── lib_sch/                 # Local symbol library files
│   └── outputs/            # Manufacturing outputs, exports, and generated artifacts
├── docs/
│   ├── architecture.md
│   ├── bringup.md
│   ├── calibration.md
│   ├── firmware-overview.md
│   ├── hardware-overview.md
│   ├── pinout.md
│   └── summaries/          # Summary notes and rework history
├── reference/
│   ├── datasheets/         # Reference component datasheets
│   └── licenses/           # License/reference documents
├── archive/
│   ├── backups/            # Backup zips, cache/rescue files, and *-bak artifacts
│   ├── legacy-kicad/       # Legacy KiCad schematic/netlist files
│   └── temporary/          # Autosave, lock, and temporary design files
└── README.md
```

The repository is now organized so that active firmware lives under `firmware/`, the active KiCad project lives under `hardware/kicad/`, generated hardware outputs live under `hardware/outputs/`, and reference material is stored under `reference/`.

## Main Subsystems

### Hardware
- main transceiver schematic and PCB
- hierarchical sheets for mixer, polyphase, and receiver audio sections
- local symbol and footprint libraries
- manufacturing outputs and design reports

### Firmware
- `firmware/src/main.cpp` as the control entry point
- `firmware/lib/DisplayUI` for the TFT display layer
- `firmware/lib/RotaryInput` for tuning encoder input
- `firmware/lib/PushButtons` for front-panel button handling
- `firmware/lib/SI5351Control` for synthesizer control and band-specific quadrature setup

## Documentation

Project documentation is organized under `docs/`:

- `docs/architecture.md` — overall system architecture
- `docs/bringup.md` — bring-up notes and observations
- `docs/calibration.md` — calibration workflow and notes
- `docs/hardware-overview.md` — hardware design structure and subsystem roles
- `docs/firmware-overview.md` — firmware organization and responsibilities
- `docs/pinout.md` — firmware-visible pin assignments
- `docs/summaries/` — summary notes for key refactors, programming flow, and quadrature correction work

## Current Status

The repository currently contains:
- active KiCad project files under `hardware/kicad/`
- firmware for Pico-based control and display under `firmware/`
- generated outputs under `hardware/outputs/`
- reference materials under `reference/`
- archived legacy, backup, and intermediate design artifacts under `archive/`

As the project evolves, the active KiCad project and PlatformIO firmware remain the primary editable sources in their dedicated subdirectories.

## Toolchain

### Hardware
- KiCad 8 project files are present under `hardware/kicad/`

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
