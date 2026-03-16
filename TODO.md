# TODO

## In Progress
- **ESP32-S3 support** — board profile define exists, needs hardware testing and any S3-specific pin/peripheral adjustments

## Planned

### Tag Format Support
- **OpenTag3D** — support as an additional tag format (long-term, `TagKind::OpenTag3D` is reserved)

### Hardware / Build
- **LCD truly optional** — `ENABLE_LCD` define exists in `UserConfig.h` but `main.cpp` always initializes the LCD; gate I2C init, `lcdManager.begin()`, and `lcdManager.startTask()` behind the flag
- **LED pin configurable via UserConfig.h** — currently `STATUS_LED_PIN` is set in `platformio.ini` build flags; move to `UserConfig.h` alongside `ENABLE_STATUS_LED`
- **Scanner naming** — configurable name (e.g. `Toolhead1-scanner`, `Lane1-scanner`) via `UserConfig.h`, reflected in BLE device name and MQTT topics

### Integration
- **Spoolman write support** — write spool data fetched from Spoolman directly to a tag via BLE UI

### Architecture / Overlap to Resolve
- **Dual weight-sync ownership** — both `SpoolmanManager` (scanner) and the SpoolSense middleware `tag_sync` module can write updated remaining weight back to a tag. They go through the same write queue so there's no hardware conflict, but long-term one side should own this responsibility. Candidate: let the middleware be the single owner and disable/remove weight writeback from `SpoolmanManager`.

## Completed
- **NTAG215 / UID-only tags** — ISO14443A detection via PN5180ISO14443, UID published as `GENERIC_TAG_DETECTED`, LCD shows "Generic Tag / UID scan only"
- **Tag classification model** — `TagProtocol`, `TagKind`, `TagScanResult` with UID-length heuristic
- **LED FreeRTOS task** — non-blocking flash (3 white flashes on scan), persistent filament color after tag removal, breathing animation for low spool (≤100g)
- **SLIX2 WDT fix** — batched 16-block reads replace single 78-block transaction; eliminates cascading BUSY pin spin-wait timeouts
- **Project rename** — `openprinttag_scanner` → `spoolsense_scanner` throughout MQTT topics, HA discovery, BLE name, source files
- **Compile-time config** — all settings in `include/UserConfig.h`; BLE settings config removed
