# Processor Power Section — Draft Schematic
Date: 2026-08-29

Purpose
- Draft schematic for the processor (RP2040) power section and power-latch circuit.
- Intended for review before creating a formal KiCad schematic sheet.

Overview
- VIN: external 5V (or other battery input) coming into board.
- V_SYS: regulated system rail (3.3V) powered through high-side PMOS latch Q1.
- POWER button: momentary push button used to latch power on/off via firmware.

Components
- Q1: P-channel MOSFET (high-side switch). Example: AO3407A or similar low-RdsON P-MOSFET.
- Q2: N-channel MOSFET (gate pull-down/latch driver). Example: 2N7002 or BSS138.
- R1: 1 MΩ pull-up from GATE_CTRL to VIN.
- R2: 1 kΩ series from button to GATE_CTRL (optional, for debouncing/noise suppression).
- R3: 100 kΩ pull-down on Q2 gate to GND.
- C1: 100 nF on GATE_CTRL to VIN for noise suppression (increase to 220 nF if needed).
- SW1: Momentary push button between GATE_CTRL and GND.
- D1: Schottky diode (optional) from V_SYS to VIN for ESD/protection.
- U1: LDO 3.3V regulator from V_SYS to VCC_3V3 (if separate regulation is used); alternatively V_SYS may be the LDO output.

Netlist sketch / connections
- VIN -> Q1 S (PMOS source)
- Q1 D -> V_SYS (PMOS drain to system rail/LDO input)
- Q1 G -> node GATE_CTRL
- R1 between GATE_CTRL and VIN (pull-up)
- C1 between GATE_CTRL and VIN (bypass)
- SW1 between GATE_CTRL and GND (momentary)
- R2 between SW1 and GATE_CTRL if series resistor used
- Q2 D -> GATE_CTRL
- Q2 S -> GND
- Q2 G -> MCU `POWER_HOLD` GPIO (through optional series resistor 1k)
- R3 between Q2 G and GND (pull-down)
- MCU `POWER_BUTTON` GPIO -> SW1 sense (through button to GND)
- Optionally place a small RC on the MCU input pin for debounce (10nF + 100k)

Suggested values and footprint notes
- PMOS: SOT-23 footprint, low Rds(on) <50 mΩ at chosen gate voltage
- NMOS: SOT-23 small-signal FET
- LDO: SOT-223 or SOT-23 depending on thermal requirements, 150–500 mA rating
- Use 0603 passive footprints for R/C

Safety and bring-up notes
- Before first power-up, verify PMOS orientation and that source is tied to VIN.
- Add a current-limited bench supply when testing; start at 100 mA limit.
- Place electrolytic/tantalum bulk cap (10–47 µF) on V_SYS near the regulator.
- Consider adding TVS or transient suppression on VIN if used in harsh environments.

Next steps
- Convert this draft to a proper KiCad schematic sheet with exact footprints.
- Add connector/power input filtering (fuse, polarity protection) as required by your enclosure.
- If you want, I can generate a KiCad schematic (.sch) draft or a PCB component placement recommendation.
