# SpoolSense Scanner

## Overview
SpoolSense Scanner is an ESP32‑based NFC scanner designed for managing 3D printer filament spools using NFC tags. It supports reading and writing tags formatted according to the OpenPrintTag standard and is designed to integrate with the SpoolSense ecosystem.

The scanner allows users to tap a filament spool to identify it, retrieve metadata from the NFC tag, and trigger external automation or spool tracking workflows.

Configuration is performed through a web interface delivered over Bluetooth Low Energy (BLE), allowing WiFi credentials and other device settings to be configured without recompiling firmware.

## How to configure
1.  On first boot, or when WiFi is not configured, the device will start a BLE service.
2.  Connect to the device named "SpoolSenseScanner" from your computer or phone.
3.  Once connected, you can access the configuration web interface to:
    *   Configure WiFi SSID and password.
    *   Set the IP address and API key for your PrusaLink-compatible printer.

## Functionality
* **NFC Tag Reading/Writing:** Reads and writes NFC tags formatted according to the OpenPrintTag specification.
* **UID Tag Support (planned):** Future support for simple UID‑only NFC tags such as NTAG215 used with spool tracking systems like Spoolman.
* **LCD Display (optional):** Displays device status, NFC scan results, and system information.
* **Bluetooth Configuration:** Provides a web‑based UI over BLE for easy device setup.
* **Extensible Architecture:** The firmware is designed so additional tag formats and integrations can be added over time.

# Hardware Setup


## Hardware Needed
*   NFC Reader/Writer: PN5180 NFC module (ISO 15693)
*   LCD Screen: [16x2 I2C LCD](https://a.co/d/dryhwvd) (only 1 needed)
*   ESP32: [ESP32 DevKitC V4](https://a.co/d/gW3zBIJ) (only 1 needed)
*   USB Cable: USB-A to USB-C (1)
*   Jumper wires: male-to-female Dupont wires (9)
*   Optional Status LED: SK6812 RGBW (WS2812-compatible addressable LED, 1 pixel)

## Optional Status LED

The scanner firmware now supports an optional single‑pixel addressable status LED. This LED provides visual feedback for scanner state and spool/tag events.

**Recommended LED:**

* SK6812 RGBW (WS2812‑compatible addressable LED)

The firmware is configured for **SK6812 RGBW timing and color order** using:

`NEO_GRBW + NEO_KHZ800`

### Wiring

Default LED data pin:

`GPIO4`

> **Note:** A single SK6812 RGBW LED module is recommended. Many small breakout boards include the necessary capacitor and resistor already. If using a bare LED, a ~330Ω resistor on the data line is recommended for signal stability.

Typical wiring for a single LED:

| LED Pin | ESP32 Pin |
|-------|-----------|
| VCC | 5V |
| GND | GND |
| DIN | GPIO4 |

A common ground between the ESP32 and the LED is required.

### LED Status States

| Event | LED Behavior |
|------|--------------|
| Booting | White |
| WiFi connected | Cyan |
| Ready | Blue |
| Tag detected | Yellow flash |
| Valid spool/tag | Filament color |
| Blank/invalid tag | Red flash |
| Tag removed | Off |
| Write success | Green flash, then firmware restores filament color |
| Write failure | Red flash, then firmware restores filament color or turns LED off |

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

### Printables BOM (non-printed parts)
The model page lists this BOM:

| Item | Qty | 
|---|---:|---|
| PN5180 NFC module | 1 | 
| ESP32-WROOM-32 (40 pin) | 1 |
| 16x2 I2C LCD module | 1 | 
| USB-A to USB-C cable | 1 | 
| Dupont jumper wires (M/F) | 9 | 

## Hardware Configuration
Connect the components to the ESP32 as follows:

**16x2 I2C LCD (I2C):**
*   **GND:** GND
*   **VCC:** 5V
*   **SDA:** GPIO 23
*   **SCL:** GPIO 22

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

# Specs Referenced:
*   OpenPrintTag tag generator: https://openprinttag.org/generator/ 
*   Spoolman API: https://donkie.github.io/Spoolman/#tag/filament/operation/Find_filaments_filament_get 
