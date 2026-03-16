# Changelog

## [1.0.0] - 2026-03-16

### Added
- `UserConfig.h` compile-time configuration — WiFi, MQTT, Spoolman, LCD, LED,
  automation mode, and board selection are now set before flashing
- `AUTOMATION_MODE` define: `0` = Self Directed (scanner auto-deducts filament),
  `1` = Controlled by Home Assistant
- Simplified board selection: `#define BOARD_ESP32_WROOM` or `#define BOARD_ESP32_S3`

### Changed
- Project renamed from `openprinttag_scanner` to `spoolsense_scanner`
- MQTT base topic changed from `openprinttag/` to `spoolsense/`
- HA discovery entity IDs changed from `openprinttag_*` to `spoolsense_*`
  (old entities are auto-removed on first connection)
- BLE device name changed from `OpenPrintTag-XXXX` to `SpoolSense-XXXX`
- `ConfigurationManager` loads from compile-time config instead of NVS

### Removed
- BLE device configuration (WiFi, MQTT, Spoolman settings via BLE app)
- Settings tab from BLE web UI — spool tag operations remain
- PrusaLink / direct printer integration — out of scope for SpoolSense
- `PrinterManager`, `PrusaLinkAPIStrategy`, `BgcodeParser` and related files
