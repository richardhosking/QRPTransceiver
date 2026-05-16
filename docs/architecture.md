# QRPTransceiver Architecture

## 1. Purpose

QRPTransceiver is a low-power HF transceiver project that combines:

- an analog phasing-based RF/baseband architecture
- SI5351-based local oscillator generation
- Raspberry Pi Pico digital control
- a TFT-based user interface
- KiCad hardware design files and PlatformIO-based firmware

The repository currently contains both the hardware design and the firmware used to control the synthesizer, user input devices, and display.

## 2. Project Goals

The current design appears to target the following goals:

- build a compact HF SSB transceiver
- use phasing techniques rather than a conventional crystal IF
- support multiple amateur HF bands
- provide digitally controlled tuning and band/mode selection
- use low-cost and widely available components
- keep the firmware modular enough to separate UI, tuning input, and synthesizer control

## 3. High-Level System Overview

At a high level, the system is organized into these major subsystems:

1. **RF and analog signal path**
   - receives and processes RF signals
   - performs phasing/mixing operations
   - routes signal energy through receiver audio/baseband stages

2. **Quadrature frequency generation**
   - generates the LO signals required by the phasing architecture
   - uses the SI5351 clock generator as the main frequency source
   - applies band-specific quadrature setup logic in firmware

3. **Control firmware**
   - maintains VFO frequency, band, and mode state
   - handles persistent settings storage
   - coordinates synthesizer, display, buttons, and rotary input

4. **User interface**
   - presents frequency and mode information on an ILI9341 TFT display
   - allows tuning and control through a rotary encoder and push buttons

5. **Hardware documentation and manufacturing outputs**
   - KiCad schematic and PCB files
   - generated Gerbers, PDFs, and reports
   - reference datasheets and design notes

## 4. Repository-Level Architecture

The repository contains two tightly coupled but distinct domains:

### 4.1 Hardware design domain

The hardware domain is centered on KiCad project files, including:

- main project and board files
- top-level hierarchical schematic
- subsystem schematics such as mixer, polyphase, and receiver audio sections
- PCB layout
- footprint and symbol libraries
- manufacturing outputs and reports

This domain defines the transceiver's electrical implementation and board layout.

### 4.2 Firmware domain

The firmware domain is a PlatformIO/Arduino-style codebase for Raspberry Pi Pico. It contains:

- application entry point in `src/`
- reusable libraries in `lib/`
- headers in `include/`
- scripts and tests
- configuration in `platformio.ini`

This domain implements the runtime behavior of the transceiver controller.

## 5. Hardware Architecture

## 5.1 Top-level schematic structure

The KiCad top-level schematic is organized using hierarchical sheets. The important hardware blocks currently visible in the design are:

- main transceiver sheet
- mixer section
- polyphase section
- receiver audio section

This indicates a design where the signal path is intentionally partitioned into functional analog subsystems.

## 5.2 RF / phasing concept

The project is described as an **SSB HF transceiver using phasing techniques**. In practical terms, this implies:

- quadrature-related signal generation and processing are central to the design
- the mixer and polyphase sections are likely responsible for phase relationships used to reject the unwanted sideband
- sideband suppression depends on maintaining amplitude and phase accuracy across the signal chain

Because phasing architectures are sensitive to imbalance, calibration and layout discipline are important parts of the overall architecture.

## 5.3 Main analog subsystems

### Mixer subsystem

The mixer subsystem is expected to:

- combine RF and local oscillator signals
- generate or recover quadrature baseband/audio components
- participate in sideband selection or rejection depending on signal direction

This block should be treated as one of the primary RF-critical sections of the design.

### Polyphase subsystem

The polyphase subsystem is expected to:

- generate or process 90-degree phase-shifted signals
- support phasing-based image or sideband cancellation
- define frequency-sensitive analog behavior that may need tuning or characterization

This is a core architectural block for any phasing transceiver.

### Receiver audio subsystem

The receiver audio subsystem is expected to:

- filter and amplify low-frequency recovered signals
- condition audio for monitoring or downstream processing
- provide an analog endpoint for receive-path verification during bring-up

This block is a natural place for gain shaping, filtering, and audio output conditioning.

## 5.4 PCB architecture

The PCB file indicates the project includes a board layout and manufacturing outputs. At an architectural level, the PCB should be understood as containing at least these physical partitions:

- RF-sensitive analog paths
- local oscillator / clock routing
- audio/baseband analog stages
- digital control and display connections
- power distribution and grounding

For maintainability and performance, the layout should preserve clear separation between noisy digital circuits and sensitive analog/RF sections.

## 6. Frequency Generation and Synthesizer Architecture

## 6.1 SI5351 as the main LO source

The firmware includes a dedicated `SI5351Control` library. This makes the SI5351 the primary digital frequency source for the transceiver.

Responsibilities of this subsystem include:

- initializing the SI5351
- setting VFO frequency
- enabling/disabling outputs
- configuring band-specific quadrature behavior
- exposing synthesizer readiness and current frequency state

## 6.2 Band-aware quadrature control

The synthesizer code defines band profiles for amateur bands including:

- 160 m
- 80 m
- 40 m
- 30 m
- 20 m
- 17 m
- 15 m
- 12 m
- 10 m

This suggests that quadrature generation is not handled as a single generic configuration but is tuned using precomputed band profiles.

Architecturally, this is important because it means:

- the LO subsystem contains band-specific knowledge
- frequency generation and phase behavior are part of the firmware design, not only hardware
- calibration may differ by band

## 6.3 Frequency correction model

The firmware includes a crystal correction value in parts-per-billion. This means the architecture already acknowledges oscillator error and compensates it in software.

This is a strong architectural choice because it allows:

- post-assembly calibration
- compensation for crystal tolerance
- easier alignment without hardware modification

## 7. Firmware Architecture

## 7.1 Firmware responsibilities

The firmware acts as the control plane for the transceiver. It is responsible for:

- initializing hardware peripherals
- maintaining tuning state
- managing frequency bands and step sizes
- controlling the display
- reading user input
- storing and restoring persistent settings
- programming the SI5351 synthesizer

The firmware is not just a UI layer; it is the coordination layer that makes the analog hardware usable as a complete radio.

## 7.2 Main application layer

The main application code in `src/main.cpp` appears to manage:

- current VFO frequency
- current operating mode
- allowed tuning range
- step sizes
- default frequencies per band
- current band index
- dirty-state tracking for persistent settings

This is effectively the system state model of the transceiver.

## 7.3 Library-based modularity

The firmware already uses separate libraries for major functions:

### DisplayUI

Responsibilities:

- initialize and access the ILI9341 display
- configure orientation and display settings
- render splash and main screens
- update frequency and mode display fields

This module is the presentation layer.

### RotaryInput

Responsibilities:

- initialize the rotary encoder interface
- decode Gray-code transitions
- detect button presses
- control tuning step size

This module is the primary tuning input layer.

### PushButtons

Responsibilities:

- read additional front-panel buttons
- support mode, band, and step selection
- provide button events to the main application layer

### SI5351Control

Responsibilities:

- initialize clock generation
- apply band-specific profiles
- set VFO frequency
- manage output enable state
- report synthesizer status

This module is the frequency-control layer.

## 7.4 Persistent settings

The main firmware includes flash-backed settings with a magic value and version field. Architecturally, this means the transceiver preserves user state across power cycles.

Likely persistent state includes:

- selected band
- per-band frequencies
- current tuning configuration
- other user preferences as the firmware evolves

This is a good fit for a radio UI, since users expect frequency memories and last-used settings to be retained.

## 8. User Interface Architecture

## 8.1 Physical controls

The interface includes:

- a rotary encoder with push button
- three additional push buttons
- a TFT display

This creates a compact front-panel interaction model suitable for a small transceiver.

## 8.2 Display model

The UI library currently supports:

- splash screen
- main operating screen
- frequency display updates
- mode display
- status bar regions
- placeholder S-meter area

This suggests a screen architecture based on a single main operating view rather than a menu-heavy system.

## 8.3 UI state and operating model

The present interaction model appears to be:

- rotary encoder changes VFO frequency
- button actions change step, mode, or band
- screen redraws reflect current operating state

A future evolution could formalize this into an explicit UI state machine, but the current architecture is suitable for a single-purpose front panel.

## 9. Operating Modes and Band Model

The current mode model includes:

- LSB
- USB
- CW
- FT8
- WSPR

This is an interesting architectural choice because it separates **display/operating intent** from pure RF hardware details. It suggests the firmware is intended to support multiple operating styles, even if some modes share the same underlying analog path.

The band model currently includes amateur band defaults and remembers a working frequency per band. This is a practical radio control feature and should remain a first-class concept in the architecture.

## 10. Data and Control Flow

## 10.1 Control flow from user input to frequency output

A typical tuning flow is:

1. user rotates encoder
2. rotary module produces a step delta
3. main application applies the selected tuning step
4. VFO frequency is updated
5. synthesizer is reprogrammed
6. display is updated to reflect the new frequency and mode

This path is the central interaction loop of the system.

## 10.2 Control flow for band/mode changes

A typical band or mode change flow is:

1. user presses a control button
2. main application updates current mode or band index
3. band default or stored band frequency is restored
4. synthesizer is reconfigured if needed
5. display is refreshed

## 10.3 Boot flow

A probable boot sequence is:

1. initialize MCU runtime
2. initialize display and draw splash screen
3. initialize input devices
4. load persistent settings from flash
5. initialize SI5351 with configured correction
6. apply default or restored VFO frequency and mode
7. draw main screen

## 11. Calibration Architecture

Calibration is not an optional add-on for this design; it is part of the architecture.

Important calibration concerns include:

- SI5351 absolute frequency correction
- quadrature phase accuracy
- gain balance between I/Q or phasing paths
- sideband suppression verification
- per-band behavior

The existing crystal correction approach shows that firmware-assisted calibration is already part of the design philosophy.

A dedicated calibration procedure document should exist alongside this architecture.

## 12. Design Constraints and Tradeoffs

The design appears to balance several constraints:

### Low-cost and availability
Using Raspberry Pi Pico, SI5351, and common display modules keeps the platform accessible.

### Simplicity vs. performance
A phasing architecture can reduce reliance on IF filters but introduces sensitivity to analog balance and calibration.

### Modular firmware
The firmware is organized cleanly into libraries, which supports iterative improvement.

### Mixed-domain complexity
The project spans analog RF design, phasing concepts, digital control, PCB layout, and UI, which means architectural clarity is especially important.

## 13. Known Architectural Gaps

Based on the current repository, these areas need clearer design documentation:

- explicit receive signal path description
- whether transmit is already implemented in hardware or only planned
- exact relationship between quadrature synthesis and analog phasing network
- power architecture and regulator scheme
- front-panel control mapping
- calibration workflow by band
- test strategy for RF, LO, and audio subsystems

These gaps do not necessarily reflect design problems, but they do reduce maintainability and onboarding ease.

## 14. Recommended Canonical Architecture Organization

To make the architecture easier to maintain, the repository should treat the following as canonical:

### Hardware source of truth
- current KiCad 8 project files
- hierarchical schematics
- current PCB layout
- local symbol and footprint libraries

### Firmware source of truth
- `platformio.ini`
- `src/`
- `lib/`
- `include/`
- `test/`

### Generated outputs
- Gerbers
- PDFs
- DRC reports

### Archived artifacts
- old KiCad legacy-format files
- autosaves
- backups
- copied project files
- cache files

## 15. Recommended Next Documentation

This architecture file should be accompanied by:

- `docs/hardware-overview.md`
- `docs/firmware-overview.md`
- `docs/bringup.md`
- `docs/calibration.md`
- `docs/pinout.md`
- `docs/block-diagram.md`

## 16. Summary

QRPTransceiver is a mixed hardware/firmware transceiver project built around a phasing-based HF architecture. Its defining characteristics are:

- analog phasing signal processing
- SI5351-based quadrature-oriented local oscillator control
- Raspberry Pi Pico-based digital coordination
- modular firmware for UI, tuning input, and synthesizer control
- integrated KiCad board and schematic design files

The project already has a solid modular foundation in both hardware hierarchy and firmware library structure. The main architectural need now is not a redesign of the system itself, but clearer documentation, stronger separation of canonical files from generated or legacy artifacts, and explicit documentation of signal flow, calibration, and subsystem boundaries.
