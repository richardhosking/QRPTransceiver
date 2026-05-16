# QRPTransceiver Calibration Guide

## 1. Purpose

This document defines a practical calibration approach for QRPTransceiver. Because this is a phasing-based HF transceiver, calibration is a core part of the design rather than an optional finishing step.

The main goals are to:

- align oscillator frequency accurately
- verify band behavior
- optimize quadrature-related performance
- improve sideband suppression
- document repeatable adjustment procedures

## 2. Why Calibration Matters in This Design

Phasing architectures depend strongly on balance between signal paths. Small errors in phase, amplitude, or oscillator accuracy can cause:

- frequency offset
- weak or distorted reception
- poor image rejection
- poor unwanted-sideband suppression
- variation in performance across bands

For that reason, calibration should be done methodically and recorded.

## 3. Calibration Categories

The calibration effort can be divided into these categories:

1. **frequency calibration**
2. **band alignment verification**
3. **quadrature / phase verification**
4. **gain balance verification**
5. **sideband suppression optimization**
6. **operating verification across bands**

## 4. Required or Recommended Equipment

Recommended equipment:

- stable frequency reference or calibrated receiver
- oscilloscope
- frequency counter if available
- RF signal source if available
- audio monitoring equipment
- notebook or calibration log

Helpful but optional:

- two-channel oscilloscope for phase comparison
- spectrum analyzer
- SDR for observing unwanted responses

## 5. Firmware-Supported Frequency Calibration

The firmware currently includes an SI5351 correction value in parts-per-billion. This provides a software-level method to trim the output frequency without modifying hardware.

This should be treated as the primary correction mechanism for absolute LO error.

## 6. Frequency Calibration Procedure

## 6.1 Objective

Adjust the SI5351 correction value so the actual generated frequency matches the intended VFO frequency as closely as practical.

## 6.2 Method

1. boot the transceiver with current firmware
2. select a known VFO frequency
3. measure the SI5351 output using a frequency counter, oscilloscope, or calibrated receiver method
4. calculate the observed frequency error
5. update the firmware correction value
6. rebuild and reflash firmware
7. repeat until acceptable accuracy is reached

## 6.3 Notes

- perform the measurement after the board has reached a stable temperature
- use a frequency in a region that is convenient to measure accurately
- verify that the correction improves agreement at more than one band if possible

## 7. Band Verification Procedure

Once the main frequency correction is acceptable, verify behavior across all supported bands.

Current firmware band coverage includes:

- 160 m
- 80 m
- 40 m
- 30 m
- 20 m
- 17 m
- 15 m
- 12 m
- 10 m
- 6 m as a default-frequency target in the main application band table

For each band:

1. switch to the band
2. observe the default or recalled operating frequency
3. verify tuning changes are reflected correctly
4. verify oscillator output remains stable
5. compare displayed frequency to measured or known-correct frequency

Record any systematic band-specific offset or instability.

## 8. Quadrature / Phase Verification

## 8.1 Objective

Verify that the quadrature-related LO behavior and analog phasing paths are performing consistently enough for useful sideband suppression.

## 8.2 Method

Where measurement access is practical:

- compare relevant phase-related nodes using a dual-channel oscilloscope
- verify expected phase relationship between corresponding paths
- compare amplitude balance across paired paths
- repeat on multiple bands, especially low, mid, and high HF ranges

## 8.3 What to Watch For

Potential indicators of trouble include:

- significant amplitude mismatch between paired channels
- phase offset that varies strongly by band
- one path missing or heavily attenuated
- unstable waveform quality or clipping

## 9. Gain Balance Verification

In a phasing receiver or transceiver, gain mismatch can degrade unwanted-sideband rejection almost as much as phase mismatch.

Suggested procedure:

1. inject a consistent test signal
2. measure corresponding levels in the relevant paired paths
3. compare amplitudes through the receive chain
4. identify stages where mismatch appears

Possible correction methods depend on the hardware implementation and may involve:

- component tolerance selection
- resistor/capacitor adjustment
- improved layout or grounding in future revisions

## 10. Sideband Suppression Optimization

## 10.1 Objective

Maximize rejection of the unwanted sideband or image response.

## 10.2 Practical approach

1. tune to a known test condition or injected signal
2. observe desired response and unwanted response
3. compare behavior across bands
4. identify whether the dominant limitation appears to be:
   - frequency error
   - phase mismatch
   - gain mismatch
   - layout coupling
   - firmware frequency setup

## 10.3 Important note

Do not attempt to solve all sideband suppression problems in firmware. Some limits will come from analog tolerances, routing, or component matching.

## 11. Calibration Order

Recommended calibration order:

1. power and bring-up completion
2. oscillator frequency correction
3. band verification
4. phase/quadrature verification
5. gain-balance verification
6. sideband suppression checks
7. repeat spot checks on multiple bands

This order minimizes wasted effort and avoids tuning analog behavior around an incorrect frequency baseline.

## 12. Recording Calibration Data

A calibration log should capture at least:

- board revision
- date
- firmware commit
- SI5351 correction value used
- measured error before correction
- measured error after correction
- observations per band
- phase/gain mismatch observations
- sideband suppression observations
- any temporary or permanent hardware modifications

## 13. Acceptance Criteria

Exact targets may evolve, but a good practical definition of success is:

- displayed and generated frequency agree closely enough for normal operation
- tuning is stable across bands
- quadrature behavior is consistent and repeatable
- no major band is obviously degraded relative to the others without explanation
- unwanted sideband/image suppression is visibly improved after calibration

## 14. Future Improvements

Future calibration support could include:

- storing correction and calibration values in persistent settings
- band-specific correction values if needed
- a built-in service or diagnostics screen
- test modes for fixed-frequency output or calibration signal generation
- documented measurement points on the schematic/PCB silkscreen

## 15. Summary

Calibration for QRPTransceiver should focus on:

- accurate SI5351 frequency generation
- consistent band behavior
- good phase and gain balance
- improved sideband suppression

Because the project uses phasing techniques, repeatable calibration is essential to achieving good practical radio performance.
