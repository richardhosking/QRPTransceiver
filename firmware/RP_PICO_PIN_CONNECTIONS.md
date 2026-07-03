# RP Pico pin connections

Pin connections inferred from the firmware source.

## Pinout table

| Function | RP Pico GPIO | RP Pico pin | Direction | Notes |
| Encoder A | GP2 | 4 | Input | Rotary encoder channel A |
| Encoder B | GP3 | 5 | Input | Rotary encoder channel B |
| I2C SDA | GP4 | 6 | Bidirectional | Shared bus for SI5351 and PCF8574 |
| I2C SCL | GP5 | 7 | Bidirectional | Shared bus for SI5351 and PCF8574 |
| Encoder button | GP6 | 9 | Input | Push switch on encoder |
| Mode button | GP7 | 10 | Input | Front panel button |
| Band button | GP8 | 11 | Input | Front panel button |
| Step button | GP9 | 12 | Input | Front panel button |
| FN / Save button | GP10 | 14 | Input | Front panel button |
| TX / RX button | GP11 | 15 | Input | Front panel button |
| RxMute | GP12 | 16 | Output | Receive mute output |
| SsbMute | GP13 | 17 | Output | SSB mute output |
| CwMute | GP14 | 18 | Output | CW mute output |
| Power button | GP15 | 19 | Input | Short press wake, long press soft power off |
| TFT MISO | GP16 | 21 | Input | Optional display readback |
| TFT CS | GP17 | 22 | Output | ILI9341 chip select |
| TFT SCK | GP18 | 24 | Output | SPI0 SCK |
| TFT MOSI | GP19 | 25 | Output | SPI0 TX |
| TFT DC | GP20 | 26 | Output | ILI9341 data/command |
| TFT RST | GP21 | 27 | Output | ILI9341 reset |
| TFT BL | GP22 | 29 | Output | Backlight control |
| S-meter | GP26 | 31 | Analog input | ADC input |
| Onboard LED | LED_BUILTIN | — | Output | Status indicator |

## Display: ILI9341 SPI TFT

| Signal | RP Pico GPIO | RP Pico pin | Notes |
|---|---:|---:|---|
| CS | GP17 | 22 | Chip select |
| DC | GP20 | 26 | Data/command |
| RST | GP21 | 27 | Reset |
| MOSI | GP19 | 25 | SPI0 TX |
| SCK | GP18 | 24 | SPI0 SCK |
| MISO | GP16 | 21 | SPI0 RX, optional readback |
| BL | GP22 | 29 | Backlight |

## Rotary encoder

| Signal | RP Pico GPIO | RP Pico pin | Notes |
|---|---:|---:|---|
| A | GP2 | 4 | Encoder channel A |
| B | GP3 | 5 | Encoder channel B |
| Button | GP6 | 9 | Encoder push button |

## Front panel push buttons

| Button | RP Pico GPIO | RP Pico pin | Notes |
|---|---:|---:|---|
| Mode | GP7 | 10 | Cycle mode |
| Band | GP8 | 11 | Cycle band |
| Step | GP9 | 12 | Cycle tuning step |
| FN / Save | GP10 | 14 | Save settings |
| TX / RX | GP11 | 15 | Toggle TX/RX |
| Power | GP15 | 19 | Short press wake, long press soft power off |

## Board control outputs and analog input

| Signal | RP Pico GPIO | RP Pico pin | Notes |
|---|---:|---:|---|
| RxMute | GP12 | 16 | Receive mute output |
| SsbMute | GP13 | 17 | SSB mute output |
| CwMute | GP14 | 18 | CW mute output |
| S-meter | GP26 | 31 | Analog input |

## SI5351 synthesizer

| Device | Interface | Notes |
|---|---|---|
| RX SI5351 | I2C address 0x60 | GP4 / GP5 | Uses the shared I2C bus |
| TX SI5351 | I2C address 0x61 | GP4 / GP5 | Uses the shared I2C bus |

## Band filter control

| Signal | RP Pico GPIO | Notes |
|---|---|---|
| Band filter select | I2C PCF8574 at 0x20 | GP4 / GP5 | GPIO pins not used when I2C mode is selected |

## Built-in

| Signal | RP Pico GPIO | Notes |
|---|---|---|
| Onboard LED | LED_BUILTIN | Status indicator |

If needed, I can also turn this into a compact wiring diagram or a single pinout table sorted by GPIO number.