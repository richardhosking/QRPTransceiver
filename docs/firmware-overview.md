# QRPTransceiver Firmware Overview

## 1. Scope

This document describes the structure and responsibilities of the QRPTransceiver firmware. The firmware runs on a Raspberry Pi Pico and provides the control layer for tuning, user interaction, display output, and SI5351 frequency generation.

## 2. Firmware Role in the System

The firmware is the operational control plane of the transceiver. It is responsible for making the hardware usable as a radio by coordinating:

- VFO tuning
- band selection
- operating mode selection
- synthesizer configuration
- user input handling
- front-panel display output
- persistent settings storage

Without the firmware, the hardware design is a set of subsystems; with the firmware, those subsystems become an interactive transceiver platform.

## 3. Firmware Technology Stack

The repository indicates a firmware stack based on:

- Raspberry Pi Pico
- PlatformIO
- Arduino-compatible framework
- external device libraries for the display and SI5351

The code is structured in a modular embedded style rather than as a single monolithic application file.

## 4. Repository Layout

The firmware-related repository structure currently includes:

- `src/` — application entry point and top-level behavior
- `lib/` — modular libraries for display, rotary input, buttons, and synthesizer control
- `include/` — shared headers
- `test/` — tests
- `scripts/` — helper scripts
- `platformio.ini` — firmware build and configuration entry point

This is a good foundation for maintainable firmware development.

## 5. Main Responsibilities

The firmware currently appears to implement these major responsibilities:

### 5.1 System initialization
- initialize runtime and peripherals
- initialize display
- initialize input devices
- initialize SI5351 synthesizer
- restore or initialize operating state

### 5.2 Frequency control
- maintain the current VFO frequency
- apply minimum and maximum tuning limits
- support different tuning step sizes
- remember working frequencies for each band

### 5.3 Mode and band control
- track current operating mode
- change band defaults or stored band frequencies
- update display and synthesizer state after control changes

### 5.4 User interface control
- render splash and main screens
- update frequency and mode display fields
- reflect transceiver state visually

### 5.5 Persistent settings
- save settings to flash
- restore settings at boot
- track dirty state and versioning

## 6. Main Application Architecture

## 6.1 `src/main.cpp`

The main application layer acts as the system coordinator. It appears to hold or manage:

- current VFO frequency
- current mode
- valid tuning range
- tuning step table
- band default frequencies
- currently selected band
- settings persistence metadata

This file is the central state-management and orchestration layer of the firmware.

## 6.2 Application state model

The firmware already has the beginnings of a coherent state model. Important state elements include:

- current frequency
- current mode
- current band
- selected tuning step
- per-band stored frequencies
- settings dirty flag

This state should be treated as the canonical runtime model for the transceiver.

## 7. Modular Library Architecture

The firmware is split into reusable libraries under `lib/`. This is one of the strongest parts of the current design.

## 7.1 DisplayUI

The `DisplayUI` module is responsible for the front-panel display layer.

Current responsibilities include:

- providing access to the display instance
- display initialization
- reading display probe data
- UI configuration
- drawing splash and main screens
- updating the visible frequency and mode

Architecturally, this module should remain focused on presentation rather than business logic.

### Recommended boundary
`DisplayUI` should:
- draw UI state
- format visual elements
- own screen layout details

`DisplayUI` should not:
- decide operating policy
- own VFO truth
- perform synthesizer logic

## 7.2 RotaryInput

The `RotaryInput` module handles tuning encoder behavior.

Current responsibilities include:

- GPIO setup for encoder pins
- Gray-code decoding
- directional step detection
- encoder button edge detection
- tuning step value management

Architecturally, this module is the low-level physical input decoder for the main tuning control.

### Recommended boundary
`RotaryInput` should:
- read hardware state
- return delta steps and button events

`RotaryInput` should not:
- directly change frequency
- directly update the display
- contain band or mode logic

## 7.3 PushButtons

The `PushButtons` module is intended to handle the additional front-panel buttons.

Expected responsibilities include:

- reading non-encoder button states
- debouncing or edge detection
- converting button actions into application-level events

This module should be the companion input layer to the rotary encoder.

## 7.4 SI5351Control

The `SI5351Control` module is responsible for the programmable oscillator subsystem.

Current responsibilities include:

- initializing the SI5351
- accepting configuration such as crystal frequency and correction
- setting VFO frequency
- configuring quadrature-related output behavior
- applying band-specific profiles
- enabling or disabling outputs
- reporting readiness and current VFO value

This module is the firmware's abstraction over the LO hardware.

### Recommended boundary
`SI5351Control` should:
- own synthesizer device behavior
- translate desired frequency state into SI5351 operations
- contain band profile and correction logic

`SI5351Control` should not:
- own screen rendering
- own button interpretation
- decide UI workflow

## 8. Operating Model

## 8.1 Tuning workflow

The normal tuning loop is expected to be:

1. read encoder movement
2. convert movement into signed tuning steps
3. scale by selected tuning resolution
4. clamp to valid range
5. update VFO state
6. program SI5351 to new frequency
7. refresh display

This is the highest-frequency control loop in the application.

## 8.2 Band-change workflow

The normal band-change flow is expected to be:

1. detect button event
2. increment or select band
3. restore stored or default frequency for that band
4. apply synthesizer configuration changes
5. update visible UI fields

## 8.3 Mode-change workflow

The normal mode-change flow is expected to be:

1. detect button event
2. update mode state
3. refresh any derived behavior or display fields
4. keep the mode visible on the main screen

## 9. Frequency and Band Model

The firmware includes:

- a valid tuning range from low HF through 6 m
- a step table with multiple resolutions
- default frequencies for multiple amateur bands
- retained per-band frequency state

This is a sound operating model for a compact radio because it allows:

- quick tuning changes
- predictable band recall
- user-friendly control behavior

## 10. Persistent Settings Architecture

The firmware includes flash-backed settings identified by:

- a magic value
- a version field

This is the right foundation for reliable on-device persistence.

A robust persisted settings model should eventually include:

- current band
- current frequency or per-band frequencies
- tuning step selection
- selected mode
- calibration/configuration values if appropriate

Versioning is important so settings can evolve safely as firmware changes.

## 11. UI Architecture

## 11.1 Current UI model

The current display model appears to center on a single primary operating screen with:

- frequency display
- mode area
- band area
- status bar
- S-meter placeholder

This is well suited to a compact hardware front panel.

## 11.2 Recommended UI layering

The UI architecture will remain easier to maintain if it is split conceptually into:

### View model
A simple representation of what should be shown:
- frequency text
- mode text
- band text
- status fields

### Rendering layer
The actual display drawing code in `DisplayUI`

### Application controller
Main logic that decides when the view model changes

This reduces coupling between control logic and drawing logic.

## 12. Error Handling and Robustness

Firmware for a radio controller should handle at least these fault conditions gracefully:

- SI5351 initialization failure
- display initialization or communication problems
- invalid or corrupt flash settings
- out-of-range frequency requests
- input bounce or noisy encoder transitions

The current modular architecture makes it practical to improve robustness incrementally.

## 13. Recommended Future Refactoring Directions

The current structure is already solid, but future improvements could include:

### 13.1 Explicit application state struct
Move global state into a structured object such as:
- VFO
- band
- mode
- step
- persistent settings metadata

### 13.2 Event-driven input handling
Represent button and encoder actions as explicit events rather than direct inline mutations.

### 13.3 UI view model
Create a lightweight UI data model that the renderer consumes.

### 13.4 Hardware abstraction consistency
Ensure all hardware-facing code stays inside dedicated modules rather than leaking into `main.cpp`.

### 13.5 Centralized calibration/config structure
Store synthesizer correction, band offsets, and similar tunable values in a dedicated configuration layer.

## 14. Testing Opportunities

The presence of a `test/` directory means the project can grow into more systematic validation. Useful firmware tests would include:

- step-size logic
- frequency clamping
- band-memory behavior
- mode cycling behavior
- settings serialization/deserialization
- band profile selection logic
- frequency formatting for display

Even if hardware access remains manual, core logic can be validated independently.

## 15. Recommended Firmware Documentation Additions

In addition to this overview, useful companion documents would be:

- `docs/pinout.md`
- `docs/bringup.md`
- `docs/calibration.md`
- `docs/ui-behavior.md`
- `docs/settings-format.md`

## 16. Summary

The QRPTransceiver firmware is a modular embedded control system for a phasing-based HF transceiver. Its central role is to coordinate:

- tuning and band/mode state
- SI5351 frequency generation
- user input from encoder and buttons
- ILI9341 display output
- persistence of radio operating state

The current separation into `DisplayUI`, `RotaryInput`, `PushButtons`, and `SI5351Control` is a strong architectural base. The main next steps are clearer documentation, slightly more explicit state modeling, and continued separation between application logic, rendering, and hardware control.
