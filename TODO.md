# TODO

## In Progress
- **ESP32-S3 support** — board profile define exists, needs hardware testing and any S3-specific pin/peripheral adjustments

## Bugs

- **Spoolman 400 on spool create** — `openprinttag_uuid` in the extra field is double-escaped (`"\"DAD4E374080104E0\""` instead of `"DAD4E374080104E0"`); every new spool create fails with HTTP 400
- **setupRF() stuck after ISO15693 read** — after a successful multi-block read, `setupRF()` fails on the next scan loop iteration; currently patched with `lastSeenValid` clear to force a hardware reset, but the root cause (PN5180 RF state not cleanly restored after batched reads) is unresolved — side effect is repeated SpoolDetected events and Spoolman spam each cycle
- **ENABLE_STATUS_LED is a no-op** — defined in `UserConfig.h` and stored in `DeviceConfig` but never wired into any `#if` guard; actual LED compilation is controlled by `USE_STATUS_LED` in `platformio.ini`, making the UserConfig define misleading

## Planned

### Tag Format Support
- **OpenTag3D** — support as an additional tag format (long-term, `TagKind::OpenTag3D` is reserved)

### Hardware / Build
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
- **LCD truly optional** — `#ifdef ENABLE_LCD` gates I2C init, `lcdManager.begin()`, `lcdManager.startTask()`, and all LCD calls in `main.cpp`; `nullptr` passed to `ApplicationManager` when disabled
- **Built-in tag writer** — HTTP server on port 80, mDNS as `spoolsense.local`, REST API (`/api/status`, `/api/write-tag`, `/api/format-tag`), UI embedded in firmware via `TagWriterHTML.h`
