# TODO

## Bugs

- ~~[P0] **setupRF() stuck after ISO15693 read** — after a successful multi-block read, `setupRF()` fails on the next scan loop iteration; currently patched with `lastSeenValid` clear to force a hardware reset, but the root cause (PN5180 RF state not cleanly restored after batched reads) is unresolved — side effect is repeated SpoolDetected events and Spoolman spam each cycle~~
- ~~[P0] **Spoolman spool lookup creates duplicates** — `parseSpoolIdByUuid` has the same nested-object depth bug as the filament parser; spool response contains nested `filament` and `vendor` objects whose `id` fields confuse the streaming JSON reader, causing the scanner to create a new spool every scan instead of updating the existing one~~
- [P1] **ledManager references not gated behind ENABLE_STATUS_LED** — `ApplicationManager.cpp` calls `ledManager.showFilamentColor()`, `ledManager.showOff()`, `ledManager.flashTagDetected()` etc. without `#ifdef ENABLE_STATUS_LED` guards. Compiles on device (extern resolves) but breaks native tests where LEDManager isn't available. Need to wrap all ledManager calls in `#ifdef ENABLE_STATUS_LED` blocks
- ~~[P2] **Remaining legacy `openprinttag` naming** — project name in `CMakeLists.txt` and `.code-workspace` filename~~
- ~~[P2] **TigerTag dropdown API fetch creates invisible options** — fixed~~

## Planned

### Tag Format Support
- ~~[P3] **OpenTag3D** — support as an additional tag format (long-term, `TagKind::OpenTag3D` is reserved)~~
- [P2] **Proprietary tag format support** — OpenRFID project (GPL v3, /Users/sjordan/Code/OpenRFID) has parsers for manufacturer-specific tag formats. Can be used as reference for format documentation. Licensing: middleware can use directly if made GPL; scanner firmware must implement independently. Formats available:
  - **Bambu Lab** — MIFARE Classic 1K, AES-encrypted, HKDF-SHA256 key derivation from UID + salt. Full data: material, color, weight, temps, production date, tray UID
  - **Creality** — proprietary format on MIFARE Classic/Ultralight
  - **Elegoo** — proprietary format on MIFARE Classic/Ultralight
  - **Anycubic** — proprietary format on MIFARE Classic/Ultralight
  - **Snapmaker** — proprietary format on MIFARE Classic/Ultralight
  - **OpenSpool** — NDEF-based format
- ~~**TigerTag** — NTAG213 (ISO14443A) fixed binary layout format; simpler than OpenPrintTag (144 bytes, raw byte offsets, no CBOR); ISO14443A detection already works via PN5180ISO14443; no consumed_weight field so weight tracking stays in Spoolman only; has ECDSA signature (64 bytes) for authentication; spec at https://github.com/TigerTag-Project/TigerTag-RFID-Guide~~
- [P2] **Bambu Lab spool tags** — MIFARE Ultralight AES (MF0AES) with AES-128 encrypted data pages. PN5180 can activate via ISO14443A (same anticollision as NTAG). **Phase 1:** detect Bambu tag via ATQA/SAK or GET_VERSION chip response, classify as `TagKind::BambuTag`, extract UID, treat as UID-only for Spoolman registration. Public pages (0-1) are readable without auth. **Phase 2:** best-effort metadata decode from unencrypted public pages — community reverse engineering suggests some material/color bytes may be in the clear area. Fragile and undocumented but worth investigating.

### PN5180 Library
- [P2] **`readData` buffer overload** — tueddy/hyutrn forks add `readData(int len, uint8_t *buffer)` which writes into a caller-provided buffer instead of heap-allocating; reduces heap churn on a memory-constrained device
- [P3] **`getInventoryMultiple()` research** — tueddy/hyutrn forks implement multi-tag inventory with 16-slot collision handling; investigate whether a scanner positioned between two spools could read both simultaneously; would need physical testing to determine if PN5180 RF field geometry supports it in practice
- [P3] **`isCardPresent()` for ISO14443** — tueddy/hyutrn forks add a simple boolean card-presence check for Type A tags; could simplify and clean up the generic tag detection path
- [P2] **Consider upgrading to hyutrn fork** — actively maintained (v2.3.7, Sept 2025), FreeRTOS-aware (reduced blocking delays), ~500 bytes smaller, fixes unknown manufacturer ID 0xFF; same API as tueddy so migration path is straightforward but requires constructor and call-site updates

### Performance
- [P2] **Instrument the write path** — add timing logs for each phase: format duration, number of blocks written, total write time, verify duration; output something like `format=320ms blocks=11 write=1840ms verify=410ms`; needed before any optimization to know where time is actually spent
- [P3] **Dirty-block write optimization** — investigate whether the current write path already skips unchanged blocks or writes all blocks every time; writing only dirty blocks is the biggest firmware-side win available without changing libraries
- [P3] **Blank vs. existing tag write time** — format step adds writes before the payload even starts; profile separately to understand if slow writes are blank-tag-only or affect rewrites too
- [P3] **Write Multiple Blocks investigation** — check whether the SLIX2 tags in use support ISO15693 Write Multiple Blocks (command 0x24); if so, batching block writes could reduce round-trips; note: many tags do not support this command so verify against tag datasheet first

### Firmware / Infrastructure
- ~~[P1] **OTA firmware updates** — support over-the-air updates via WiFi so deployed scanners can be updated without USB reflash; ESP32 Arduino OTA is available~~
- [P2] **MQTT reconnect robustness** — audit whether `HomeAssistantManager` cleanly handles broker drops and reconnects in long-running deployments; verify subscriptions are re-established after reconnect
- [P3] **Configurable log verbosity** — add `LOG_LEVEL` define to `UserConfig.h` (e.g. DEBUG/INFO/WARN) to reduce serial noise in production without losing full output for debugging

### Web / UI
- ~~**Status page** — add a landing page at `http://spoolsense.local/` showing current spool, WiFi signal, MQTT status, uptime, and free heap; makes the device debuggable without serial access~~
- ~~[P1] **Web-based config** — add a protected config page at `spoolsense.local/config` to replace BLE-based configuration; allow WiFi/MQTT/Spoolman settings to be changed without reflashing~~
- [P1] **Troubleshooting page** — add a page at `spoolsense.local/troubleshooting` for verifying scanner setup. Tests: Spoolman connectivity (GET /api/v1/info), MQTT broker connectivity (connect + publish test), WiFi signal strength (RSSI), NFC reader status (PN5180 firmware version, RF state), free heap/uptime. Display scanner device ID prominently so users can find it for middleware config. Show pass/fail for each test with actionable error messages.
- ~~[P1] **Scanner device ID on web UI** — display the scanner's device ID on the landing page so users can find it for middleware config~~
- ~~[P1] **Unified installer** — `spoolsense-installer` repo under the SpoolSense org; interactive CLI that covers both scanner and middleware~~
- [P1] **Tag writer auto-populate** — when a tag with existing data is placed on the reader, auto-fill the writer form fields with the tag's current values (material, color, weight, manufacturer, etc.); lets users scan a tag to check its contents and overwrite individual fields

### Tag Writer Enhancements
- ~~**Tag reader view** — scan any tag, auto-detect the format (OpenPrintTag, OpenTag3D, TigerTag, UID-only), display all data in a clean read-only view; foundation for auto-populate and format-specific writing~~
- ~~[P3] **OpenTag3D writer** — write OpenTag3D tags from the web UI; same NDEF + CBOR pattern as OpenPrintTag but with OpenTag3D MIME type and version handling; depends on OpenTag3D reader support~~
- ~~**TigerTag writer** — write TigerTag format to NTAG213 tags from the web UI; fixed byte layout, no CBOR; unsigned only (ECDSA signing requires TigerTag private key); depends on TigerTag reader support~~
- [P1] **UID-only Spoolman registration** — scan a plain UID tag (NTAG215 etc.), display the UID, offer a "Register in Spoolman" button that creates a spool entry with that UID as `nfc_id`; no data written to the tag itself
- [P2] **TigerTag SpoolmanDB mapping** — TigerTag maintains a Spoolman-compatible materials database at https://github.com/TigerTag-Project/TigerTag-RFID-Guide/tree/main/SpoolmanDB; live API at `https://api.tigertag.io/api:tigertag/SpoolmanDB/materials` returns 100 materials with id/material/density/extruder_temp/bed_temp; the `id` field matches the `material_id` in the TigerTag binary format (e.g. 38219=PLA, density=1.24); simplest approach: document that users should import `materials.json` into Spoolman so filament matching from TigerTag scans just works; longer term: hardcode top ~20 material densities in firmware for offline use

### Hardware / Build
- [P2] **Scanner naming** — configurable name (e.g. `Toolhead1-scanner`, `Lane1-scanner`) via `UserConfig.h`, reflected in BLE device name and MQTT topics
- [P2] **TFT display support (ST7789)** — explore replacing the 16x2 I2C LCD with an ST7789 TFT module; color display could show filament color swatch, spool info, and scanner status in a richer format; would need SPI (shared bus with PN5180 or separate), new display driver library, and a display task refactor

### Debugging / Logging
- ~~[P2] **No serial output on tag write** — when the web UI triggers a write, nothing is logged to serial; add a write-dispatched log line to make debugging easier~~

### Integration
- [P2] **Spoolman write support** — write spool data fetched from Spoolman directly to a tag via BLE UI

### Ecosystem
- [P3] **Shared specs repo** — `spoolsense-specs` repo under the SpoolSense org documenting tag formats (OpenPrintTag, OpenTag3D, NTAG215 UID-only), MQTT payload schema, and REST API contract between scanner and middleware; becomes the source of truth both repos reference

### Spoolman Integration
- [P1] **OpenPrintTag extra fields** — register additional Spoolman extra fields to surface OpenPrintTag data in the Spoolman UI: `material_name`, `min_print_temp`, `max_print_temp`, `preheat_temp`, `min_bed_temp`, `max_bed_temp`, `openprinttag_version`; installer should auto-register these; scanner writes them during sync
- [P1] **Preserve existing extra fields on update** — Spoolman's API replaces the entire `extra` object on update rather than merging; sync logic must read existing extra fields first, merge in updated values, then write the combined set to avoid clobbering fields set by other systems (e.g. `active_toolhead` set by the middleware)

### AFC Integration
- ~~[P1] **Direct AFC lane control from tag data** — implemented in middleware v1.5.0. SET_COLOR, SET_MATERIAL, SET_WEIGHT sent from tag data without Spoolman. Works with afc_stage (shared scanner) and afc_lane (per-lane scanner).~~

### Architecture / Overlap to Resolve
- [P1] **Skip redundant Spoolman syncs** — every tag placement triggers a PATCH even if nothing changed. Add local cache: if UID + weight + material match last sync, skip the API call. Cache invalidated by middleware MQTT write commands or 2-hour TTL. See `docs/deep-thoughts.md` for design questions around cache invalidation and source-of-truth ownership.
- [P2] **Dual weight-sync ownership** — both `SpoolmanManager` (scanner) and the SpoolSense middleware `tag_sync` module can write updated remaining weight back to a tag. They go through the same write queue so there's no hardware conflict, but long-term one side should own this responsibility. Candidate: let the middleware be the single owner and disable/remove weight writeback from `SpoolmanManager`.
- [P2] **Scanner vs middleware sync architecture review** — deep dive needed into who owns what. Scanner creates spools, middleware updates weight, both can PATCH Spoolman. Risk of duplicate work, race conditions, and conflicting sources of truth. See `docs/deep-thoughts.md` for full analysis.

## Completed
- **Scanner device ID on web UI** — device ID displayed on landing page
- **Unified installer** — `spoolsense-installer` repo with interactive CLI for scanner + middleware setup
- **Direct AFC lane control from tag data** — middleware v1.5.0 sends SET_COLOR/SET_MATERIAL/SET_WEIGHT without Spoolman
- **NTAG215 / UID-only tags** — ISO14443A detection via PN5180ISO14443, UID published as `GENERIC_TAG_DETECTED`, LCD shows "Generic Tag / UID scan only"
- **Tag classification model** — `TagProtocol`, `TagKind`, `TagScanResult` with UID-length heuristic
- **LED FreeRTOS task** — non-blocking flash (3 white flashes on scan), persistent filament color after tag removal, breathing animation for low spool (≤100g)
- **SLIX2 WDT fix** — batched 16-block reads replace single 78-block transaction; eliminates cascading BUSY pin spin-wait timeouts
- **Project rename** — `openprinttag_scanner` → `spoolsense_scanner` throughout MQTT topics, HA discovery, BLE name, source files
- **Compile-time config** — all settings in `include/UserConfig.h`; BLE settings config removed
- **LCD truly optional** — `#ifdef ENABLE_LCD` gates I2C init, `lcdManager.begin()`, `lcdManager.startTask()`, and all LCD calls in `main.cpp`; `nullptr` passed to `ApplicationManager` when disabled
- **Built-in tag writer** — HTTP server on port 80, mDNS as `spoolsense.local`, REST API (`/api/status`, `/api/write-tag`, `/api/format-tag`), UI embedded in firmware via `TagWriterHTML.h`
- **LED pin in BoardPins.h** — pin auto-selected per board; `STATUS_LED_PIN` build flag removed
- **ENABLE_STATUS_LED unified** — `USE_STATUS_LED` build flag removed; `ENABLE_STATUS_LED` from `UserConfig.h` is now the single flag; `ApplicationManager.cpp` fixed to include `UserConfig.h` so LED calls compile in correctly
- **ESP32-S3 support** — `BoardPins.h` auto-selects pins per board; `platformio.ini` has `esp32s3zero` env; `ARDUINO_USB_CDC_ON_BOOT` enabled; LED type `NEO_RGB` for S3 onboard WS2812; hardware validated on S3-Zero with PN5180
- **Spoolman 400 on spool create** — fixed double-escaped UUID, switched to `nfc_id` extra field, wrapped value as JSON string
- **Material type dropdown** — fixed tag writer dropdown order to match OpenPrintTag enum (TPU=2, ABS=3, ASA=4); was writing wrong type codes
- **S3-Zero LED color order** — changed from `NEO_GRB` to `NEO_RGB`; red and green were swapped
- **Spoolman filament exact match** — switched to ArduinoJson for filament lookup; streaming parser couldn't handle nested vendor objects, was creating duplicate filaments
- **NVS config support** — `ConfigurationManager` reads from NVS partition first, per-key fallback to compile-time defaults; enables pre-built binary flashing via installer
- **GitHub Actions release workflow** — auto-builds ESP32-WROOM and ESP32-S3-Zero firmware on tag push, attaches bootloader + partitions + firmware to release
- **SpoolSense Installer** — interactive CLI (`spoolsense-installer` repo) that downloads firmware, generates NVS config, verifies chip/flash, and flashes via esptool; also installs middleware with systemd service
- **SpoolSense GitHub org** — both repos transferred to `github.com/SpoolSense`, org landing page created
