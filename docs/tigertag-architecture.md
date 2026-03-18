# TigerTag Integration Architecture

## Overview

TigerTag is an open-source RFID tagging system for 3D printer filament spools using NTAG213 (ISO14443A, 144 bytes). Data is stored in a fixed binary layout at known byte offsets — no NDEF, no CBOR.

Spec: https://github.com/TigerTag-Project/TigerTag-RFID-Guide

## Tag Format

| Field | Bytes | Offset | Encoding |
|-------|-------|--------|----------|
| ID TigerTag (magic) | 4 | 0 | 0x5C15E2E4 = V1.0 Offline |
| ID Product | 4 | 4 | 32-bit; 0xFFFFFFFF = Maker |
| ID Material | 2 | 8 | Lookup table (e.g. 0x954B = PLA) |
| ID Diameter | 1 | 10 | 0x38 = 1.75mm, 0xDD = 2.85mm |
| ID Aspect 1 | 1 | 11 | Surface finish (Silk, Matt, etc.) |
| ID Aspect 2 | 1 | 12 | Surface finish |
| ID Type | 1 | 13 | 0x8E = Filament, 0xAD = Resin |
| ID Brand | 2 | 14 | Lookup table (e.g. 0xC5DC = Polymaker) |
| ID Unit | 1 | 16 | 0x15 = grams, 0x4F = liters |
| Color1 RGBA | 4 | 17 | Primary color |
| Color2 RGB | 3 | 21 | Secondary color |
| Color3 RGB | 3 | 24 | Tertiary color |
| Transmission Dist | 2 | 27 | HueForge parameter (value / 10) |
| Measure | 3 | 29 | Weight in grams |
| Nozzle Temp Min | 1 | 32 | Celsius |
| Nozzle Temp Max | 1 | 33 | Celsius |
| Dry Temp | 1 | 34 | Celsius |
| Dry Time | 1 | 35 | Hours |
| Bed Temp Min | 1 | 36 | Celsius |
| Bed Temp Max | 1 | 37 | Celsius |
| Timestamp | 4 | 38 | Seconds since 2000-01-01 UTC |
| Reserved | 12 | 42 | Future use |
| Emoji | 4 | 54 | UTF-8 encoded |
| Custom Message | 28 | 58 | ASCII/UTF-8 (max 28 chars) |
| Signature (ECDSA) | 64 | 86 | Pages 24-39; r(32) + s(32) |

All multi-byte values are big-endian.

## Detection Flow

```
detectTag()
  ↓
classifyTag(uid, uidLength)
  ├── uidLength == 8 → ISO15693 → OpenPrintTag path (unchanged)
  │
  └── uidLength ∈ {4,7} → ISO14443A
                             ↓
                      readISO14443Pages(4, 20)
                      Read 80 bytes (pages 4-23)
                             ↓
                      Check magic at offset 0
                      (0x5C15E2E4 = TigerTag V1.0)
                             ↓
                      ┌──────┴──────┐
                      ▼             ▼
                Magic match    No match
                      ↓             ↓
                Parse TigerTag  GenericUidTag
                binary layout   (existing path)
                      ↓
                Fill spool data
                (material, color,
                 weight, temps)
                      ↓
                sendSpoolDetectedMessage()
                (same path as OpenPrintTag)
```

## Files Changed

| File | Change |
|------|--------|
| `src/NFCTypes.h` | Add `TagKind::TigerTag` to enum |
| `src/NFCConnectionI.h` | Add `readISO14443Pages()` to interface |
| `src/HardwareNFCConnection.cpp/h` | Implement `readISO14443Pages()` using `mifareBlockRead` |
| `src/NFCManager.cpp` | After ISO14443A detection, read pages, check magic, route to parser |
| `src/TigerTagParser.cpp/h` | **New** — parse fixed byte layout, lookup tables for material/brand IDs |

## Lookup Tables

TigerTag uses 2-byte IDs for materials and brands that resolve via lookup tables. The full database is embedded in firmware as static const arrays:

- **Materials**: 81 entries (~2KB flash)
- **Brands**: 98 entries (~3KB flash)
- **Diameters**: 3 entries (negligible)
- **Aspects**: ~15 entries (~200 bytes)
- **Total**: ~5KB flash

Unknown IDs fall back to displaying the raw hex code (e.g. "Material: 0xABCD").

Source: https://github.com/TigerTag-Project/TigerTag-RFID-Guide/tree/main/database

A GitHub Actions workflow to auto-update the database from TigerTag's repo is a future TODO.

## Key Differences from OpenPrintTag

| | OpenPrintTag | TigerTag |
|---|---|---|
| Tag chip | ICODE SLIX2 (ISO15693) | NTAG213 (ISO14443A) |
| Memory | 320 bytes | 144 bytes |
| Encoding | NDEF + CBOR (flexible) | Raw bytes (fixed layout) |
| Tag cost | ~$0.50-1.00 | ~$0.10-0.20 |
| Weight tracking on tag | Yes (consumed_weight in aux) | No |
| Digital signature | No | ECDSA-P256 (64 bytes) |
| Lookup tables | Inline strings | External ID → name mapping |

## Spoolman Integration

TigerTag has no on-tag weight tracking field. The Spoolman sync flow:

1. Scanner reads TigerTag → gets material, brand, color, weight, temps
2. Spoolman sync creates vendor + filament + spool (same flow as OpenPrintTag)
3. Weight tracking happens entirely in Spoolman (not on the tag)
4. No `spoolman_id` written back to the tag (NTAG213 pages are writable but TigerTag format doesn't define a field for it)

## NOT in Scope (Initial Implementation)

- **TigerTag writer** — read-only first; writing deferred
- **ECDSA signature verification** — requires TigerTag public key; deferred
- **TigerTag API calls** — fully offline, no runtime API dependency
- **Auto-update pipeline for lookup tables** — deferred to TODO
