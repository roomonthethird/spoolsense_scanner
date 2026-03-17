# Changelog

## [1.1.0] - 2026-03-16 — Built-in Tag Writer (Early Beta)

### Added
- Built-in HTTP web server (port 80) — reachable at `http://spoolsense.local` after WiFi connects
- Tag writer UI served directly from the device — write OpenPrintTag fields from any browser on the local network
- REST API for tag operations: `GET /api/status`, `POST /api/write-tag`, `POST /api/format-tag`
- Tag writer supports all OpenPrintTag fields: material type, manufacturer, custom material name,
  full/remaining weight, color, density, diameter, print/bed/preheat temperatures, and Spoolman ID
- CORS headers on all endpoints for local development testing
- mDNS registration as `spoolsense.local`
- NFC read retry logic (up to 3 attempts with RF reset between retries) — fixes ISO15693 read
  failures that previously left the NFC stack in a stuck state
- `setupRF()` failure recovery: clears stuck RF state immediately instead of waiting 30s for watchdog

### Changed
- NFC scan loop now performs a full hardware reset when `setupRF()` fails with a tag present,
  recovering in one cycle instead of ~30 seconds
- LCD is now truly optional — `ENABLE_LCD 0` in `UserConfig.h` fully gates I2C init, LCD task,
  and all LCD calls; no Wire library or LCD code compiled when disabled

## [1.0.0] - 2026-03-14 — Initial Release

### Added
- OpenPrintTag (ISO15693) full CBOR/NDEF read and write via PN5180
- ISO14443A tag detection (NTAG215 and other Type A tags) via PN5180ISO14443
  library (Copyright 2019 Dirk Carstensen, LGPL-2.1)
- `TagProtocol`, `TagKind`, and `TagScanResult` classification model for
  multi-format tag routing
- `GENERIC_TAG_DETECTED` application event for UID-only ISO14443A tags —
  LCD shows "Generic Tag / UID scan only", HA publishes `blank:false`
- `OpenTag3D` reserved in `TagKind` enum for future support
- `UserConfig.h` compile-time configuration — WiFi, MQTT, Spoolman, LCD, LED,
  automation mode, and board selection are now set before flashing
- `AUTOMATION_MODE` define: `0` = Self Directed (scanner auto-deducts filament),
  `1` = Controlled by Home Assistant
- Simplified board selection: `#define BOARD_ESP32_WROOM` or `#define BOARD_ESP32_S3`
- Status LED (SK6812 RGBW) support via FreeRTOS task — non-blocking flash,
  persistent filament color, breathing animation for low spool (≤100g)
- Full SpoolDetected payload dump to serial log for debugging
- Read cache batching (16-block reads) to fix WDT crash on SLIX2 tags with
  per-command block limits

### Changed
- Project renamed from `openprinttag_scanner` to `spoolsense_scanner`
- MQTT base topic changed from `openprinttag/` to `spoolsense/`
- HA discovery entity IDs changed from `openprinttag_*` to `spoolsense_*`
  (old entities are auto-removed on first connection)
- BLE device name changed from `OpenPrintTag-XXXX` to `SpoolSense-XXXX`
- `ConfigurationManager` loads from compile-time config instead of NVS
- LED persists last filament color after tag removal — no longer clears on
  `TAG_REMOVED`; color stays until the next scan
- Tag scan flash changed from single yellow to 3 white flashes

### Removed
- BLE device configuration (WiFi, MQTT, Spoolman settings via BLE app)
- Settings tab from BLE web UI — spool tag operations remain
- PrusaLink / direct printer integration — out of scope for SpoolSense
- `PrinterManager`, `PrusaLinkAPIStrategy`, `BgcodeParser` and related files
