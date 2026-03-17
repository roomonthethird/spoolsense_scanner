# TODO

## Bugs

- **Spoolman 400 on spool create** — `openprinttag_uuid` in the extra field is double-escaped (`"\"DAD4E374080104E0\""` instead of `"DAD4E374080104E0"`); every new spool create fails with HTTP 400
- **setupRF() stuck after ISO15693 read** — after a successful multi-block read, `setupRF()` fails on the next scan loop iteration; currently patched with `lastSeenValid` clear to force a hardware reset, but the root cause (PN5180 RF state not cleanly restored after batched reads) is unresolved — side effect is repeated SpoolDetected events and Spoolman spam each cycle

- **Spoolman material mismatch** — serial log showed tag written as ABS but Spoolman matched it to TPU (filament id=3); likely a vendor/filament lookup bug or stale test data in Spoolman, needs investigation
- **Remaining legacy `openprinttag` naming** — several files still reference the old identity: BLE device name in `BluetoothManager.cpp`, HTML title in `docs/index.html`, project name in `CMakeLists.txt`, and `.code-workspace` filename

## Planned

### Tag Format Support
- **OpenTag3D** — support as an additional tag format (long-term, `TagKind::OpenTag3D` is reserved)

### PN5180 Library
- **`readData` buffer overload** — tueddy/hyutrn forks add `readData(int len, uint8_t *buffer)` which writes into a caller-provided buffer instead of heap-allocating; reduces heap churn on a memory-constrained device
- **`getInventoryMultiple()` research** — tueddy/hyutrn forks implement multi-tag inventory with 16-slot collision handling; investigate whether a scanner positioned between two spools could read both simultaneously; would need physical testing to determine if PN5180 RF field geometry supports it in practice
- **`isCardPresent()` for ISO14443** — tueddy/hyutrn forks add a simple boolean card-presence check for Type A tags; could simplify and clean up the generic tag detection path
- **Consider upgrading to hyutrn fork** — actively maintained (v2.3.7, Sept 2025), FreeRTOS-aware (reduced blocking delays), ~500 bytes smaller, fixes unknown manufacturer ID 0xFF; same API as tueddy so migration path is straightforward but requires constructor and call-site updates

### Performance
- **Instrument the write path** — add timing logs for each phase: format duration, number of blocks written, total write time, verify duration; output something like `format=320ms blocks=11 write=1840ms verify=410ms`; needed before any optimization to know where time is actually spent
- **Dirty-block write optimization** — investigate whether the current write path already skips unchanged blocks or writes all blocks every time; writing only dirty blocks is the biggest firmware-side win available without changing libraries
- **Blank vs. existing tag write time** — format step adds writes before the payload even starts; profile separately to understand if slow writes are blank-tag-only or affect rewrites too
- **Write Multiple Blocks investigation** — check whether the SLIX2 tags in use support ISO15693 Write Multiple Blocks (command 0x24); if so, batching block writes could reduce round-trips; note: many tags do not support this command so verify against tag datasheet first

### Firmware / Infrastructure
- **OTA firmware updates** — support over-the-air updates via WiFi so deployed scanners can be updated without USB reflash; ESP32 Arduino OTA is available
- **MQTT reconnect robustness** — audit whether `HomeAssistantManager` cleanly handles broker drops and reconnects in long-running deployments; verify subscriptions are re-established after reconnect
- **Configurable log verbosity** — add `LOG_LEVEL` define to `UserConfig.h` (e.g. DEBUG/INFO/WARN) to reduce serial noise in production without losing full output for debugging

### Web / UI
- **Status page** — add a landing page at `http://spoolsense.local/` showing current spool, WiFi signal, MQTT status, uptime, and free heap; makes the device debuggable without serial access
- **Web-based config** — add a protected config page at `spoolsense.local/config` to replace BLE-based configuration; allow WiFi/MQTT/Spoolman settings to be changed without reflashing
- **Unified installer** [P1] — a `spoolsense-installer` repo under the SpoolSense org; interactive CLI that covers both scanner and middleware: asks board type, WiFi, MQTT, Spoolman URL, toolhead mode, etc.; generates `UserConfig.h` for the scanner and the middleware config YAML; flashes firmware via esptool; validates connectivity end-to-end. Goal: new user runs one command and is fully operational without editing any files manually.

### Hardware / Build

- **Scanner naming** — configurable name (e.g. `Toolhead1-scanner`, `Lane1-scanner`) via `UserConfig.h`, reflected in BLE device name and MQTT topics

### Debugging / Logging
- **No serial output on tag write** — when the web UI triggers a write, nothing is logged to serial; add a write-dispatched log line to make debugging easier

### Integration
- **Spoolman write support** — write spool data fetched from Spoolman directly to a tag via BLE UI

### Ecosystem
- **Shared specs repo** — `spoolsense-specs` repo under the SpoolSense org documenting tag formats (OpenPrintTag, OpenTag3D, NTAG215 UID-only), MQTT payload schema, and REST API contract between scanner and middleware; becomes the source of truth both repos reference

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
- **LED pin in BoardPins.h** — pin auto-selected per board; `STATUS_LED_PIN` build flag removed
- **ENABLE_STATUS_LED unified** — `USE_STATUS_LED` build flag removed; `ENABLE_STATUS_LED` from `UserConfig.h` is now the single flag; `ApplicationManager.cpp` fixed to include `UserConfig.h` so LED calls compile in correctly
- **ESP32-S3 support** — `BoardPins.h` auto-selects pins per board; `platformio.ini` has `esp32s3zero` env; `ARDUINO_USB_CDC_ON_BOOT` enabled; LED type `NEO_GRB` for S3 onboard WS2812; hardware validated on S3-Zero with PN5180
