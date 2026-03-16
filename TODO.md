# TODO

## In Progress
- **ESP32-S3 support** — board profile define exists, needs hardware testing and any S3-specific pin/peripheral adjustments

## Planned

### Tag Format Support
- **NTAG215 / UID-only tags** — simple UID-based workflow for Spoolman/SpoolSense without OpenPrintTag data; detect tag type first, route to correct handler
- **OpenTag3D** — support as an additional tag format (long-term)

### Hardware / Build
- **LCD truly optional** — `ENABLE_LCD` define exists in `UserConfig.h` but `main.cpp` always initializes the LCD; gate I2C init, `lcdManager.begin()`, and `lcdManager.startTask()` behind the flag
- **LED pin configurable via UserConfig.h** — currently `STATUS_LED_PIN` is set in `platformio.ini` build flags; move to `UserConfig.h` alongside `ENABLE_STATUS_LED`

### Integration
- **Spoolman write support** — write spool data fetched from Spoolman directly to a tag via BLE UI
