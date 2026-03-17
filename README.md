<p align="center">
  <img src="docs/spoolsense-logo.png" width="200" alt="SpoolSense">
</p>

# SpoolSense Scanner

## Overview
SpoolSense Scanner is an ESP32‑based NFC scanner designed for managing 3D printer filament spools using NFC tags. It is designed to integrate with the SpoolSense ecosystem and supports multiple NFC tag formats.

**OpenPrintTag (ISO15693)** is the primary supported format, with full read and write support including filament metadata, weight tracking, and Spoolman sync. UID‑only tags such as NTAG215 (ISO14443A) are also supported — the scanner detects the tag and publishes the UID, allowing middleware to look up the spool by ID.

I am also exploring support for **OpenTag3D** as an additional tag format in a future release.

The scanner allows users to tap a filament spool to identify it, retrieve metadata from the NFC tag, and trigger external automation or spool tracking workflows. A built-in web interface — accessible at `http://spoolsense.local` after connecting to WiFi — lets you write OpenPrintTag data directly to a tag from any browser on your local network, with no app or external tools required.

Configuration is set at compile time by editing `include/UserConfig.h` before flashing.

## How to configure
1. Copy the example config: `cp include/UserConfig.example.h include/UserConfig.h`
2. Edit `include/UserConfig.h` and fill in your settings:
   - WiFi SSID and password
   - MQTT broker host, port, and credentials
   - Spoolman URL (optional)
   - LCD and status LED enable flags
   - Automation mode (`0` = Self Directed, `1` = Controlled by HA)
   - Board selection (`BOARD_ESP32_WROOM` or `BOARD_ESP32_S3`)
3. Flash the firmware: `pio run -t upload`

## Supported Tag Formats

| Format | Protocol | Support |
|--------|----------|---------|
| OpenPrintTag | ISO15693 | Full read/write — CBOR/NDEF filament data, weight tracking, Spoolman sync |
| UID-only (NTAG215, etc.) | ISO14443A | UID detected and published as `GENERIC_TAG_DETECTED`; middleware looks up spool by UID |

OpenPrintTag tags must be written in OpenPrintTag format per the [OpenPrintTag specification](https://openprinttag.org/generator/).

## Functionality
* **NFC Tag Reading/Writing:** Reads and writes OpenPrintTag-formatted NFC tags.
* **Built-in Tag Writer UI *(early beta)*:** After connecting to WiFi, navigate to `http://spoolsense.local` from any browser on your local network to write OpenPrintTag data directly to a tag — no app required. Supports all fields including material type, manufacturer, weight, color, density, diameter, temperatures, and Spoolman ID.
* **Home Assistant Integration:** Publishes spool state via MQTT with full HA discovery support.
* **Spoolman Sync (optional):** Syncs spool weight and metadata with a Spoolman instance.
* **BLE Spool Operations:** Write tag data, set filament weight, and manage spools via the BLE web UI.
* **LCD Display (optional):** Displays device status, NFC scan results, and system information.
* **Extensible Architecture:** The firmware is designed so additional tag formats can be added over time.

# Hardware Setup


## Hardware Needed
*   NFC Reader/Writer: PN5180 NFC module (ISO 15693)
*   ESP32: ESP32-WROOM-32 (primary supported board — e.g. [ESP32 DevKitC V4](https://a.co/d/gW3zBIJ)). ESP32-S3 support is actively being tested on the ESP32-S3-Zero, with plans to add full support for smaller form-factor boards.
*   USB Cable: USB-A to USB-C (1)
*   Jumper wires: male-to-female Dupont wires (9)
*   LCD Screen: [16x2 I2C LCD](https://a.co/d/dryhwvd) (optional — full optional support is in progress)
*   Status LED: SK6812 RGBW (WS2812-compatible addressable LED, 1 pixel) (optional)

## Optional Status LED

The scanner firmware now supports an optional single‑pixel addressable status LED. This LED provides visual feedback for scanner state and spool/tag events.

**Recommended LED:**

* SK6812 RGBW (WS2812‑compatible addressable LED)

The firmware is configured for **SK6812 RGBW timing and color order** using:

`NEO_GRBW + NEO_KHZ800`

### LED Status States

| Event | LED Behavior |
|------|--------------|
| Booting | White (W channel) |
| WiFi connected | Cyan |
| Ready | Blue |
| Tag detected | 3 white flashes |
| Valid OpenPrintTag spool | Filament color (solid); breathing if ≤100g remaining |
| Generic/UID-only tag | 3 white flashes, then off |
| Blank/invalid tag | Red flash, then off |
| Tag removed | Color persists — stays at last scanned filament color |
| Write success | Green flash, then restores filament color |
| Write failure | Red flash, then restores filament color |

### Enabling the LED

The status LED feature is **optional** and is controlled via the build configuration in `platformio.ini`.

To enable the LED, add the following build flags:

```
build_flags =
    -DUSE_STATUS_LED=1
    -DSTATUS_LED_PIN=4
```

**Explanation:**

* `USE_STATUS_LED` enables compilation of the LED feature.
* `STATUS_LED_PIN` sets the ESP32 GPIO pin used for the LED data line.

You may change the pin to any suitable GPIO supported by your ESP32 board. For example:

```
build_flags =
    -DUSE_STATUS_LED=1
    -DSTATUS_LED_PIN=16
```

### Disabling the LED

If you do **not** have the optional status LED installed, simply omit the `USE_STATUS_LED` flag from `platformio.ini`.

Example with LED disabled:

```
build_flags =
```

When the flag is not present, the LED code is **not compiled**, and the firmware behaves exactly like the original scanner firmware with no LED functionality.

## Hardware Configuration
Connect the components to the ESP32 as follows:

**16x2 I2C LCD (I2C):**
*   **GND:** GND
*   **VCC:** 5V
*   **SDA:** GPIO 23
*   **SCL:** GPIO 22

**SK6812 RGBW Status LED:**

> A single SK6812 RGBW LED module is recommended. Many small breakout boards include the necessary capacitor and resistor already. If using a bare LED, a ~330Ω resistor on the data line is recommended for signal stability.

| LED Pin | ESP32 Pin |
|-------|-----------|
| VCC | 5V or 3.3V |
| GND | GND |
| DIN | GPIO4 (default, configurable) |

A common ground between the ESP32 and the LED is required.

**PN5180 NFC Module (SPI, right side of ESP32 top to bottom, skipping D12):**

| PN5180 Pin | ESP32 Pin | Direction | Notes |
|------------|-----------|-----------|-------|
| RST        | D13       | Output    | Hardware reset (active low) |
| *(skip)*   | *D12*     | *—*       | *Strapping pin, skip* |
| NSS        | D14       | Output    | SPI chip select (active low) |
| MOSI       | D27       | Output    | SPI data to PN5180 |
| MISO       | D26       | Input     | SPI data from PN5180 |
| SCK        | D25       | Output    | SPI clock |
| BUSY       | D33       | Input     | SPI flow control |
| GPIO       | D32       | Input     | Card detection (future use) |
| IRQ        | D35       | Input     | Interrupt, active HIGH (input-only pin, no pull-up) |
| AUX        | D34       | Input     | Auxiliary monitoring (input-only pin, no pull-up) |
| REQ        | —         | —         | **Not connected.** Only needed for PN5180 firmware updates. |
| VIN        | 5V        | Power     | |
| GND        | GND       | Power     | |

> **Note:** D35 and D34 are input-only pins on the ESP32 (no internal pull-up). D12 is a strapping pin and is skipped.

## Contributing & Help Wanted

> **This project is currently in Alpha.** Testing is being done by myself and one other person. There will be bugs — please help!

**How you can help:**

- **Beta testers** — If you build one, I'd love to hear how it goes. Try it out and [open an issue](https://github.com/sjordan0228/spoolsense_scanner/issues) with any bugs or feedback you find.
- **Case design** — I have zero CAD skills. If you're handy with Fusion 360, FreeCAD, or anything similar, I'd love a printable enclosure that fits the ESP32-WROOM-32 + PN5180 + optional LCD. Open an issue or reach out.
- **Bug reports & feedback** — Even general impressions are helpful. If something doesn't work the way you'd expect, please open an issue.

## Credits

This project is derived from and inspired by the original **openprinttag_scanner** project created by Ryan C.

* Original project: https://github.com/ryanch/openprinttag_scanner

SpoolSense Scanner builds on that foundation while adapting the firmware for the **SpoolSense ecosystem**, adding features such as:

- Optional status LED support
- Compile‑time device configuration
- Expanded NFC tag support (planned)
- Hardware profile support for multiple ESP32 variants

Many thanks to the original author and contributors for the work that made this project possible.

### Libraries Used

* **PN5180 Library** by Andreas Trappmann — ISO15693 and ISO14443A NFC driver for the PN5180 module: https://github.com/ATrappmann/PN5180-Library/

# Specs Referenced:
*   OpenPrintTag tag generator: https://openprinttag.org/generator/ 
*   Spoolman API: https://donkie.github.io/Spoolman/#tag/filament/operation/Find_filaments_filament_get 
