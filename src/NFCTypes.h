#ifndef NFC_TYPES_H
#define NFC_TYPES_H

#include <cstdint>
#include <ctime>
#include "NFCWriteTypes.h"
#include "openprinttag_lib.h"

enum class TagProtocol : uint8_t {
    ISO15693,
    ISO14443A,
    Unknown
};

enum class TagKind : uint8_t {
    OpenPrintTag,   // ordinal 0 — memset to zero produces safe default
    GenericUidTag,  // UID-only tag (e.g. NTAG215) — ISO14443A
    OpenTag3D,      // OpenTag3D format — ISO14443A
    TigerTag,       // TigerTag format — ISO14443A (NTAG213/215/216)
    BlankTag,
    Unsupported
};

struct TagScanResult {
    TagProtocol protocol;
    TagKind kind;
    char uid_hex[17];   // null-terminated UID hex string (up to 8 bytes = 16 hex chars)
    bool present;
    bool tag_data_valid;
};

struct CurrentSpoolState {
    bool present;
    bool blank_tag_present;  // Deprecated: use kind == TagKind::BlankTag instead
    TagKind kind;
    char spool_id[17];
    uint8_t uid[8];              // ISO15693 uses 8-byte UID
    uint8_t uid_length;
    opt_tag_t tag_data;          // Cached openprinttag data
    bool tag_data_valid;
};

// Recent spool entry for history tracking (RAM only)
struct RecentSpoolEntry {
    char spool_id[17];
    uint8_t material_type;
    uint8_t color[4];            // RGBA
    char manufacturer[33];
    int grams_remaining;
    time_t last_seen;  // Unix timestamp (seconds)
    bool valid;
    bool synced_to_spoolman;
    int32_t spoolman_id;
};

#endif // NFC_TYPES_H
