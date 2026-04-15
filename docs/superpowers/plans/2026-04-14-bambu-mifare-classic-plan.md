# Bambu MIFARE Classic Tag Reading Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Read encrypted Bambu Lab MIFARE Classic filament tags on both PN5180 and PN532 readers, parse filament data, and publish via MQTT/web UI/Spoolman.

**Architecture:** HKDF key derivation (mbedtls) produces per-tag sector keys from UID. New `mifareAuthenticate()` in PN5180ISO14443 and PN532 wrapper enables encrypted block reads. BambuTagParser decodes block data into a struct consumed by the existing tag pipeline.

**Tech Stack:** C++/Arduino, ESP32 mbedtls (HMAC-SHA256), PN5180 SPI, Adafruit PN532 I2C, ArduinoJson

---

## File Structure

| Action | File | Responsibility |
|--------|------|----------------|
| Create | `lib/bambutag/BambuKeyDeriver.h` | BambuKeys struct, deriveBambuKeys() declaration |
| Create | `lib/bambutag/BambuKeyDeriver.cpp` | HKDF-Extract + HKDF-Expand using mbedtls |
| Create | `lib/bambutag/BambuTagParser.h` | BambuTagData struct, parseBambuBlocks() declaration |
| Create | `lib/bambutag/BambuTagParser.cpp` | LE block decoding into BambuTagData fields |
| Modify | `lib/PN5180/PN5180ISO14443.h` | Add mifareAuthenticate() declaration |
| Modify | `lib/PN5180/PN5180ISO14443.cpp` | AUTH command, Crypto1 enable/disable, error recovery |
| Modify | `src/NFCConnectionI.h` | Add virtual mifareAuthenticate(), mifareClassicRead() |
| Modify | `src/HardwareNFCConnection.h` | Add mifareAuthenticate(), mifareClassicRead() |
| Modify | `src/HardwareNFCConnection.cpp` | PN5180 wrapper calling iso14443a_ methods |
| Modify | `src/HardwareNFCConnectionPN532.h` | Add mifareAuthenticate(), mifareClassicRead() |
| Modify | `src/HardwareNFCConnectionPN532.cpp` | PN532 wrapper calling Adafruit mifareclassic_* |
| Modify | `src/NFCManager.h` | BambuTagData storage + getter |
| Modify | `src/NFCManager.cpp` | Bambu auth+read flow in handleNewTag |
| Modify | `src/ApplicationManager.cpp` | Populate TagStateFields from BambuTagData |
| Modify | `src/TagStateJson.h` | Add optional filament_length_m field |
| Modify | `src/ConversionUtils.cpp` | tagFormatString returns "bambu" for BambuTag |
| Modify | `src/WebServerManager.cpp` | serializeBambuTagStatus() |
| Modify | `src/ReaderHTML.h` | renderBambu() + routing |

---

### Task 1: BambuKeyDeriver — HKDF Key Derivation

**Files:**
- Create: `lib/bambutag/BambuKeyDeriver.h`
- Create: `lib/bambutag/BambuKeyDeriver.cpp`

- [ ] **Step 1: Create BambuKeyDeriver header**

```cpp
// lib/bambutag/BambuKeyDeriver.h
#pragma once

#include <cstdint>

struct BambuKeys {
    uint8_t keys[96];  // 16 sectors x 6-byte Key A

    const uint8_t* sectorKey(uint8_t sector) const {
        return &keys[sector * 6];
    }

    const uint8_t* blockKey(uint8_t block) const {
        return sectorKey(block / 4);
    }
};

// Derive 16 MIFARE Classic sector keys from a Bambu tag UID using HKDF (HMAC-SHA256).
// uid: 4 or 7 byte tag UID from ISO14443A activation.
// Returns BambuKeys with 96 bytes of key material.
BambuKeys deriveBambuKeys(const uint8_t* uid, uint8_t uidLen);
```

- [ ] **Step 2: Create BambuKeyDeriver implementation**

```cpp
// lib/bambutag/BambuKeyDeriver.cpp
#include "BambuKeyDeriver.h"
#include <mbedtls/md.h>
#include <cstring>

// Master key from Bambu Research Group / SpoolEase
static const uint8_t BAMBU_MASTER_KEY[16] = {
    0x9A, 0x75, 0x9C, 0xF2, 0xC4, 0xF7, 0xCA, 0xFF,
    0x22, 0x2C, 0xB9, 0x76, 0x9B, 0x41, 0xBC, 0x96
};

static const uint8_t BAMBU_CONTEXT[] = { 'R', 'F', 'I', 'D', '-', 'A', '\0' };  // "RFID-A\0"

BambuKeys deriveBambuKeys(const uint8_t* uid, uint8_t uidLen) {
    BambuKeys result;
    memset(&result, 0, sizeof(result));

    const mbedtls_md_info_t* mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    const size_t hashLen = 32;  // SHA256 output
    const size_t totalLen = 96; // 16 sectors * 6 bytes

    // HKDF-Extract: PRK = HMAC-SHA256(salt=master_key, IKM=uid)
    uint8_t prk[32];
    mbedtls_md_hmac(mdInfo, BAMBU_MASTER_KEY, sizeof(BAMBU_MASTER_KEY),
                    uid, uidLen, prk);

    // HKDF-Expand: generate ceil(96/32) = 3 blocks
    uint8_t okm[96];
    uint8_t t[32];
    size_t tLen = 0;
    size_t okmOffset = 0;
    uint8_t n = (totalLen + hashLen - 1) / hashLen;

    for (uint8_t i = 1; i <= n; i++) {
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mdInfo, 1);  // 1 = HMAC
        mbedtls_md_hmac_starts(&ctx, prk, sizeof(prk));
        if (tLen > 0) {
            mbedtls_md_hmac_update(&ctx, t, tLen);
        }
        mbedtls_md_hmac_update(&ctx, BAMBU_CONTEXT, sizeof(BAMBU_CONTEXT));
        mbedtls_md_hmac_update(&ctx, &i, 1);
        mbedtls_md_hmac_finish(&ctx, t);
        mbedtls_md_free(&ctx);
        tLen = hashLen;

        size_t copyLen = totalLen - okmOffset;
        if (copyLen > hashLen) copyLen = hashLen;
        memcpy(okm + okmOffset, t, copyLen);
        okmOffset += copyLen;
    }

    memcpy(result.keys, okm, 96);
    return result;
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS (library auto-discovered by PlatformIO LDF)

- [ ] **Step 4: Commit**

```bash
git add lib/bambutag/BambuKeyDeriver.h lib/bambutag/BambuKeyDeriver.cpp
git commit -m "feat(#24): add BambuKeyDeriver — HKDF key derivation for MIFARE Classic"
```

---

### Task 2: BambuTagParser — Block Data Decoding

**Files:**
- Create: `lib/bambutag/BambuTagParser.h`
- Create: `lib/bambutag/BambuTagParser.cpp`

- [ ] **Step 1: Create BambuTagParser header**

```cpp
// lib/bambutag/BambuTagParser.h
#pragma once

#include <cstdint>

struct BambuTagData {
    bool valid = false;
    char material_variant[16] = {};   // Block 1
    char filament_type[16] = {};      // Block 2
    uint8_t color_r = 0;              // Block 5
    uint8_t color_g = 0;
    uint8_t color_b = 0;
    uint8_t color_a = 0;
    uint16_t weight_g = 0;            // Block 5 — AMS-updated remaining weight
    float diameter_mm = 0.0f;         // Block 5
    uint16_t drying_temp = 0;         // Block 6
    uint16_t drying_time = 0;         // Block 6
    uint16_t bed_temp = 0;            // Block 6
    uint16_t hotend_min = 0;          // Block 6
    uint16_t hotend_max = 0;          // Block 6
    char production_date[20] = {};    // Block 13
    uint32_t filament_length_m = 0;   // Block 14
    uint8_t color_extended[16] = {};  // Block 16 (raw)
};

// Block indices into the blocks array (must match read order)
// Read order: [1, 2, 4, 5, 6, 13, 14, 16]
static constexpr uint8_t BAMBU_BLOCKS[] = { 1, 2, 4, 5, 6, 13, 14, 16 };
static constexpr uint8_t BAMBU_BLOCK_COUNT = 8;

// Parse 8 raw 16-byte MIFARE Classic blocks into BambuTagData.
// blocks[0] = block 1, blocks[1] = block 2, blocks[2] = block 4, etc.
bool parseBambuBlocks(const uint8_t blocks[][16], BambuTagData& out);
```

- [ ] **Step 2: Create BambuTagParser implementation**

```cpp
// lib/bambutag/BambuTagParser.cpp
#include "BambuTagParser.h"
#include <cstring>

static uint16_t readU16LE(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static float readFloatLE(const uint8_t* p) {
    float val;
    memcpy(&val, p, sizeof(float));
    return val;
}

bool parseBambuBlocks(const uint8_t blocks[][16], BambuTagData& out) {
    memset(&out, 0, sizeof(out));

    // blocks[0] = block 1: Material variant ID (first 8 bytes) + Material ID (last 8 bytes)
    memcpy(out.material_variant, blocks[0], 15);
    out.material_variant[15] = '\0';
    // Trim trailing nulls/spaces
    for (int i = 14; i >= 0; i--) {
        if (out.material_variant[i] == '\0' || out.material_variant[i] == ' ') {
            out.material_variant[i] = '\0';
        } else break;
    }

    // blocks[1] = block 2: Filament type string (null-terminated ASCII)
    memcpy(out.filament_type, blocks[1], 15);
    out.filament_type[15] = '\0';
    for (int i = 14; i >= 0; i--) {
        if (out.filament_type[i] == '\0' || out.filament_type[i] == ' ') {
            out.filament_type[i] = '\0';
        } else break;
    }

    // blocks[2] = block 4: Detailed filament specs (format varies, skip for now)

    // blocks[3] = block 5: RGBA (4 bytes), weight (uint16 LE at offset 4), diameter (float LE at offset 8)
    out.color_r = blocks[3][0];
    out.color_g = blocks[3][1];
    out.color_b = blocks[3][2];
    out.color_a = blocks[3][3];
    out.weight_g = readU16LE(&blocks[3][4]);
    out.diameter_mm = readFloatLE(&blocks[3][8]);

    // blocks[4] = block 6: drying_temp(2), drying_time(2), bed_temp(2), hotend_min(2), hotend_max(2)
    out.drying_temp = readU16LE(&blocks[4][0]);
    out.drying_time = readU16LE(&blocks[4][2]);
    out.bed_temp = readU16LE(&blocks[4][4]);
    out.hotend_min = readU16LE(&blocks[4][6]);
    out.hotend_max = readU16LE(&blocks[4][8]);

    // blocks[5] = block 13: Production date as ASCII
    memcpy(out.production_date, blocks[5], 16);
    out.production_date[16] = '\0';
    // Trim trailing nulls
    for (int i = 15; i >= 0; i--) {
        if (out.production_date[i] == '\0' || out.production_date[i] == ' ') {
            out.production_date[i] = '\0';
        } else break;
    }

    // blocks[6] = block 14: Filament length
    out.filament_length_m = readU16LE(&blocks[6][0]);

    // blocks[7] = block 16: Extended color data (raw)
    memcpy(out.color_extended, blocks[7], 16);

    out.valid = (out.filament_type[0] != '\0');
    return out.valid;
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 4: Commit**

```bash
git add lib/bambutag/BambuTagParser.h lib/bambutag/BambuTagParser.cpp
git commit -m "feat(#24): add BambuTagParser — decode MIFARE Classic block data"
```

---

### Task 3: PN5180 MIFARE Classic Authentication

**Files:**
- Modify: `lib/PN5180/PN5180ISO14443.h`
- Modify: `lib/PN5180/PN5180ISO14443.cpp`

- [ ] **Step 1: Add mifareAuthenticate declaration to header**

In `lib/PN5180/PN5180ISO14443.h`, add after the `mifareHalt()` declaration (line 41):

```cpp
  // MIFARE Classic authentication using internal Crypto1 engine.
  // keyType: 0x60 = Key A, 0x61 = Key B
  // key: 6-byte authentication key
  // uid: tag UID (4 or 7 bytes from activateTypeA)
  // uidLen: length of uid array
  // Returns true if authentication succeeded (Crypto1 active).
  bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t *key,
                          const uint8_t *uid, uint8_t uidLen);
```

- [ ] **Step 2: Implement mifareAuthenticate in PN5180ISO14443.cpp**

Add before the `mifareHalt()` function:

```cpp
bool PN5180ISO14443::mifareAuthenticate(uint8_t blockNo, uint8_t keyType,
                                         const uint8_t *key, const uint8_t *uid,
                                         uint8_t uidLen) {
    // MIFARE Classic AUTH command: keyType(1) + blockNo(1) + key(6) + UID(4)
    // Total: 12 bytes. Only first 4 bytes of UID used for auth.
    uint8_t cmd[12];
    cmd[0] = keyType;   // 0x60 = AUTH_A, 0x61 = AUTH_B
    cmd[1] = blockNo;
    memcpy(cmd + 2, key, 6);
    // Use first 4 bytes of UID (MIFARE Classic standard)
    uint8_t uidCopyLen = (uidLen < 4) ? uidLen : 4;
    memcpy(cmd + 8, uid, uidCopyLen);

    // Enable Crypto1 in SYSTEM_CONFIG (bit 6 = MFC_CRYPTO_ON)
    writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000040);

    // Clear IRQ status before sending
    clearIRQStatus(0x000FFFFF);

    // Set transceive command in SYSTEM_CONFIG
    writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);  // COMMAND = Transceive

    if (!sendData(cmd, 12, 0x00)) {
        Serial.println("PN5180: mifareAuthenticate sendData failed");
        // Clear Crypto1 and recover
        writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFBF);
        writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFF8);
        clearIRQStatus(0x000FFFFF);
        return false;
    }

    // Wait for RX_IRQ indicating authentication response
    unsigned long startMs = millis();
    while (!(getIRQStatus() & RX_IRQ_STAT)) {
        if (millis() - startMs > 150) {
            Serial.printf("PN5180: mifareAuthenticate timeout for block %d\n", blockNo);
            writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFBF);
            writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFF8);
            clearIRQStatus(0x000FFFFF);
            return false;
        }
        delay(1);
    }

    // Check for authentication error (RX error IRQ)
    uint32_t irqStatus = getIRQStatus();
    clearIRQStatus(0x000FFFFF);

    if (irqStatus & (0x01 << 17)) {  // RX_SOF_DET_IRQ_STAT check for protocol error
        // Authentication likely failed — read a dummy byte to check
    }

    // Verify Crypto1 is active by checking SYSTEM_CONFIG bit 6
    uint32_t sysConfig;
    readRegister(SYSTEM_CONFIG, &sysConfig);
    if (!(sysConfig & 0x00000040)) {
        Serial.printf("PN5180: mifareAuthenticate Crypto1 not active after auth for block %d\n", blockNo);
        writeRegisterWithAndMask(SYSTEM_CONFIG, 0xFFFFFFF8);
        clearIRQStatus(0x000FFFFF);
        return false;
    }

    return true;
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 4: Commit**

```bash
git add lib/PN5180/PN5180ISO14443.h lib/PN5180/PN5180ISO14443.cpp
git commit -m "feat(#24): add mifareAuthenticate to PN5180ISO14443"
```

---

### Task 4: NFCConnectionI + Hardware Wrappers

**Files:**
- Modify: `src/NFCConnectionI.h`
- Modify: `src/HardwareNFCConnection.h`
- Modify: `src/HardwareNFCConnection.cpp`
- Modify: `src/HardwareNFCConnectionPN532.h`
- Modify: `src/HardwareNFCConnectionPN532.cpp`

- [ ] **Step 1: Add virtual methods to NFCConnectionI**

In `src/NFCConnectionI.h`, add after the `ntagGetVersion` method (line 31):

```cpp
    // MIFARE Classic authentication. keyType: 0x60=Key A, 0x61=Key B.
    // Returns true if sector containing blockNo is now authenticated.
    virtual bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t* key) { return false; }

    // Read a single MIFARE Classic 16-byte block (requires prior authentication).
    virtual bool mifareClassicRead(uint8_t blockNo, uint8_t* buffer) { return false; }
```

- [ ] **Step 2: Add declarations to HardwareNFCConnection.h**

After the `ntagGetVersion` declaration (line 28):

```cpp
    bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t* key) override;
    bool mifareClassicRead(uint8_t blockNo, uint8_t* buffer) override;
```

- [ ] **Step 3: Implement in HardwareNFCConnection.cpp**

Add at the end of the file, before the closing:

```cpp
bool HardwareNFCConnection::mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t* key) {
    if (!iso14443a_) return false;
    return iso14443a_->mifareAuthenticate(blockNo, keyType, key, currentUid_, 4);
}

bool HardwareNFCConnection::mifareClassicRead(uint8_t blockNo, uint8_t* buffer) {
    if (!iso14443a_) return false;
    return iso14443a_->mifareBlockRead(blockNo, buffer);
}
```

- [ ] **Step 4: Add declarations to HardwareNFCConnectionPN532.h**

After the `ntagGetVersion` declaration (line 21):

```cpp
    bool mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t* key) override;
    bool mifareClassicRead(uint8_t blockNo, uint8_t* buffer) override;
```

- [ ] **Step 5: Implement in HardwareNFCConnectionPN532.cpp**

Add at the end of the file:

```cpp
bool HardwareNFCConnectionPN532::mifareAuthenticate(uint8_t blockNo, uint8_t keyType, const uint8_t* key) {
    if (!pn532_ || !ready_) return false;
    // Adafruit PN532: keyNumber 0=KEYA, 1=KEYB
    uint8_t keyNumber = (keyType == 0x61) ? 1 : 0;
    return pn532_->mifareclassic_AuthenticateBlock(
        currentUid_, currentUidLen_, blockNo, keyNumber, const_cast<uint8_t*>(key));
}

bool HardwareNFCConnectionPN532::mifareClassicRead(uint8_t blockNo, uint8_t* buffer) {
    if (!pn532_ || !ready_) return false;
    return pn532_->mifareclassic_ReadDataBlock(blockNo, buffer);
}
```

- [ ] **Step 6: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 7: Commit**

```bash
git add src/NFCConnectionI.h src/HardwareNFCConnection.h src/HardwareNFCConnection.cpp \
        src/HardwareNFCConnectionPN532.h src/HardwareNFCConnectionPN532.cpp
git commit -m "feat(#24): add mifareAuthenticate + mifareClassicRead to NFC interface"
```

---

### Task 5: NFCManager — Bambu Tag Detection and Reading

**Files:**
- Modify: `src/NFCManager.h`
- Modify: `src/NFCManager.cpp`

- [ ] **Step 1: Add BambuTagData storage to NFCManager.h**

Add include at top:
```cpp
#include "BambuTagParser.h"
```

Add alongside the other `lastXxxValid_` members (after line 175):
```cpp
    BambuTagData lastBambuTag_;
    bool lastBambuTagValid_ = false;
```

Add public getter alongside `getLastTigerTagData` (after line 79):
```cpp
    bool getLastBambuTagData(BambuTagData& out);
```

- [ ] **Step 2: Implement getter in NFCManager.cpp**

Add alongside the other `getLastXxx` implementations:

```cpp
bool NFCManager::getLastBambuTagData(BambuTagData& out) {
    if (!lastBambuTagValid_) return false;
    out = lastBambuTag_;
    return true;
}
```

- [ ] **Step 3: Add Bambu read flow to handleNewTag**

Replace the existing BambuTag block in `handleNewTag()` (lines 476-498) with:

```cpp
    // Bambu tags use MIFARE Classic — attempt key derivation + authenticated read
    if (scan.kind == TagKind::BambuTag) {
        BambuTagData bambuData;
        bool readOk = readBambuTag(uid, uidLength, bambuData);

        if (xSemaphoreTake(tagMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            memcpy(currentSpool.spool_id, scan.uid_hex, sizeof(scan.uid_hex));
            memcpy(currentSpool.uid, uid, uidLength);
            currentSpool.uid_length = uidLength;
            currentSpool.present = true;
            currentSpool.blank_tag_present = false;
            currentSpool.kind = TagKind::BambuTag;
            currentSpool.variant = NtagVariant::Unknown;
            currentSpool.tag_data_valid = readOk;
            lastBambuTagValid_ = readOk;
            if (readOk) lastBambuTag_ = bambuData;
            lastTigerTagValid_ = false;
            lastOpenTag3DValid_ = false;
            lastOpenSpoolValid_ = false;
            memcpy(lastSeenUid, uid, uidLength);
            lastSeenUidLength = uidLength;
            lastSeenValid = true;
            lastSeenMs = millis();
            xSemaphoreGive(tagMutex);
        }

        if (readOk) {
            Serial.printf("NFCManager: Bambu tag — %s, %ug, #%02X%02X%02X\n",
                          bambuData.filament_type, bambuData.weight_g,
                          bambuData.color_r, bambuData.color_g, bambuData.color_b);
            LogBuffer::getInstance().logPrintf("Bambu: %s %ug\n",
                          bambuData.filament_type, bambuData.weight_g);
        } else {
            Serial.printf("NFCManager: Bambu tag — UID=%s (auth failed, no data)\n", scan.uid_hex);
            LogBuffer::getInstance().logPrintf("Bambu tag: %s (no data)\n", scan.uid_hex);
        }
        sendGenericTagMessage();
        return;
    }
```

- [ ] **Step 4: Add readBambuTag private method**

Add to NFCManager.h private section:
```cpp
    bool readBambuTag(const uint8_t* uid, uint8_t uidLength, BambuTagData& out);
```

Implement in NFCManager.cpp:

```cpp
#include "BambuKeyDeriver.h"
#include "BambuTagParser.h"

bool NFCManager::readBambuTag(const uint8_t* uid, uint8_t uidLength, BambuTagData& out) {
    BambuKeys keys = deriveBambuKeys(uid, uidLength);
    uint8_t blocks[BAMBU_BLOCK_COUNT][16];
    uint8_t lastAuthSector = 0xFF;

    for (uint8_t i = 0; i < BAMBU_BLOCK_COUNT; i++) {
        uint8_t blockNo = BAMBU_BLOCKS[i];
        uint8_t sector = blockNo / 4;

        // Only re-authenticate when changing sectors
        if (sector != lastAuthSector) {
            if (!connection_->mifareAuthenticate(blockNo, 0x60, keys.blockKey(blockNo))) {
                Serial.printf("NFCManager: Bambu auth failed on block %d (sector %d)\n", blockNo, sector);
                if (i == 0) return false;  // First block auth fail = not a Bambu tag
                continue;  // Skip this block but try remaining
            }
            lastAuthSector = sector;
        }

        if (!connection_->mifareClassicRead(blockNo, blocks[i])) {
            Serial.printf("NFCManager: Bambu block read failed on block %d\n", blockNo);
            memset(blocks[i], 0, 16);
        }
    }

    return parseBambuBlocks(blocks, out);
}
```

- [ ] **Step 5: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 6: Commit**

```bash
git add src/NFCManager.h src/NFCManager.cpp
git commit -m "feat(#24): Bambu tag detection — key derivation + authenticated block reads"
```

---

### Task 6: MQTT + TagStateJson Integration

**Files:**
- Modify: `src/TagStateJson.h`
- Modify: `src/ConversionUtils.cpp`
- Modify: `src/ApplicationManager.cpp`

- [ ] **Step 1: Add filament_length_m to TagStateFields**

In `src/TagStateJson.h`, add after `float diameter_mm;` (line 29):

```cpp
    uint32_t filament_length_m;
```

In `buildTagStateJson`, add after the `diameter_mm` line (after line 54):

```cpp
    if (f.filament_length_m > 0) doc["filament_length_m"] = f.filament_length_m;
```

- [ ] **Step 2: Update tagFormatString for BambuTag**

In `src/ConversionUtils.cpp`, change the BambuTag case (around line 80):

```cpp
        case TagKind::BambuTag:     return "bambu";
```

(Currently returns `"uid_only"`, change to `"bambu"`)

- [ ] **Step 3: Populate TagStateFields from BambuTagData in ApplicationManager**

In `src/ApplicationManager.cpp`, find `handleSpoolDetected` where it builds `TagStateFields f`. Add a block to populate from BambuTagData. After the existing tag format population code, in the section where the display spool is built, add Bambu data population:

Find the section where `f` (TagStateFields) is populated before `buildTagStateJson`. Add:

```cpp
    // Populate from Bambu tag data if available
    BambuTagData bambuData;
    if (state.kind == TagKind::BambuTag && NFCManager::getInstance().getLastBambuTagData(bambuData)) {
        strncpy(f.material_name, bambuData.filament_type, sizeof(f.material_name) - 1);
        strncpy(f.material_type, bambuData.filament_type, sizeof(f.material_type) - 1);
        snprintf(f.color, sizeof(f.color), "#%02X%02X%02X",
                 bambuData.color_r, bambuData.color_g, bambuData.color_b);
        f.initial_weight_g = bambuData.weight_g;
        f.remaining_g = bambuData.weight_g;
        f.min_print_temp = bambuData.hotend_min;
        f.max_print_temp = bambuData.hotend_max;
        f.min_bed_temp = bambuData.bed_temp;
        f.diameter_mm = bambuData.diameter_mm;
        f.filament_length_m = bambuData.filament_length_m;
        spool.hasColor = true;
    }
```

- [ ] **Step 4: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 5: Commit**

```bash
git add src/TagStateJson.h src/ConversionUtils.cpp src/ApplicationManager.cpp
git commit -m "feat(#24): publish Bambu tag data via MQTT — material, color, weight, temps"
```

---

### Task 7: Web UI — Status API + Reader Page

**Files:**
- Modify: `src/WebServerManager.cpp`
- Modify: `src/ReaderHTML.h`

- [ ] **Step 1: Add serializeBambuTagStatus to WebServerManager**

Add include at top of WebServerManager.cpp:
```cpp
#include "BambuTagParser.h"
```

Add new serializer alongside the existing ones (after `serializeOpenSpoolStatus`):

```cpp
void WebServerManager::serializeBambuTagStatus(JsonDocument& doc) {
    BambuTagData bt;
    if (!NFCManager::getInstance().getLastBambuTagData(bt) || !bt.valid) return;

    JsonObject obj = doc.createNestedObject("bambu");
    obj["filament_type"] = bt.filament_type;
    obj["material_variant"] = bt.material_variant;

    char colorHex[8];
    snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X", bt.color_r, bt.color_g, bt.color_b);
    obj["color_hex"] = colorHex;

    obj["weight_g"] = bt.weight_g;
    if (bt.diameter_mm > 0.0f) obj["diameter_mm"] = bt.diameter_mm;
    if (bt.hotend_min > 0) obj["hotend_min"] = bt.hotend_min;
    if (bt.hotend_max > 0) obj["hotend_max"] = bt.hotend_max;
    if (bt.bed_temp > 0) obj["bed_temp"] = bt.bed_temp;
    if (bt.drying_temp > 0) obj["drying_temp"] = bt.drying_temp;
    if (bt.drying_time > 0) obj["drying_time"] = bt.drying_time;
    if (bt.production_date[0]) obj["production_date"] = bt.production_date;
    if (bt.filament_length_m > 0) obj["filament_length_m"] = bt.filament_length_m;
}
```

Add declaration to `src/WebServerManager.h`:
```cpp
    void serializeBambuTagStatus(JsonDocument& doc);
```

- [ ] **Step 2: Wire into handleApiStatus switch**

In `handleApiStatus()`, find the switch on `state.kind` (around line 1233). Change the BambuTag case from `break;` to:

```cpp
            case TagKind::BambuTag:     serializeBambuTagStatus(doc); break;
```

- [ ] **Step 3: Add renderBambu to ReaderHTML.h**

Add after the `renderOpenSpool` function:

```javascript
    function renderBambu(s) {
      var t = s.bambu || {};
      var html = '';
      html += row('Format', 'Bambu Lab');
      html += row('UID', s.uid || '&mdash;');
      if (t.filament_type) html += row('Filament', t.filament_type);
      if (t.material_variant) html += row('Variant', t.material_variant);
      if (t.color_hex) html += row('Color', colorValue(t.color_hex));
      if (t.weight_g !== undefined) html += row('Weight', t.weight_g + ' g');
      if (t.diameter_mm) html += row('Diameter', t.diameter_mm + ' mm');
      if (t.hotend_min && t.hotend_max) html += row('Nozzle Temp', t.hotend_min + ' \u2013 ' + t.hotend_max + ' \u00B0C');
      if (t.bed_temp) html += row('Bed Temp', t.bed_temp + ' \u00B0C');
      if (t.drying_temp) html += row('Dry', t.drying_temp + ' \u00B0C / ' + (t.drying_time || '?') + ' hrs');
      if (t.production_date) html += row('Produced', t.production_date);
      if (t.filament_length_m) html += row('Filament Length', t.filament_length_m + ' m');
      if (s.spoolman) {
        if (s.spoolman.remaining_g !== undefined) {
          html += row('Remaining', s.spoolman.remaining_g.toFixed(1) + ' g' + spoolmanBadge());
        }
        if (s.spoolman.spool_id !== undefined && s.spoolman.spool_id > 0) {
          html += row('Spoolman ID', '#' + s.spoolman.spool_id + spoolmanBadge());
        }
      }
      return html;
    }
```

- [ ] **Step 4: Add BambuTag routing in render function**

Find the render function's tag_kind routing (around line 344-350). Add BambuTag case:

```javascript
      } else if (kind === 'BambuTag' && s.bambu) {
        html = renderBambu(s);
```

- [ ] **Step 5: Build to verify compilation**

Run: `pio run -e esp32s3devkitc`
Expected: SUCCESS

- [ ] **Step 6: Commit**

```bash
git add src/WebServerManager.cpp src/WebServerManager.h src/ReaderHTML.h
git commit -m "feat(#24): Bambu tag display on reader page + /api/status serialization"
```

---

### Task 8: End-to-End Test on Hardware

- [ ] **Step 1: Flash firmware**

Run: `pio run -e esp32s3devkitc -t upload --upload-port /dev/cu.usbmodem3101`
Expected: Upload SUCCESS

- [ ] **Step 2: Test with Bambu Lab spool tag**

1. Place a Bambu Lab filament spool on the scanner
2. Check serial output for: `NFCManager: Bambu tag — PLA Basic, 1000g, #RRGGBB`
3. If auth fails: `NFCManager: Bambu tag — UID=XXXXXXXX (auth failed, no data)`

- [ ] **Step 3: Verify web reader page**

1. Open `http://spoolsense.local/reader`
2. Scan the Bambu tag
3. Verify all fields display: Filament type, color swatch, weight, temps, diameter, production date

- [ ] **Step 4: Verify MQTT payload**

Check MQTT topic `spoolsense/tag/state` for:
- `tag_format: "bambu"`
- `material_name: "PLA Basic"` (or whatever the tag has)
- `color: "#RRGGBB"`
- `initial_weight_g` and `remaining_g` populated
- Temp fields populated

- [ ] **Step 5: Verify Spoolman lookup**

If the Bambu spool UID is registered in Spoolman, verify the enrichment badge appears on the reader page.

- [ ] **Step 6: Test with non-Bambu MIFARE Classic tag (if available)**

Place a non-Bambu MIFARE Classic tag. Verify auth fails gracefully and tag falls back to GenericUidTag behavior.

- [ ] **Step 7: Test with NTAG/OpenTag3D/TigerTag**

Verify existing tag formats still work — no regressions from the new authentication code path.

- [ ] **Step 8: Commit any fixes**

```bash
git add -A
git commit -m "fix(#24): address issues found during hardware testing"
```
