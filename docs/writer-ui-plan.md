# Tag Writer UI Redesign Plan

## Overview

Split the single-page tag writer into a multi-page web UI with shared navigation, served from the ESP32 at `spoolsense.local`.

## Pages

| Route | Purpose |
|-------|---------|
| `/` | Landing page — links to all tools |
| `/reader` | Read Tag — auto-detect format, display all data read-only |
| `/writer/openprinttag` | OpenPrintTag writer (existing form, refactored) |
| `/writer/tigertag` | TigerTag writer (new) |

## Static Assets

| Route | Purpose |
|-------|---------|
| `/css/shared.css` | Shared dark theme, nav, buttons, form layout |
| `/js/shared.js` | Shared utilities (API polling, color picker sync, validation) |

## API Endpoints

| Method | Route | Change |
|--------|-------|--------|
| GET | `/api/status` | Extended — add `tag_kind` field + TigerTag data |
| POST | `/api/write-tag` | Unchanged |
| POST | `/api/format-tag` | Unchanged |
| POST | `/api/write-tigertag` | New — write TigerTag binary to NTAG pages |

## Architecture Decisions

| Decision | Choice |
|----------|--------|
| Shared CSS/JS | Separate endpoints (`/css/shared.css`, `/js/shared.js`) |
| Status API | Extend `/api/status` with `tag_kind` field |
| TigerTag write | New `WRITE_TIGERTAG` type with 40-byte payload in union |
| TigerTag dropdowns | Fetch from TigerTag API (`api.tigertag.io`) + hardcoded fallback |
| Payload size | Add `tigertag_data[40]` to NFCWriteRequest union (7 byte increase) |

## File Plan

### New files
- `src/LandingHTML.h` — Landing page PROGMEM
- `src/ReaderHTML.h` — Tag reader page PROGMEM
- `src/TigerTagWriterHTML.h` — TigerTag writer page PROGMEM
- `src/SharedCSS.h` — Shared CSS PROGMEM
- `src/SharedJS.h` — Shared JS PROGMEM

### Modified files
- `src/TagWriterHTML.h` — CSS/JS extracted, nav bar added
- `src/WebServerManager.cpp/h` — New endpoints + TigerTag write handler
- `src/NFCWriteTypes.h` — `WRITE_TIGERTAG` + `tigertag_data[40]`
- `src/NFCManager.cpp` — `WRITE_TIGERTAG` handler
- `src/HardwareNFCConnection.cpp/h` — `writeISO14443Pages` method
- `src/NFCConnectionI.h` — `writeISO14443Pages` interface
- `test/native/StubNFCConnection.h` — stub `writeISO14443Pages`

## Design Notes

- Each page shows the official logo for its tag format (OpenPrintTag logo, TigerTag logo)
- The landing page shows the SpoolSense logo
- During writes, display a prominent warning: "Keep the tag still — do not remove until writing is complete"
- Status page shows a progress indicator with clear done/error states

## TigerTag Write Flow

```
User fills form → POST /api/write-tigertag
    ↓
WebServerManager parses JSON
    ↓
Assembles 40-byte TigerTag binary layout:
  bytes 0-3:   Version ID (0x5BF59264)
  bytes 4-7:   Product ID (0xFFFFFFFF = Maker)
  bytes 8-9:   Material ID (big-endian)
  byte 10:     Aspect 1 ID
  byte 11:     Aspect 2 ID
  byte 12:     Type ID (142 = Filament)
  byte 13:     Diameter ID (56 = 1.75mm)
  bytes 14-15: Brand ID (big-endian)
  bytes 16-19: Color RGBA
  bytes 20-22: Weight (big-endian 3 bytes)
  byte 23:     Unit ID (21 = grams)
  bytes 24-25: Nozzle Min (big-endian uint16)
  bytes 26-27: Nozzle Max (big-endian uint16)
  byte 28:     Dry Temp
  byte 29:     Dry Time (hours)
  byte 30:     Bed Min
  byte 31:     Bed Max
    ↓
Enqueues WRITE_TIGERTAG request with 40-byte payload
    ↓
NFCManager activates ISO14443A tag → writes pages 4-13
    ↓
UI polls /api/status to verify write
```

## TigerTag Dropdown Population

```
Page loads → fetch('https://api.tigertag.io/api:tigertag/brand/get/all')
    ↓
Success → populate dropdown from API data (always current)
    ↓
Failure → fall back to hardcoded common entries (offline capable)
```
