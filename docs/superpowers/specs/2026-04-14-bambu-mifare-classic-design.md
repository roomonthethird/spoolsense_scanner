# Bambu MIFARE Classic Tag Reading — Design Spec

## Goal

Add full Bambu Lab MIFARE Classic tag reading to the SpoolSense scanner on both PN5180 and PN532 readers. Derive per-tag authentication keys from the UID using HKDF, read filament data blocks, and publish to MQTT/HA/Spoolman using the existing tag pipeline.

## Background

Bambu Lab filament spools use MIFARE Classic 1K tags with per-tag encryption keys derived from the UID via HKDF (HMAC-SHA256). The community (SpoolEase by yanshay, Bambu Research Group) has reverse-engineered the key derivation algorithm and block layout. The AMS writes updated weight data back to the tag during prints, making block 5's weight field a live value — not just factory initial weight.

**Scope**: Read-only. Bambu tags have an RSA-2048 signature (sectors 10-15) that prevents writing without Bambu's private key. No project in the community supports writing.

**Follow-up**: Issue #157 tracks Spoolman weight sync for mixed environments (user has both SpoolSense/Voron printers and a Bambu AMS — tag weight may change between scans).

## Key Derivation Algorithm

Ported from SpoolEase (`shared/src/pn532_ext.rs`):

- **Master key**: `9A 75 9C F2 C4 F7 CA FF 22 2C B9 76 9B 41 BC 96` (16 bytes)
- **HKDF-Extract**: `PRK = HMAC-SHA256(master_key, uid)`
- **HKDF-Expand**: Context = `"RFID-A\0"` (7 bytes), generate 96 bytes (16 sectors x 6-byte Key A)
- **Key B**: Always `00 00 00 00 00 00` (unused for reads)
- `sectorKey(n)` = bytes `[n*6 .. (n+1)*6]` from expanded output
- `blockKey(blockNo)` = `sectorKey(blockNo / 4)`

Uses ESP32's built-in mbedtls (`mbedtls_md_hmac()` with `MBEDTLS_MD_SHA256`) — no new dependency.

## Bambu Tag Block Layout

MIFARE Classic 1K = 16 sectors x 4 blocks = 64 blocks. Every 4th block is a sector trailer (keys + access bits). All data is little-endian.

### Blocks Read: `[1, 2, 4, 5, 6, 13, 14, 16]`

| Block | Sector | Content |
|-------|--------|---------|
| 1 | 0 | Material variant ID (8 bytes) + Material ID (8 bytes) |
| 2 | 0 | Filament type string (e.g. "PLA Basic", null-terminated ASCII) |
| 4 | 1 | Detailed filament specs |
| 5 | 1 | RGBA color (4 bytes), weight in grams (uint16 LE), diameter as float LE |
| 6 | 1 | Drying temp/time (uint16 LE), bed temp, hotend min/max |
| 13 | 3 | Production date as ASCII: `YYYY_MM_DD_HH_MM` |
| 14 | 3 | Filament length (factory static, never updated by AMS) |
| 16 | 4 | Extended color information |

**Block 5 weight is live-updated by the AMS** — represents current remaining weight, not factory initial. This is critical for mixed-environment users (#157).

## Architecture

### Layer 1: NFC Library — MIFARE Classic Authentication

**PN5180ISO14443** (in `lib/PN5180/`):

```cpp
bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t *key,
                        const uint8_t *uid, uint8_t uidLen);
```

- Sends AUTH command (0x60 = Key A, 0x61 = Key B) with block number, key, and UID
- PN5180 internal Crypto1 engine handles the challenge-response
- On success: sets `MFC_CRYPTO_ON` bit (bit 6) in `SYSTEM_CONFIG` register
- After auth, existing `mifareBlockRead()` reads encrypted blocks transparently
- Auth failure recovery: clear Crypto1 bit (`SYSTEM_CONFIG & 0xFFFFFFBF`), force transceiver idle (`SYSTEM_CONFIG & 0xFFFFFFF8`), flush RX, clear all IRQs

**HardwareNFCConnection** (PN5180 wrapper):

```cpp
bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t *key);
```

Passes through to PN5180 method using UID captured during `activateTypeA`.

**HardwareNFCConnectionPN532** (PN532 wrapper):

Same interface. Calls Adafruit PN532 library's `mifareclassic_AuthenticateBlock()`.

### Layer 2: Key Derivation — BambuKeyDeriver

New library: `lib/bambutag/`

```cpp
struct BambuKeys {
    uint8_t keys[96];  // 16 sectors x 6 bytes
    const uint8_t* sectorKey(uint8_t sector) const;
    const uint8_t* blockKey(uint8_t block) const;
};

BambuKeys deriveBambuKeys(const uint8_t* uid, uint8_t uidLen);
```

- Stateless, stack-allocated (96 bytes), no heap
- Uses `mbedtls_md_hmac()` from ESP32 framework

### Layer 3: Tag Data Parsing — BambuTagParser

New library: `lib/bambutag/`

```cpp
struct BambuTagData {
    bool valid;
    char material_variant[16];
    char filament_type[16];
    uint8_t color_r, color_g, color_b, color_a;
    uint16_t weight_g;
    float diameter_mm;
    uint16_t drying_temp;
    uint16_t drying_time;
    uint16_t bed_temp;
    uint16_t hotend_min;
    uint16_t hotend_max;
    char production_date[20];
    uint32_t filament_length_m;
    uint8_t color_extended[16];
};

bool parseBambuBlocks(const uint8_t blocks[8][16], BambuTagData& out);
```

- Takes 8 raw 16-byte block buffers, populates struct
- All uint16 fields decoded as little-endian
- No NFC dependency — purely data transformation, testable without hardware

### Layer 4: Integration

**NFCManager detection flow** (after `activateTypeA`):

1. Check SAK byte — `0x08` = MIFARE Classic 1K
2. Derive keys: `BambuKeys keys = deriveBambuKeys(uid, uidLen)`
3. For each block in `[1, 2, 4, 5, 6, 13, 14, 16]`:
   - `mifareAuthenticate(block, KEY_A, keys.blockKey(block))`
   - `mifareBlockRead(block, buffer)`
4. If first auth fails → not a Bambu tag → fall back to `GenericUidTag`
5. Parse blocks: `parseBambuBlocks(blocks, bambuData)`
6. Set `TagKind::BambuTag`, store `BambuTagData`, send `SPOOL_DETECTED`

**MQTT / Home Assistant** (`TagStateFields` mapping):

| TagStateFields | BambuTagData source |
|---|---|
| `material_name` | `filament_type` |
| `color` | RGBA → `#RRGGBB` |
| `initial_weight_g` | `weight_g` (AMS-updated) |
| `min_print_temp` | `hotend_min` |
| `max_print_temp` | `hotend_max` |
| `min_bed_temp` | `bed_temp` |
| `diameter_mm` | `diameter_mm` |
| `tag_format` | `"bambu"` |
| `filament_length_m` (new, optional) | `filament_length_m` |

**TFT Display**: "Bambu" colored label, material + color swatch.

**Web UI Reader Page**: `serializeBambuTagStatus()` adds `"bambu"` JSON object to `/api/status`. Reader page renders all fields.

**Spoolman**: UID used as `nfc_id` for lookup, same as `GenericUidTag`. Bambu tag data populates Spoolman fields on first scan if no spool exists.

## File Changes

| Action | File | Purpose |
|--------|------|---------|
| Create | `lib/bambutag/BambuKeyDeriver.h` | HKDF key derivation (header) |
| Create | `lib/bambutag/BambuKeyDeriver.cpp` | HKDF implementation using mbedtls |
| Create | `lib/bambutag/BambuTagParser.h` | `BambuTagData` struct + parser (header) |
| Create | `lib/bambutag/BambuTagParser.cpp` | Block parsing implementation |
| Modify | `lib/PN5180/PN5180ISO14443.h` | Add `mifareAuthenticate()` declaration |
| Modify | `lib/PN5180/PN5180ISO14443.cpp` | Implement MIFARE Classic auth + Crypto1 |
| Modify | `src/HardwareNFCConnection.h` | Add virtual `mifareAuthenticate()` |
| Modify | `src/HardwareNFCConnection.cpp` | PN5180 wrapper implementation |
| Modify | `src/HardwareNFCConnectionPN532.h` | Add `mifareAuthenticate()` |
| Modify | `src/HardwareNFCConnectionPN532.cpp` | PN532 wrapper via Adafruit lib |
| Modify | `src/NFCManager.h` | Store `BambuTagData`, add getter |
| Modify | `src/NFCManager.cpp` | Bambu detection flow in readAndParseTag |
| Modify | `src/ApplicationManager.cpp` | Populate TagStateFields from BambuTagData |
| Modify | `src/TagStateJson.h` | Add optional `filament_length_m` |
| Modify | `src/WebServerManager.cpp` | `serializeBambuTagStatus()` |
| Modify | `src/TFTManager.cpp` | Bambu tag label on TFT |
| Modify | `src/ReaderHTML.h` | Render Bambu data on reader page |

## Dependencies

- **mbedtls**: Ships with ESP32 Arduino framework. Used for HMAC-SHA256 in key derivation. No new library dependency.
- **Adafruit PN532**: Already in `platformio.ini`. Has `mifareclassic_AuthenticateBlock()` built in.

## Out of Scope

- Writing to Bambu tags (RSA-2048 signature prevents this)
- Spoolman weight sync from AMS-updated tags (tracked in #157)
- Bambu tag data on writer pages (read-only format, no writer needed)
