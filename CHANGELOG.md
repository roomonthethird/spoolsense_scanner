# Changelog

## [1.3.3] - 2026-03-18 — Multi-page Web UI, TigerTag, OTA Updates

### Added
- Multi-page web UI at `spoolsense.local` — landing page with tool cards, shared navigation across all pages
- Tag Reader page (`/reader`) — auto-detects tag format (OpenPrintTag, TigerTag, generic UID), displays all data read-only, stops polling on detection with "Scan Again" button
- TigerTag reader support — detect and parse NTAG213/215 TigerTag binary format with embedded material/brand lookup tables
- TigerTag writer page (`/writer/tigertag`) — write filament data to NTAG tags in TigerTag format with material, brand, color, weight, diameter, aspect, and temperature fields
- `writeISO14443Pages()` — NTAG page write support via PN5180 `mifareBlockWrite4` (command 0xA2)
- `WRITE_TIGERTAG` NFC write type with 40-byte binary payload
- OTA firmware update page (`/update`) — check for updates from GitHub releases with release notes display, one-click download and flash with progress tracking, manual `.bin` upload
- Dual OTA partition table (`ota_0` + `ota_1`, 1.88MB each) — enables automatic rollback on failed update
- Async OTA download — background FreeRTOS task streams firmware from GitHub HTTPS, browser polls `/api/ota-status` for progress
- NFC scan task pause/resume during OTA upload
- `GET /api/version` — returns firmware version and board type
- `GET /api/ota-status` — returns OTA download/flash state and progress
- `POST /api/update-from-url` — ESP32 downloads and flashes firmware from a URL
- `POST /api/upload-firmware` — multipart binary upload for manual OTA
- `/api/status` extended with `tag_kind` field and nested `tigertag` object for TigerTag data
- `POST /api/write-tigertag` — assemble and enqueue TigerTag binary write
- OpenPrintTag and TigerTag logos served as PROGMEM PNGs at `/img/openprinttag.png` and `/img/tigertag.png`
- Logos displayed in writer page card headers
- Spool archive on re-tag — automatically archives old Spoolman spool when tag is re-written with different filament, or same filament with weight jump on nearly empty spool (≤100g → >500g)
- `FIRMWARE_VERSION` build flag in `platformio.ini` — single source of truth for version, shown on update page and in HA discovery
- Shared CSS (`/css/shared.css`) and JS (`/js/shared.js`) served as cacheable endpoints
- Firmware Update card on landing page
- Update link in navigation bar across all pages
- Git commit-msg hook to prevent AI attribution in commits

### Changed
- OpenPrintTag writer moved from `/` to `/writer/openprinttag` — landing page is now the tool hub
- CSS and JS extracted from inline to shared PROGMEM endpoints
- LCD status line shows `NFC+ Wifi+` / `SM+ MQTT+` (replaces BLE indicator with MQTT connection status)
- `DEVICE_VERSION` aliased to `FIRMWARE_VERSION` from build flags (was hardcoded `"0.76 BETA"`)
- ESP32-S3 NeoPixel color order fixed from `NEO_GRB` to `NEO_RGB`

### Removed
- BLE stack — `BluetoothManager.cpp/.h` deleted, `CONFIG_BT` build flags removed, BLE init removed from `main.cpp`. Saves ~540KB flash. Configuration moving to web UI

## [1.2.0] - 2026-03-18 — TigerTag Reader, NVS Config, Spoolman Fixes

### Added
- TigerTag reader support — detect and parse NTAG213/215 TigerTag binary format (ISO14443A) with material, brand, color, weight, and temperature data via embedded lookup tables
- NVS config support — `ConfigurationManager` reads from NVS partition first, per-key fallback to compile-time defaults; enables pre-built binary flashing via installer without recompiling
- GitHub Actions release workflow — auto-builds ESP32-WROOM and ESP32-S3-Zero firmware on tag push, attaches bootloader + partitions + firmware to GitHub release
- SpoolSense Installer — interactive CLI (`spoolsense-installer` repo) that downloads firmware, generates NVS config, verifies chip/flash, and flashes via esptool
- SpoolSense GitHub org — both repos transferred to `github.com/SpoolSense`

### Fixed
- Spoolman 400 on spool create — fixed double-escaped UUID, switched to `nfc_id` extra field, wrapped value as JSON string
- Spoolman filament exact match — switched to ArduinoJson for filament lookup; streaming parser couldn't handle nested vendor objects, was creating duplicate filaments
- Material type dropdown — fixed tag writer dropdown order to match OpenPrintTag enum (TPU=2, ABS=3, ASA=4); was writing wrong type codes
- setupRF() after tag re-detection — fixed LED state being overwritten after setupRF failure

## [1.1.1] - 2026-03-17 — ESP32-S3-Zero Support

### Added
- ESP32-S3-Zero as a second supported board alongside ESP32-WROOM-32
- `include/BoardPins.h` — centralized pin definitions for all boards, auto-selected at compile time
- `[env:esp32s3zero]` PlatformIO build environment with USB CDC serial, 4MB flash, and S3-specific BLE init
- S3-Zero onboard WS2812 RGB LED support (GPIO 21, no external wiring needed)
- S3-Zero LCD support on GPIO 1 (SDA) / GPIO 2 (SCL)
- S3-Zero wiring documentation in README with power sharing notes

### Changed
- Unified LED enable flag: `ENABLE_STATUS_LED` in `UserConfig.h` replaces `USE_STATUS_LED` build flag in `platformio.ini`
- LED pin now defined in `BoardPins.h` (was `-DSTATUS_LED_PIN=4` build flag)
- Serial baud rate changed from 9600 to 115200 on WROOM
- `platformio.ini` refactored to use shared base `[env]` section — both board environments inherit common settings
- BLE controller init on S3 handled by `BLEDevice::init()` (WROOM retains manual classic BT memory release)
- LED pixel type auto-selected: NEO_GRB (S3 onboard WS2812) or NEO_GRBW (WROOM external SK6812)

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
- `ConfigurationManager` loads from compile-time config instead of NVS
- LED persists last filament color after tag removal — no longer clears on
  `TAG_REMOVED`; color stays until the next scan
- Tag scan flash changed from single yellow to 3 white flashes

### Removed
- BLE device configuration (WiFi, MQTT, Spoolman settings via BLE app)
- Settings tab from BLE web UI — spool tag operations remain
- PrusaLink / direct printer integration — out of scope for SpoolSense
- `PrinterManager`, `PrusaLinkAPIStrategy`, `BgcodeParser` and related files
