# Controller Sheet — Processor Power Latch (Draft)
Date: 2026-08-29

This file is a draft representation of the processor power-latch circuit placed
into the project's `controller` KiCad sheet. It is intended to be used as the
source for creating a proper KiCad schematic sheet (`.kicad_sch`) by copying
the components and nets into your KiCad project.

See also: [hardware/kicad/drafts/POWER_CIRCUIT_SCHEMATIC_DRAFT.md](../kicad/drafts/POWER_CIRCUIT_SCHEMATIC_DRAFT.md)

SHEET: Controller / Power Latch

Symbols / Refdes
- Q1: PMOS_HIGH (AO3407A) — high-side P-channel MOSFET
- Q2: NMOS_LOW (2N7002) — gate pull-down / latch driver
- R1: R_PU_GATE (1M) — gate pull-up to VIN
- R2: R_BTN_SER (1k) — optional series resistor from button to gate
- R3: R_Q2_G_PD (100k) — pull-down on Q2 gate
- C1: C_GATE_BYP (100nF) — gate bypass cap to VIN
- SW1: SW_POWER (momentary push)
- D1: D1_SCHOTTKY (optional) — Schottky from V_SYS to VIN (ESD/protect)
- U1: LDO_3V3 (optional) — regulator from V_SYS to 3V3

Net connections (KiCad net names)
- VIN       : connector VIN
- V_SYS     : net V_SYS (PMOS drain / LDO input)
- GATE_CTRL : net GATE_CTRL (PMOS gate, Q2 drain, button node)
- GND       : ground
- MCU_PWR_HOLD : net POWER_HOLD (MCU GPIO to Q2 gate)

Netlist sketch (human readable)

  VIN -----+--------------------+------------------------------
           |                    |                              
           |                   R1 (1M)                         
           |                    |                              
          Q1[S]                 +-- GATE_CTRL -----------------+
        PMOS (Q1)               |                             |
           |                    |                            C1
  V_SYS <--+--------------------+                            100nF
                                                              |
                                                             VIN

  SW1 between GATE_CTRL and GND (momentary)
  R2 optional series between SW1 and GATE_CTRL

  Q2: NMOS
    D -> GATE_CTRL
    S -> GND
    G -> MCU_PWR_HOLD (via 1k series if desired)
    R3 -> between Q2 G and GND

Expected behavior (firmware integration)
- Short press SW1: latch on sequence — MCU asserts `POWER_HOLD` high.
- Long press SW1: firmware mutes, saves state, clears `POWER_HOLD` to shut down.

Placement notes for the controller sheet
- Place Q1 close to the VIN connector on the sheet so the power flow is clear.
- Put decoupling caps (10 µF + 0.1 µF) on V_SYS near the LDO and MCU VCC pins.
- Draw the `POWER_HOLD` net to the MCU symbol on the controller sheet and label it.

Footprint suggestions (to be chosen in KiCad)
- Q1: SOT-23 P-MOS footprint
- Q2: SOT-23 N-MOSFET footprint
- LDO: SOT-223 or SOT-23-5 based on thermal needs
- Passives: 0603

Conversion note
- This file is intentionally human-readable; to integrate into an existing KiCad
  project, create a new sheet named `controller_power` and copy the symbols,
  nets, and footprints into that sheet. I can generate a `.kicad_sch` file if
  you want a machine-importable sheet — tell me and I'll produce one.
