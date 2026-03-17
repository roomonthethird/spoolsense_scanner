# OpenTag3D Integration Architecture

## Overview

This document describes how OpenTag3D support would be added to SpoolSense Scanner. The goal is full read and write support — detection, parsing, MQTT/HA publishing, Spoolman sync, and web writer UI — following the same patterns already established for OpenPrintTag.

---

## What is OpenTag3D?

OpenTag3D is an open standard for 3D printer filament NFC tags (v1.000 at time of writing). Key differences from OpenPrintTag:

| | OpenPrintTag | OpenTag3D |
|--|--|--|
| Protocol | ISO15693 (SLIX2) | ISO14443A (NTAG213/215/216) |
| Encoding | CBOR wrapped in NDEF | Fixed binary offsets in NDEF |
| NDEF MIME type | `application/vnd.openprinttag` | `application/opentag3d` |
| Remaining weight | Yes (dynamic, updated on write) | No — weight fields are manufacture-time only |
| Temperature encoding | Raw °C | °C ÷ 5 (single byte) |
| Extended data | No | Yes — serial number, manufacture date, MFI, drying, volumetric speed |
| Online lookup | No | Optional URL field |

---

## Data Fields

### Core (144 bytes — NTAG213 minimum)

| Field | Offset | Size | Notes |
|-------|--------|------|-------|
| Tag Version | 0x00 | 2 | e.g. 1000 = v1.000 |
| Base Material Name | 0x02 | 5 | UTF-8, e.g. "ABS  " |
| Material Modifiers | 0x07 | 5 | UTF-8, optional |
| Filament Manufacturer | 0x1B | 16 | UTF-8 |
| Color Name | 0x2B | 32 | UTF-8, optional |
| Color 1 (RGBA) | 0x4B | 4 | sRGB bytes |
| Colors 2–4 (RGBA) | 0x50–0x5B | 4 each | Optional, transparent black if unused |
| Target Diameter | 0x5C | 2 | Micrometers (1750 = 1.75mm) |
| Target Weight | 0x5E | 2 | Grams (total spool) |
| Print Temperature | 0x60 | 1 | °C ÷ 5 |
| Bed Temperature | 0x61 | 1 | °C ÷ 5 |
| Density | 0x62 | 2 | µg/cm³ (1240000 = 1.240 g/cm³) |
| Transmission Distance | 0x64 | 2 | mm ÷ 0.1, optional |

### Extended (NTAG215/216 or SLIX2)

| Field | Offset | Size | Notes |
|-------|--------|------|-------|
| Online Data URL | 0x70 | 32 | ASCII, no `https://` prefix |
| Serial Number | 0x90 | 16 | UTF-8 |
| Manufacture Date | 0xA0 | 4 | 2B year, 1B month, 1B day |
| Manufacture Time | 0xA4 | 3 | Hour, minute, second UTC |
| Spool Core Diameter | 0xA7 | 1 | mm |
| MFI Temperature | 0xA8 | 1 | °C ÷ 5 |
| MFI Load | 0xA9 | 1 | g ÷ 10 |
| MFI Value | 0xAA | 1 | g/10min ÷ 10 |
| Measured Tolerance | 0xAB | 1 | Micrometers |
| Empty Spool Weight | 0xAC | 2 | Grams |
| Measured Filament Weight | 0xAE | 2 | Grams |
| Measured Filament Length | 0xB0 | 2 | Meters |
| Max Dry Temperature | 0xB2 | 1 | °C ÷ 5 |
| Dry Time | 0xB3 | 1 | Hours |
| Min/Max Print Temp | 0xB4–0xB5 | 1 each | °C ÷ 5 |
| Min/Max Bed Temp | 0xB6–0xB7 | 1 each | °C ÷ 5 |
| Min/Max/Target Volumetric Speed | 0xB8–0xBA | 1 each | mm³/s |

---

## Detection Flow

OpenTag3D tags use ISO14443A — the same protocol as NTAG215 UID-only tags. The difference is that OpenTag3D tags have a readable NDEF payload, while UID-only tags do not. The detection path must distinguish between them.

### Current flow (today):

```
scanLoop()
  ├── Try ISO15693 inventory
  │     └── Found → classifyTag() → OpenPrintTag or BlankTag
  └── Try ISO14443A activateTypeA()
        └── Found → GenericUidTag (UID only, no NDEF read)
```

### Proposed flow with OpenTag3D:

```
scanLoop()
  ├── Try ISO15693 inventory
  │     └── Found → classifyTag() → OpenPrintTag or BlankTag
  └── Try ISO14443A activateTypeA()
        └── Found → attempt NDEF read
              ├── NDEF MIME = "application/opentag3d" → OpenTag3D
              └── No NDEF or unrecognized MIME → GenericUidTag
```

`classifyTag()` in `NFCManager.cpp` handles the ISO14443A branch and currently always returns `GenericUidTag`. It would be extended to attempt an NDEF read and check the MIME type before falling back to `GenericUidTag`.

---

## New Library: `opentag3d_lib`

OpenTag3D's binary format is simple enough that the parser/encoder can be a small standalone library, mirroring `lib/openprinttag/openprinttag_lib.c`.

### Proposed location

```
lib/opentag3d/
  opentag3d_lib.h
  opentag3d_lib.c
```

### Version handling (required by spec)

The spec mandates specific version compatibility behavior. The library must return a version status so the caller can act on it:

```c
typedef enum {
    OT3D_OK = 0,
    OT3D_VERSION_WARNING,   // Minor version ahead of reader — warn, parse anyway
    OT3D_VERSION_ERROR,     // Major version ahead of reader — do not parse
    OT3D_PARSE_ERROR,
} opentag3d_result_t;

#define OT3D_SUPPORTED_VERSION 1000  // v1.000
```

`opentag3d_decode()` returns `OT3D_VERSION_WARNING` if the tag's major version matches but minor is ahead, and `OT3D_VERSION_ERROR` if the major version is newer than the reader supports. `ApplicationManager` must surface both states — warning via LCD/MQTT, error as a rejected scan.

### Core data struct

```c
typedef struct {
    uint16_t tag_version;
    char     base_material[6];       // 5 bytes + null
    char     material_modifiers[6];  // 5 bytes + null, optional
    char     manufacturer[17];       // 16 bytes + null
    char     color_name[33];         // 32 bytes + null, optional
    uint8_t  color_rgba[4];          // Primary color
    uint16_t diameter_um;            // Micrometers
    uint16_t target_weight_g;        // Grams
    uint8_t  print_temp_encoded;     // °C ÷ 5
    uint8_t  bed_temp_encoded;       // °C ÷ 5
    uint16_t density_ugcm3;          // µg/cm³
    // Extended fields (zero if not present):
    char     serial_number[17];
    uint16_t empty_spool_weight_g;
    uint16_t measured_filament_weight_g;
    uint16_t measured_filament_length_m;
    uint8_t  max_dry_temp_encoded;
    uint8_t  dry_time_hours;
} opentag3d_t;

// Decode from raw NDEF payload bytes
int opentag3d_decode(const uint8_t *payload, size_t len, opentag3d_t *out);

// Encode to raw bytes for writing
int opentag3d_encode(const opentag3d_t *tag, uint8_t *buf, size_t buflen);

// Helpers
static inline float opentag3d_print_temp(const opentag3d_t *t) { return t->print_temp_encoded * 5.0f; }
static inline float opentag3d_diameter_mm(const opentag3d_t *t) { return t->diameter_um / 1000.0f; }
static inline float opentag3d_density_gcc(const opentag3d_t *t) { return t->density_ugcm3 / 1000000.0f; }
```

The encode/decode functions simply read and write values at the fixed offsets defined in the spec. No CBOR needed — much simpler than `openprinttag_lib`.

---

## NFCManager Changes

### `CurrentSpoolState`

Add an `opentag3d_t` field alongside the existing `opt_tag_t`:

```cpp
struct CurrentSpoolState {
    // ... existing fields ...
    opt_tag_t  tag_data;           // OpenPrintTag payload (ISO15693)
    opentag3d_t ot3d_data;        // OpenTag3D payload (ISO14443A)
    bool tag_data_valid;
};
```

### New private methods

```cpp
bool readAndParseOpenTag3D(const uint8_t* uid, uint8_t uid_length);
bool writeOpenTag3DTag(const opentag3d_t& data);
void sendOpenTag3DDetectedMessage();
```

### Write queue

`NFCWriteTypes.h` already has a write type enum. Add:

```cpp
enum class NFCWriteType : uint8_t {
    // ... existing ...
    OpenTag3DWrite,
};
```

The write request payload would carry a serialized `opentag3d_t`. Since it is fixed binary (no heap CBOR allocation), it fits within the existing static write buffer.

---

## ApplicationManager / Events

Add a new application event parallel to `SpoolDetected`:

```cpp
// New event type
struct OpenTag3DDetected {
    char uid_hex[17];
    char manufacturer[17];
    char base_material[6];
    char color_name[33];
    uint8_t color_rgba[4];
    uint16_t target_weight_g;
    uint16_t measured_filament_weight_g;
    uint8_t  print_temp_c;    // already decoded (× 5)
    uint8_t  bed_temp_c;
    float    diameter_mm;
    float    density_gcc;
};
```

`ApplicationManager` would handle this event the same way it handles `SpoolDetected`:
- Publish to MQTT
- Trigger HA state update
- Trigger Spoolman sync (see below)
- Update LCD if enabled

---

## MQTT / Home Assistant

OpenTag3D tags would publish to a parallel topic:

```
spoolsense/<device_id>/opentag3d/state
```

HA discovery entities would mirror the OpenPrintTag set: material, manufacturer, color, weight, temperatures. The `measured_filament_weight_g` field is the closest equivalent to remaining weight, though it is set at manufacture time and not updated dynamically.

---

## Spoolman Sync

OpenTag3D has no remaining weight field — only `measured_filament_weight_g` (total at manufacture) and `empty_spool_weight_g`. For Spoolman sync:

- **Initial sync**: Use `measured_filament_weight_g` as `remaining_weight` on first detection (same as using full weight for a new spool).
- **Weight tracking**: Without a dynamic remaining weight field on the tag, weight deduction must be tracked in Spoolman only — the tag itself cannot be updated with consumed weight the way OpenPrintTag can. This is a fundamental limitation of the OpenTag3D spec.
- **UID as key**: Use the ISO14443A UID as `opentag3d_uuid` in the Spoolman extra field, same pattern as OpenPrintTag.

---

## Web Writer UI

The existing tag writer at `http://spoolsense.local` would gain an OpenTag3D tab or mode. Key differences from the OpenPrintTag form:

- No remaining weight field (write target weight instead)
- Temperature fields accept °C and encode as ÷ 5 internally
- Extended fields section for serial number, manufacture date, MFI, drying data, volumetric speed
- Must write to NTAG215/216 via ISO14443A — different REST endpoint from OpenPrintTag

New API endpoints:
```
POST /api/write-opentag3d     — write OpenTag3D payload to detected ISO14443A tag
POST /api/format-opentag3d    — format NDEF structure on a blank ISO14443A tag
```

---

## ISO14443A Write Path

Reading ISO14443A is already implemented via `PN5180ISO14443`. Writing NDEF to an NTAG215/216 requires:

1. `activateTypeA()` — already works
2. Write NDEF header to page 4 (NTAG page structure)
3. Write MIME type record (`application/opentag3d`)
4. Write binary payload at fixed offsets
5. Write NDEF terminator TLV

The NTAG215 uses 4-byte pages. The write path would use `mifareBlockWrite16()` for 16-byte aligned chunks where possible, falling back to single-page writes. This is the same challenge as OpenPrintTag block writes but with ISO14443A page semantics instead of ISO15693 block semantics.

---

## What's Already Done

- `TagKind::OpenTag3D` is reserved in `NFCTypes.h`
- `TagProtocol::ISO14443A` exists in `NFCTypes.h`
- ISO14443A hardware detection works (`PN5180ISO14443`)
- The scan loop already has the ISO14443A fallback branch
- `HardwareNFCConnection` exposes the ISO14443A connection

---

## Implementation Order

1. `lib/opentag3d/opentag3d_lib.h/.c` — parser and encoder
2. Native unit tests for encode/decode round-trip
3. Extend `classifyTag()` to attempt NDEF read on ISO14443A tags
4. `readAndParseOpenTag3D()` in `NFCManager`
5. `sendOpenTag3DDetectedMessage()` and `ApplicationManager` handler
6. MQTT publish and HA discovery for OpenTag3D
7. Spoolman sync for OpenTag3D
8. ISO14443A write path
9. Web writer UI — OpenTag3D form and endpoints

Steps 1–6 (read path) can be done and tested before the write path is started.
