# Overview:
ESP32 / ESP32-S3 Arduino project built with PlatformIO.
Primary current hardware target is ESP32 DevKit / WROOM (typically 4MB flash).
PN5180 NFC scanner firmware with OpenPrintTag support today and planned generic NFC tag support. The firmware is intended to function as a standalone NFC scanner for spool management systems such as SpoolSense rather than direct printer integrations.
16x2 I2C LCD support is optional.
All config is compile-time via UserConfig.h. OTA firmware updates via web UI.

# Guidelines:
Be concise in responses
Run compile checks before handing off: `pio run -e esp32s3zero` (primary target) and `pio run -e esp32dev` (secondary). Do NOT flash — only compile.
Consider thread safety for all changes
Add new files to source inventory (one-line)
Run tests after changes: ./scripts/run_all_tests.sh
Avoid adding more heap allocations - the device is low on memory.
Use StaticJsonDocument document, even though it is deprecated.
Use LSP plugins when searching for C/C++ and TypeScript identifiers.
All user-editable configuration must live in include/UserConfig.h.
Do not add hardcoded credentials or environment-specific settings anywhere else in the codebase.

# Project goals:
- Rename project to `spoolsense_scanner` and remove legacy `openprinttag_scanner` naming throughout the codebase, docs, BLE strings, and UI.
- Keep OpenPrintTag support as the first fully supported tag format.
- Add support for standard NFC tags such as NTAG215 for simple UID-based Spoolman / SpoolSense workflows. These tags are not OpenPrintTag and should be handled as a separate path.
- Keep LCD support optional as a first-class build/profile feature.
- Add and stabilize ESP32-S3 support.
- Long-term stretch goal: support OpenTag3D as an additional tag format later.

# Architecture summary:
main.cpp: Initializes all managers, starts FreeRTOS tasks
ApplicationManager: Central state machine + message bus, receives events (print start, spool scan, etc.) via queue and coordinates responses.
NFC Stack: NFCManager -> Hardware NFC adapter (PN5180 today) -> tag protocol handler -> openprinttag_lib (for OpenPrintTag) or generic tag handler
Spool Sync: ApplicationManager triggers sync -> SpoolmanManager queues request -> SpoolmanManager task -> HTTP requests to SpoolSense / Spoolman style APIs.
Configuration: Compile-time via UserConfig.h -> DeviceConfig -> ConfigurationManager. BLE (BluetoothManager) handles only spool tag operations at runtime. Long-term direction is a broader multi-format scanner architecture for SpoolSense.

# Source Inventory
OpenPrintTag Library
lib/openprinttag/cbor.h / cbor_native.c — Minimal CBOR implementation used by the OpenPrintTag encoder/decoder and native tests
lib/openprinttag/openprinttag_lib.c / .h — Encode/decode filament data (CBOR, NDEF)
lib/openprinttag/openprinttag_pn532.h — Example HAL adapter for PN532-style 4‑byte page NFC implementations (pn532-esp-idf)
lib/openprinttag/openprinttag_adafruit_pn532.h — HAL adapter for Adafruit_PN532 Arduino library

PN5180 Driver
lib/PN5180/Debug.cpp / .h — Hex/debug helpers
lib/PN5180/PN5180.cpp / .h — Core driver, SPI + register control
lib/PN5180/PN5180ISO15693.cpp / .h — ISO15693 protocol implementation
lib/PN5180/PN5180ISO14443.cpp / .h — ISO14443A detection (Type A activate + anticollision, NTAG215 UID) — Copyright 2019 Dirk Carstensen, LGPL-2.1

Board / Config
include/BoardPins.h — Board-conditional pin definitions (#define), auto-selected via BOARD_ESP32_S3 from UserConfig.h

Application Core
src/main.cpp — Entry point, task startup
src/ApplicationManager.cpp / .h — Central state machine + event queue
src/ConfigurationManager.cpp / .h — Device config (loaded from UserConfig.h at boot via DeviceConfig)
src/DeviceConfig.cpp / .h — Compile-time config struct populated from UserConfig.h defines

NFC
src/NFCManager.cpp / .h — NFC scan/read/write task and primary tag detection/handling entry point
src/HardwareNFCConnection.cpp / .h — PN5180 hardware adapter (ISO15693 + ISO14443A page read/write)
src/HardwareNFCConnectionPN532.cpp / .h — PN532 hardware adapter (ISO14443A only, Adafruit_PN532)
src/NFCConnectionI.h — NFC hardware interface
src/NFCTypes.h — Detected spool state structs (TagKind: OpenPrintTag, GenericUidTag, OpenTag3D, TigerTag, BlankTag)
src/NFCWriteTypes.h — Write queue types/enums (includes WRITE_TIGERTAG with 40-byte payload)
src/TigerTagParser.cpp / .h — TigerTag NTAG213 binary parser with embedded material/brand lookup tables


Spool Sync
src/SpoolmanManager.cpp / .h — Spoolman API sync + queue worker

Home Assistant
src/HomeAssistantManager.cpp / .h — MQTT client task, publish/subscribe, HA discovery

UI / UX
src/DisplayI.h — Display interface (showText, showSpool, showKeypad, showWriteResult) — implemented by LCDManager and TFTManager
src/LCDManager.cpp / .h — I2C LCD task + status updates, implements DisplayI
src/LCDDisplayLogic.h — Shared LCD message merge/timing rules
src/TFTManager.cpp / .h — ST7789 240x240 TFT display via LovyanGFX, implements DisplayI, 8-bit color sprite rendering
src/TFTConfig.h — LovyanGFX hardware config per board (SPI bus, pins, panel settings)
src/WebServerManager.cpp / .h — HTTP server (port 80, mDNS spoolsense.local); multi-page UI + API endpoints + OTA upload
src/LandingHTML.h — Landing page PROGMEM served at GET /
src/ReaderHTML.h — Tag reader page PROGMEM served at GET /reader
src/TagWriterHTML.h — OpenPrintTag writer page PROGMEM served at GET /writer/openprinttag
src/TigerTagWriterHTML.h — TigerTag writer page PROGMEM served at GET /writer/tigertag
src/UpdateHTML.h — Firmware update page PROGMEM served at GET /update (auto-check GitHub + manual upload)
src/ConfigHTML.h — Device configuration page PROGMEM served at GET /config (WiFi, MQTT, Spoolman, hardware)
src/SharedCSS.h — Shared CSS PROGMEM served at GET /css/shared.css
src/SharedJS.h — Shared JS PROGMEM served at GET /js/shared.js
src/OpenPrintTagLogo.h — OpenPrintTag logo PNG served at GET /img/openprinttag.png
src/TigerTagLogo.h — TigerTag logo PNG served at GET /img/tigertag.png
docs/writer-ui-plan.md — Tag writer UI redesign plan

Utilities
src/ConversionUtils.cpp / .h — Shared data format conversion utilities (material types, colors, density defaults)
src/InputManager.cpp / .h — Optional 3x4 matrix keypad driver; polls for key presses and enqueues KEYPAD_DIGIT/CONFIRM/CANCEL messages (compiled only when ENABLE_KEYPAD=1)

Tests
OpenPrintTag
test/test_openprinttag.c — CBOR + NDEF unit tests (mock HAL)

Native Fakes / Stubs
test/native/FakeLCDManager.h — In-memory LCD
test/native/StubApplicationManager.h — Message capture stub
test/native/StubNFCConnection.h — Simulated NFC tags
test/native/NativePlatform.cpp — Stub Serial

Native Tests
test/native/test_app_flow.cpp — App state transitions
test/native/test_lcd_manager.cpp — LCD message merge timing behavior
test/native/test_nfc_read.cpp — NFC read behavior
test/native/test_raw_write.cpp — Raw binary write to NFC tag
test/native/TestableApplicationManager.h — Queue bypass harness
test/native/TestNFCManager.h — Write queue tracker
test/native/test_helpers.h — Factories + assertions

Integration Tests
test/integration/ha.cpp — Native standalone MQTT/HA connectivity + discovery/state publisher
test/integration/Makefile — Build/run helper for local HA integration probe

Integration / HIL Test Harness
test/integration/http_server.py — Test orchestrator + mock spool management APIs + SSE server
test/integration/mock_spoolman.py — Mock Spoolman API state controller
test/integration/scenarios/base.py — BaseTestScenario with BLE bridge helpers
test/integration/scenarios/test_format_spool.py — Format spool test
test/integration/scenarios/test_set_filament.py — Set filament weight test
test/integration/scenarios/test_set_filament_profile.py — Set filament type/manufacturer test
test/integration/scenarios/test_print_e2e.py — End-to-end print simulation test
test/integration/scenarios/test_print_30_percent.py — Canceled print at 30% integration test
test/integration/scenarios/test_print_100x.py — 100x print endurance test (excluded from run-all)
test/integration/scenarios/test_recent_spools.py — Swap spool A/B and verify recently seen spool history
test/integration/scenarios/test_spoolman_sync.py — Spoolman sync verification test
test/integration/scenarios/test_color_update.py — Color field update and verification test
test/integration/scenarios/test_spool_swap_during_print.py — Mid-print spool swap edge case test
test/integration/scenarios/test_zero_weight_handling.py — Zero weight boundary and clamping test
test/integration/scenarios/test_printer_api_errors.py — PrusaLink API error resilience test
test/integration/scenarios/test_print_progress_edge_cases.py — Print progress edge cases (0%, 100%, dwell)
test/integration/scenarios/test_automation_mode_controlled.py — HA-controlled mode (no auto-deduction) test
test/integration/scenarios/test_job_disappeared_deduction.py — Job disappeared (204) bgcode fallback deduction test
test/integration/scenarios/test_real_tag.py — Raw binary write from fixture and 100g deduction verification
test/integration/scenarios/test_write_spoolman_spool.py — Write Spoolman spool test (Mode A API fetch, Mode B direct data)
test/integration/site/index.html — Web Bluetooth test runner UI
test/integration/requirements.txt — Python dependencies for integration tests (paho-mqtt)
test/integration/mqtt_config.json — MQTT broker configuration for event-driven test waits

# Notes for future direction
- Treat OpenPrintTag, generic UID tags, and future formats such as OpenTag3D as separate handler paths.
- Prefer protocol/tag detection first, then route to the correct parser/handler.
- Reuse `lib/openprinttag/*` as the OpenPrintTag engine, while evolving the surrounding scanner firmware under the `spoolsense_scanner` identity.

- Direct printer integrations (PrusaLink, OctoPrint) are intentionally out of scope for spoolsense_scanner and should not be expanded further in this project.

# gstack
Use the /browse skill from gstack for all web browsing. Never use mcp__claude-in-chrome__* tools.

Available skills:
- /plan-ceo-review
- /plan-eng-review
- /plan-design-review
- /design-consultation
- /review
- /ship
- /browse
- /qa
- /qa-only
- /qa-design-review
- /setup-browser-cookies
- /retro
- /document-release
