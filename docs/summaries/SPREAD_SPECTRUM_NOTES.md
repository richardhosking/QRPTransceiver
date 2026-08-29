 # Si5351 Spread-Spectrum (SSC) Notes

Last edited: 29 August 2026

Overview
--------
These notes collect the investigation, observed behaviour, and recommended
actions related to spread-spectrum clocking (SSC) on Si5351 modules encountered
during development of the QRPTransceiver firmware.

Background
----------
- Several Si5351 modules in the field and in development were found to power
  up with non-zero SSC parameter registers. The Etherkit `Si5351` library used
  by this project does not expose or manage SSC parameters.
- When active, SSC can impart deterministic spread or sidebands around the
  output clock that appeared as fixed-offset spurs in the receiver spectrum.

What we observed
----------------
- Fixed-offset sidebands (tens of kHz in these modules) that tracked with
  certain tuning operations and output drive transitions.
- After clearing all SSC parameter registers (registers `149..161`) the
  prominent sidebands disappeared and tuning noise characteristics improved.

What we changed (current firmware)
----------------------------------
- On init, the firmware explicitly writes `0x00` to `SI5351_SSC_PARAM0`..`SI5351_SSC_PARAM12`.
  This ensures SSC is effectively disabled for modules that power up with SSC
  enabled. The change is in `lib/SI5351Control/src/SI5351Control.cpp`.

Pros and cons of the current approach
-------------------------------------
- Pros:
  - Simple and reliable: clears any previously-enabled SSC quickly.
  - Eliminated the fixed sidebands observed during testing.

- Cons / Risks:
  - It's a blunt instrument: zeroing all SSC parameters removes any
    intentionally configured SSC behaviour and may hide vendor-specific
    calibration values.
  - Different module vendors may implement SSC-related behaviours slightly
    differently; overwriting all SSC parameters is not guaranteed to be
    appropriate for every board.

Recommended alternatives
------------------------
1. Selective disable
   - Prefer reading the SSC parameter registers at init and selectively
     clearing only the SSC enable/control bits per the SiLabs AN619 description
     if available. This preserves other SSC tuning parameters while disabling
     the spreading behaviour.

2. Preserve & log
   - Read SSC parameter registers on first boot, stash the original values in
     flash or in the serial log, then clear only the enable fields. This gives
     a reproducible rollback path for debugging.

3. External reference
   - If absolute spectral purity is a priority for production units, consider
     switching to an external reference (CLKIN) or using a higher-grade XO or
     TCXO; then set the Si5351 ref/correction appropriately.

Helpful engineering checks
-------------------------
- Read current SSC registers (example pseudo):

  ```cpp
  for (uint8_t r = SI5351_SSC_PARAM0; r <= SI5351_SSC_PARAM12; ++r) {
    Serial.print(r); Serial.print(": ");
    Serial.println(si5351_read(r), HEX);
  }
  ```

- If you see non-zero values, consider logging them before clearing.

Where to from here
-------------------
- I can implement any of the alternatives above:
  - selective disable (read/modify/write of specific bits),
  - persistent backup of SSC values to flash before clearing,
  - or a small serial calibration tool to read/write registers interactively.

If you'd like, tell me which option you prefer and I will implement it and
open a PR with the changes.

References
----------
- Silicon Labs AN619 application note (Si5351 programming guidance)
- Etherkit Si5351 library (no SSC management exposed)
SPREAD SPECTRUM NOTES
=====================

Summary
-------
Some SI5351 modules come from different vendors with spread-spectrum (SSC) features
enabled by default. This can add AM-like sidebands (observed at ±31.5 kHz) on the LO.

Mitigation
----------
- The firmware explicitly clears the SI5351 SSC parameter registers at init to ensure
  spread-spectrum is disabled on modules that power up with it enabled. The register
  range cleared is `SI5351_SSC_PARAM0` .. `SI5351_SSC_PARAM12` (addresses 149..161).
- This is a pragmatic fix; clearing SSC changes the synthesizer behaviour and will
  alter absolute-frequency calibration. The firmware therefore exposes a trim
  (`SI5351_CORRECTION_TRIM_PPB`) that can be adjusted per-board to restore
  absolute frequency accuracy after SSC is disabled.

Open Questions / Follow-ups
---------------------------
- Clearing the full parameter block is aggressive — a cleaner approach would be to
  read-modify-write only the SSC enable bit if the device supports it. This would
  preserve other SSC parameters while ensuring SSC is disabled.
- Consider adding an automated calibration routine to measure and persist the
  `SI5351_CORRECTION_TRIM_PPB` per board to account for module-to-module variation
  and temperature drift.

References
----------
- Etherkit Si5351 headers define the SSC registers but the library README marks
  spread-spectrum as "unsupported". See `lib/SI5351Control/src/SI5351Control.cpp`
  for the current zeroing implementation in `beginDevice()`.
# SI5351 Spread-Spectrum (SSC) Notes

Purpose
-------
Document the root cause analysis, evidence and practical options related to
spread-spectrum clocking (SSC) observed on some Si5351 modules during
development of the QRPTransceiver V3 firmware.

Symptoms observed
-----------------
- Fixed-offset sidebands and spurs (e.g. ~31 kHz offsets) appearing on outputs
  when certain tuning strategies or output power transitions occurred.
- Problem more visible with higher output drive and when using the direct
  phase-register quadrature tuning method on higher bands.

Root cause
----------
- Some third-party Si5351 modules power up with non-zero SSC parameter
  registers; the upstream Etherkit library does not manage SSC state.
- Active SSC + frequent multisynth/PLL changes produced measurable spurs and
  changed apparent tuning/phase behaviour.

Workaround applied
------------------
1. Firmware explicitly clears SSC parameter registers on init:

   - Registers `SI5351_SSC_PARAM0` .. `SI5351_SSC_PARAM12` are zeroed during
     `beginDevice()` so modules that power with SSC enabled are forced to a
     non-SSC state.

2. Tuning strategy changed to a fixed-divider + PLL retune method for all
   bands, which reduced interaction with any residual SSC state and lowered
   audible step noise.

Risks and trade-offs
--------------------
- Zeroing all SSC params is an aggressive action. It guarantees SSC is off but
  also discards any intentional SSC configuration some hardware might rely on.
- Alternative: toggle only the SSC enable bit if future testing shows a
  reliable documented bit for enabling/disabling SSC across vendor modules.

Recommended future actions
-------------------------
1. If module compatibility is required across many part vendors, prefer toggling
   only the SSC enable bit (if present/documented) rather than zeroing all SSC
   parameters. This requires additional testing to find the correct control
   register/bit across modules.

2. Add a diagnostic readback option during init to log SSC parameter values to
   serial for failure analysis; helpful when field reproducers report spurs.

3. For production-grade frequency accuracy, use an external 10 MHz reference or
   temperature-stabilized crystal and configure the Si5351 to use `CLKIN` via
   `set_ref_freq()` and `set_pll_input()`.

4. Document the SSC-clear behaviour in `CURRENT_SI5351_APPROACH.md` and in the
   build/production notes so maintainers are aware of the side-effect.

Files & locations
-----------------
- Implementation: `lib/SI5351Control/src/SI5351Control.cpp` (SSC clear loop)
- Summary reference: `docs/summaries/QUADRATURE_PHASE_CORRECTION_SUMMARY.txt`

End of notes
