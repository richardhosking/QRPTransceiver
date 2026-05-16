# QRPTransceiver Hardware Overview

## 1. Scope

This document describes the hardware organization of the QRPTransceiver project at the repository and subsystem level. It is intended as a guide to how the hardware design is partitioned, what the major blocks are, and how to navigate the KiCad sources.

## 2. Hardware Design Artifacts

The hardware design currently includes:

- KiCad project file
- top-level schematic
- PCB layout
- hierarchical schematic sheets
- local footprint and symbol libraries
- Gerber outputs
- PDF exports
- DRC and related reports
- datasheet references

These files collectively define the electrical design, physical PCB implementation, and manufacturing outputs.

## 3. Primary Hardware Source Files

The main editable hardware sources should be treated as:

- `QRPTransceiver.kicad_pro`
- `QRPTransceiver.kicad_sch`
- `QRPTransceiver.kicad_pcb`

These should be considered the primary design source of truth.

Additional subsystem schematics currently include:

- `ReceiverAudio.kicad_sch`
- `polyphase.kicad_sch`

The top-level schematic also references additional hierarchical sheet structure for mixer-related circuitry.

## 4. Design Philosophy

The hardware appears to follow a phasing-based HF transceiver approach. This implies a design philosophy built around:

- quadrature signal generation or processing
- analog amplitude and phase balance
- modular RF/baseband partitioning
- digital assistance for tuning and control
- practical PCB implementation using surface-mount components

Compared with a more traditional IF-filter-centered transceiver, this architecture places more importance on phase accuracy and subsystem matching.

## 5. Hardware Partitioning

The design can be understood as a set of major hardware blocks.

## 5.1 RF front end

The RF front end is expected to include:

- antenna-side signal entry
- input filtering or band-dependent response
- routing into the mixer/phasing path
- any switching required between operating conditions

This is the section most exposed to external RF conditions and should be treated as a sensitive analog block.

## 5.2 Mixer section

The mixer section is a central transceiver block. Its role is expected to include:

- translation between RF and lower-frequency signal domains
- interaction with quadrature local oscillator signals
- support for sideband-selective behavior through phasing techniques

This section is tightly coupled to the quality of the LO generation and the analog balance of the signal path.

## 5.3 Polyphase section

The polyphase section is one of the defining features of the design. It is expected to:

- establish controlled phase relationships between signal paths
- contribute to image or sideband suppression
- shape the frequency-dependent phase behavior required by the phasing architecture

This block is likely one of the most performance-sensitive parts of the analog design.

## 5.4 Receiver audio section

The receiver audio section is expected to:

- process recovered low-frequency or audio-band signals
- apply filtering and gain
- provide output suitable for listening, measurement, or further conditioning

This block is a key bring-up checkpoint because it allows observation of downconverted receive-path behavior.

## 5.5 Control and digital interface section

The design also includes a digital control domain built around a Raspberry Pi Pico and peripheral modules. This section likely includes:

- MCU connections
- SI5351 control interface
- display connections
- rotary encoder input
- push-button input
- power and reset support for digital peripherals

Although digital, this section directly influences RF usability through frequency control and operating-state selection.

## 6. Frequency Generation Hardware

## 6.1 SI5351 role

The SI5351 is the central programmable oscillator element in the design. Its hardware role is to provide the clock or LO signals required by the transceiver.

In a phasing design, this is especially important because the quality and phase relationship of generated signals directly affect sideband suppression and tuning usability.

## 6.2 Quadrature implications

The firmware organization suggests the hardware depends on band-specific quadrature behavior. That implies the hardware and firmware together form a combined frequency-generation subsystem rather than fully independent analog and digital sections.

This combined subsystem should be treated as a unified design concern during:
- bring-up
- calibration
- troubleshooting
- future revisions

## 7. PCB-Level Considerations

## 7.1 Physical separation of domains

For best performance, the board should maintain clear physical separation between:

- RF analog paths
- audio/baseband analog paths
- clock/LO routing
- digital control and display wiring
- power conversion or regulation areas

This reduces the risk of digital noise degrading analog performance.

## 7.2 Grounding and return paths

Because the project combines analog RF and digital control on one board, grounding strategy is likely to be a key factor in performance. Review areas should include:

- return current paths
- analog/digital interaction zones
- display and SPI noise coupling
- oscillator signal reference quality
- audio-stage grounding

## 7.3 Routing priorities

Routing priorities should generally favor:

- short and controlled LO paths
- clean sensitive analog routing
- separation of switching digital lines from high-gain analog stages
- careful placement of decoupling components
- consistent grounding around the mixer and polyphase networks

## 8. Hardware Interfaces

The major hardware interfaces in the project are expected to include:

### RF interfaces
- antenna and RF signal path connections
- any band-related or signal-routing interconnects

### Audio interfaces
- receiver audio output
- internal analog handoff points between mixer/polyphase/audio sections

### Digital control interfaces
- I2C or similar control connection to SI5351
- SPI connection to TFT display
- GPIO for encoder and buttons

### Power interfaces
- primary supply input
- regulated rails for analog and digital sections
- display backlight and digital peripheral supply distribution

## 9. Bring-Up Strategy by Hardware Block

A practical hardware validation order is:

1. **power rails**
   - confirm supply integrity and regulator outputs

2. **MCU and control section**
   - verify Pico boots and firmware runs

3. **display and inputs**
   - verify user interface hardware functions

4. **SI5351 / LO section**
   - verify oscillator programming and output frequency

5. **receiver audio path**
   - confirm analog audio stages are alive and stable

6. **mixer and quadrature network**
   - verify expected translation and phase-related behavior

7. **full receive chain**
   - validate end-to-end tuning and signal reception

This staged bring-up reduces ambiguity during troubleshooting.

## 10. Hardware Risks and Sensitivities

The most likely architecture-level hardware sensitivities are:

- quadrature phase accuracy
- gain or component tolerance mismatch across phasing paths
- oscillator frequency error
- coupling from digital display/control lines into analog sections
- grounding/layout effects on sideband suppression
- variation in performance across bands

These are normal concerns for a phasing-based mixed-signal transceiver and should be explicitly measured during testing.

## 11. Recommended Hardware Documentation Additions

The hardware design would benefit from dedicated supporting documents such as:

- `docs/block-diagram.md`
- `docs/pinout.md`
- `docs/power.md`
- `docs/calibration.md`
- `docs/bringup.md`
- `docs/layout-notes.md`

In addition, the top-level schematic hierarchy should use clearly descriptive sheet names so the functional structure is visible directly inside KiCad.

## 12. Recommended Canonical Hardware Organization

As the repo evolves, the hardware should ideally be organized so that:

- active KiCad 8 files are grouped together
- generated outputs are separated from source design files
- local libraries are clearly grouped
- archived backups and legacy KiCad files are moved out of the active design path
- reports and exports are treated as generated artifacts rather than editable design source

A future structure could look like:

```text
hardware/
  kicad/
    QRPTransceiver.kicad_pro
    QRPTransceiver.kicad_sch
    QRPTransceiver.kicad_pcb
    sheets/
    libraries/
  outputs/
    gerbers/
    pdf/
    drc/
reference/
  datasheets/
archive/
  legacy-kicad/
  backups/
```

## 13. Summary

The QRPTransceiver hardware is organized around a phasing-based HF transceiver architecture with a strong division between:

- mixer and phasing circuitry
- receiver audio processing
- programmable LO generation
- digital control and user interface

Its success depends not only on correct schematic connectivity, but also on careful quadrature behavior, calibration, and PCB partitioning between analog and digital domains.
