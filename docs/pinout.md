# QRPTransceiver Pinout

## 1. Scope

This document summarizes the firmware-visible pin assignments currently used by QRPTransceiver. These values are derived from the current firmware source and should be treated as the active software pin map unless hardware wiring is intentionally changed.

## 2. Raspberry Pi Pico Pin Usage Summary

## 2.1 Rotary encoder

- GP2 — rotary encoder A
- GP3 — rotary encoder B
- GP6 — rotary encoder push button

## 2.2 Front-panel buttons

- GP7 — MODE button
- GP8 — BAND button
- GP9 — STEP button
- GP10 — FN/SAVE button

## 2.3 TFT display

- GP16 — TFT MISO
- GP17 — TFT CS
- GP18 — TFT SCK
- GP19 — TFT MOSI
- GP20 — TFT DC
- GP21 — TFT RST
- GP22 — TFT backlight

## 2.4 Built-in indicator

- LED_BUILTIN — firmware heartbeat indicator

## 3. Functional Grouping

### User input
- GP2, GP3, GP6
- GP7, GP8, GP9, GP10

### Display / SPI-related
- GP16, GP17, GP18, GP19, GP20, GP21, GP22

### System status
- LED_BUILTIN

## 4. Notes

## 4.1 Input assumptions

The current firmware configures:

- rotary encoder pins with pull-ups
- encoder button with pull-up
- push buttons as active-low inputs with pull-ups

This means external wiring should match an active-low switch arrangement unless the firmware configuration is changed.

## 4.2 Push-button defaults

The push-button library default configuration supports:

- MODE
- BAND
- STEP
- FN

The main application currently initializes FN on GP10.

## 4.3 Display assumptions

The display layer is written for an ILI9341-compatible TFT and uses the pin assignments listed above. The firmware also references display probe reads and a landscape orientation UI.

## 4.4 SI5351 interface

The current firmware clearly initializes the SI5351, but the specific GPIO pins used for its control bus are not explicitly documented in the currently reviewed source snippets. On Raspberry Pi Pico Arduino builds, these may rely on the framework's default I2C pin mapping unless overridden elsewhere.

That interface should be documented explicitly once confirmed in the hardware or complete firmware configuration.

## 5. Recommended Next Step

This file should eventually be expanded with:

- SI5351 bus pins
- power-related pins if relevant
- connector names
- schematic reference designators for major interfaces
- a table cross-referencing Pico pin numbers, GPIO names, and physical connector locations

## 6. Summary Table

| Function | GPIO |
|---|---|
| Encoder A | GP2 |
| Encoder B | GP3 |
| Encoder Button | GP6 |
| MODE Button | GP7 |
| BAND Button | GP8 |
| STEP Button | GP9 |
| FN/SAVE Button | GP10 |
| TFT MISO | GP16 |
| TFT CS | GP17 |
| TFT SCK | GP18 |
| TFT MOSI | GP19 |
| TFT DC | GP20 |
| TFT RST | GP21 |
| TFT Backlight | GP22 |
| Heartbeat LED | LED_BUILTIN |
