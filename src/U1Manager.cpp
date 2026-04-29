// U1Manager.cpp — Snapmaker U1 direct-mode bridge implementation.
// All HTTP plumbing, JSON shaping, per-material defaults, and pending-augment
// state live here so ApplicationManager can stay focused on dispatch.

#include "U1Manager.h"

#ifndef NATIVE_TEST
  #include "ApplicationManager.h"  // SpoolDetectedPayload, SpoolmanSyncedPayload
  #include "NFCTypes.h"             // CurrentSpoolState
  #include "ConfigurationManager.h"
  #include <Arduino.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <ArduinoJson.h>
  #include <freertos/FreeRTOS.h>
  #include <freertos/semphr.h>
  #include <cctype>
  #include <cstring>
  #include <cstdio>
  #include <cstdlib>

extern SemaphoreHandle_t g_httpMutex;

namespace {

// Internal U1 wire-format struct — what we POST after all conversions.
struct U1FilamentInfo {
    char vendor[64] = {};
    char main_type[24] = {};   // uppercase, e.g. "PLA", "PETG"
    char sub_type[24] = {};    // e.g. "Matte", "CF", may be empty
    int  rgb_1 = -1;            // -1 = no color sent
    int  alpha = 255;
    int  hotend_min_temp = 0;   // 0 = not sent
    int  hotend_max_temp = 0;
    int  bed_temp = 0;
    uint8_t card_uid[8] = {};
    uint8_t card_uid_len = 0;
};

// Per-material defaults for OpenSpool (no bed temp on tag) and any tag that's
// missing temps. Matches common slicer defaults (Orca/PrusaSlicer/Cura).
// Only applied when MAIN_TYPE matches one of the firmware's recognized types.
struct MaterialDefaults {
    const char* main_type;  // uppercase
    int hotend_min;
    int hotend_max;
    int bed_temp;
};

constexpr MaterialDefaults DEFAULTS[] = {
    { "PLA",  200, 220, 60  },
    { "PETG", 230, 250, 70  },
    { "ABS",  240, 260, 100 },
    { "ASA",  240, 260, 100 },
    { "TPU",  220, 240, 50  },
    { "PVA",  190, 210, 60  },
    { "PC",   260, 290, 110 },
    { "PA",   250, 280, 90  },  // Nylon
};

const MaterialDefaults* findDefaults(const char* mainType) {
    if (!mainType || mainType[0] == '\0') return nullptr;
    for (const auto& d : DEFAULTS) {
        if (strcmp(d.main_type, mainType) == 0) return &d;
    }
    return nullptr;
}

// Split a material name into MAIN_TYPE (uppercase) + SUB_TYPE on first space
// or hyphen. Examples:
//   "PLA Matte"  -> "PLA"  + "Matte"
//   "PETG-CF"    -> "PETG" + "CF"
//   "PLA"        -> "PLA"  + ""
//   "PA-CF Pro"  -> "PA"   + "CF Pro"
void splitMaterialName(const char* src, char* main, size_t mainCap,
                                          char* sub, size_t subCap) {
    if (!src || src[0] == '\0') {
        if (main && mainCap) main[0] = '\0';
        if (sub && subCap) sub[0] = '\0';
        return;
    }
    size_t i = 0;
    while (src[i] && src[i] != ' ' && src[i] != '-' && i + 1 < mainCap) {
        main[i] = (char)std::toupper((unsigned char)src[i]);
        i++;
    }
    main[i] = '\0';
    // Skip the separator
    if (src[i] == ' ' || src[i] == '-') i++;
    if (src[i] && sub && subCap > 0) {
        strncpy(sub, src + i, subCap - 1);
        sub[subCap - 1] = '\0';
    } else if (sub && subCap) {
        sub[0] = '\0';
    }
}

// Pack the spool UID hex string ("04A1B2C3") into the byte array used by the
// U1's CARD_UID field. Stops at non-hex or buffer end.
void packCardUid(const char* hex, uint8_t* out, uint8_t cap, uint8_t& outLen) {
    outLen = 0;
    if (!hex) return;
    size_t hexLen = strlen(hex);
    for (size_t i = 0; i + 1 < hexLen && outLen < cap; i += 2) {
        char buf[3] = { hex[i], hex[i + 1], 0 };
        char* end = nullptr;
        long byte = strtol(buf, &end, 16);
        if (end != buf + 2) break;  // non-hex char
        out[outLen++] = (uint8_t)byte;
    }
}

// Convert RGBA bytes to RGB_1 integer (R<<16 | G<<8 | B). Returns -1 if all
// channels are zero AND alpha is also zero — caller treats as "no color".
int rgbaToRgb1(const uint8_t rgba[4]) {
    if (rgba[0] == 0 && rgba[1] == 0 && rgba[2] == 0 && rgba[3] == 0) return -1;
    return ((int)rgba[0] << 16) | ((int)rgba[1] << 8) | (int)rgba[2];
}

// Convert "#RRGGBB" or "RRGGBB" hex string to RGB_1 integer. Returns -1 on
// parse failure or empty input.
int hexColorToRgb1(const char* hex) {
    if (!hex || hex[0] == '\0') return -1;
    if (hex[0] == '#') hex++;
    unsigned int r = 0, g = 0, b = 0;
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) != 3) return -1;
    return (int)((r << 16) | (g << 8) | b);
}

// Apply firmware-recognized per-material defaults to any temp field that's
// still zero. Only fires if the MAIN_TYPE is one of the U1's protocol-mapped
// types — unrecognized types (PEEK, niche blends, etc.) leave zeros alone so
// users see the gap and can set explicit values in Spoolman or on the tag.
void applyMaterialDefaults(U1FilamentInfo& info) {
    const MaterialDefaults* d = findDefaults(info.main_type);
    if (!d) return;
    if (info.hotend_min_temp == 0) info.hotend_min_temp = d->hotend_min;
    if (info.hotend_max_temp == 0) info.hotend_max_temp = d->hotend_max;
    if (info.bed_temp == 0) info.bed_temp = d->bed_temp;
}

// Build U1 wire info from a smart-tag detection payload.
U1FilamentInfo buildFromDetection(const SpoolDetectedPayload& p) {
    U1FilamentInfo info;
    strncpy(info.vendor, p.manufacturer, sizeof(info.vendor) - 1);
    splitMaterialName(p.material_name, info.main_type, sizeof(info.main_type),
                                         info.sub_type, sizeof(info.sub_type));

    // TigerTag carries aspect (Silk/Wood/Matt) separately when material_name
    // is just the base type — promote aspect into SUB_TYPE if we don't have one.
    if (info.sub_type[0] == '\0' && p.aspect[0] != '\0') {
        strncpy(info.sub_type, p.aspect, sizeof(info.sub_type) - 1);
    }

    if (p.has_color) {
        info.rgb_1 = rgbaToRgb1(p.primary_color);
        info.alpha = p.primary_color[3] ? p.primary_color[3] : 255;
    }

    if (p.min_print_temp > 0) info.hotend_min_temp = p.min_print_temp;
    if (p.max_print_temp > 0) info.hotend_max_temp = p.max_print_temp;
    // U1 wants a single bed temp; prefer min, fall back to max — same convention
    // the firmware's own OpenSpool parser uses for OpenSpool tags.
    if (p.min_bed_temp > 0)      info.bed_temp = p.min_bed_temp;
    else if (p.max_bed_temp > 0) info.bed_temp = p.max_bed_temp;

    packCardUid(p.spool_id, info.card_uid, sizeof(info.card_uid), info.card_uid_len);
    applyMaterialDefaults(info);
    return info;
}

// Build U1 wire info from a Spoolman sync result (generic UID tag path) plus
// the current NFC state for fallback fields the sync may not have populated.
U1FilamentInfo buildFromSpoolmanSync(const SpoolmanSyncedPayload& s,
                                      const CurrentSpoolState& state) {
    U1FilamentInfo info;

    if (s.manufacturer[0] != '\0') {
        strncpy(info.vendor, s.manufacturer, sizeof(info.vendor) - 1);
    }
    splitMaterialName(s.material_name, info.main_type, sizeof(info.main_type),
                                         info.sub_type, sizeof(info.sub_type));

    if (s.color_hex[0] != '\0') {
        info.rgb_1 = hexColorToRgb1(s.color_hex);
        info.alpha = 255;
    }

    if (s.extruder_temp > 0) {
        info.hotend_min_temp = s.extruder_temp;
        info.hotend_max_temp = s.extruder_temp;  // single value -> use as both
    }
    if (s.bed_temp > 0) info.bed_temp = s.bed_temp;

    packCardUid(s.spool_id, info.card_uid, sizeof(info.card_uid), info.card_uid_len);

    // CurrentSpoolState fallback for any field Spoolman didn't return — covers
    // the rare case where a generic UID tag exists in Spoolman but with sparse
    // metadata (color or temps left blank in the Spoolman record).
    (void)state;  // reserved for future fallback logic when state fields land

    applyMaterialDefaults(info);
    return info;
}

// Returns true if every U1-required field has a meaningful value. Used to
// decide whether a pending-augment registration is worth keeping.
bool isComplete(const U1FilamentInfo& info) {
    return info.vendor[0] != '\0'
        && info.main_type[0] != '\0'
        && info.rgb_1 >= 0
        && info.hotend_min_temp > 0
        && info.hotend_max_temp > 0
        && info.bed_temp > 0;
}

// Serialize and POST. Returns the HTTP response code, -1000 if Moonraker URL
// unset, -1001 if the HTTP mutex was busy, or a negative HTTPC_ERROR_* code on
// transport failure. Channel/enabled gating is the caller's responsibility.
int postFilamentDetectSet(uint8_t channel, const U1FilamentInfo& info) {
    auto& cfg = ConfigurationManager::getInstance();
    const char* moonrakerUrl = cfg.getMoonrakerURL();
    if (!moonrakerUrl || moonrakerUrl[0] == '\0') return -1000;

    if (g_httpMutex && xSemaphoreTake(g_httpMutex, pdMS_TO_TICKS(250)) != pdTRUE) {
        return -1001;
    }

    StaticJsonDocument<512> body;
    body["channel"] = channel;
    JsonObject infoObj = body.createNestedObject("info");

    if (info.vendor[0] != '\0')   infoObj["VENDOR"] = info.vendor;
    if (info.main_type[0] != '\0') infoObj["MAIN_TYPE"] = info.main_type;
    if (info.sub_type[0] != '\0')  infoObj["SUB_TYPE"] = info.sub_type;
    if (info.rgb_1 >= 0) {
        infoObj["RGB_1"] = (long)info.rgb_1;
        infoObj["ALPHA"] = info.alpha;
    }
    if (info.hotend_min_temp > 0) infoObj["HOTEND_MIN_TEMP"] = info.hotend_min_temp;
    if (info.hotend_max_temp > 0) infoObj["HOTEND_MAX_TEMP"] = info.hotend_max_temp;
    if (info.bed_temp > 0)         infoObj["BED_TEMP"] = info.bed_temp;
    if (info.card_uid_len > 0) {
        JsonArray uidArr = infoObj.createNestedArray("CARD_UID");
        for (uint8_t i = 0; i < info.card_uid_len; i++) {
            uidArr.add((int)info.card_uid[i]);
        }
    }

    String payload;
    serializeJson(body, payload);

    char url[192];
    snprintf(url, sizeof(url), "%s/printer/filament_detect/set", moonrakerUrl);

    WiFiClient client;
    HTTPClient http;
    http.setConnectTimeout(1000);
    http.setTimeout(2000);
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    http.end();

    if (g_httpMutex) xSemaphoreGive(g_httpMutex);
    return code;
}

}  // namespace

U1Manager& U1Manager::getInstance() {
    static U1Manager instance;
    return instance;
}

void U1Manager::publishFromDetection(const SpoolDetectedPayload& payload) {
    auto& cfg = ConfigurationManager::getInstance();
    if (!cfg.isU1Enabled()) return;

    uint8_t channel = cfg.getU1Channel();
    if (channel > 3) return;  // belt-and-braces; loader already clamps

    uint32_t now = millis();
    if (moonrakerBackoffUntilMs_ != 0 && (int32_t)(now - moonrakerBackoffUntilMs_) < 0) {
        return;
    }

    U1FilamentInfo info = buildFromDetection(payload);
    int code = postFilamentDetectSet(channel, info);

    Serial.printf("U1Manager: publishFromDetection channel=%u uid=%s — HTTP %d\n",
                  (unsigned)channel, payload.spool_id, code);

    if (code == -1000 || code == -1001) {
        // Config or contention issue — neither warrants a backoff window
        return;
    }
    if (code < 0) {
        moonrakerBackoffUntilMs_ = millis() + MOONRAKER_BACKOFF_MS;
        Serial.printf("U1Manager: U1/Moonraker unreachable — backing off %u ms\n",
                      (unsigned)MOONRAKER_BACKOFF_MS);
        return;
    }
    moonrakerBackoffUntilMs_ = 0;

    // Register pending augment if POST 1 was incomplete and Spoolman is configured
    // (otherwise there's no way the augment could supply anything new).
    if (cfg.isSpoolmanEnabled() && !isComplete(info)) {
        pendingAugment_.active = true;
        strncpy(pendingAugment_.uid, payload.spool_id, sizeof(pendingAugment_.uid) - 1);
        pendingAugment_.uid[sizeof(pendingAugment_.uid) - 1] = '\0';
        pendingAugment_.expiresAtMs = millis() + PENDING_AUGMENT_TTL_MS;
        pendingAugment_.wantVendor       = (info.vendor[0] == '\0');
        pendingAugment_.wantMainType     = (info.main_type[0] == '\0');
        pendingAugment_.wantColor        = (info.rgb_1 < 0);
        pendingAugment_.wantHotendTemps  = (info.hotend_min_temp == 0 || info.hotend_max_temp == 0);
        pendingAugment_.wantBedTemp      = (info.bed_temp == 0);
    } else {
        pendingAugment_.active = false;
    }
}

void U1Manager::publishFromSpoolmanSync(const SpoolmanSyncedPayload& sync,
                                          const CurrentSpoolState& state) {
    auto& cfg = ConfigurationManager::getInstance();
    if (!cfg.isU1Enabled()) return;
    if (!sync.success || sync.spoolman_id <= 0) return;

    uint8_t channel = cfg.getU1Channel();
    if (channel > 3) return;

    uint32_t now = millis();
    if (moonrakerBackoffUntilMs_ != 0 && (int32_t)(now - moonrakerBackoffUntilMs_) < 0) {
        return;
    }

    if (sync.is_uid_lookup) {
        // Generic UID tag (NFC+) — Spoolman is the only data source. Single POST.
        U1FilamentInfo info = buildFromSpoolmanSync(sync, state);
        int code = postFilamentDetectSet(channel, info);
        Serial.printf("U1Manager: publishFromSpoolmanSync(UID) channel=%u spool=%d — HTTP %d\n",
                      (unsigned)channel, sync.spoolman_id, code);
        if (code < 0 && code != -1000 && code != -1001) {
            moonrakerBackoffUntilMs_ = millis() + MOONRAKER_BACKOFF_MS;
        } else if (code >= 0) {
            moonrakerBackoffUntilMs_ = 0;
        }
        return;
    }

    // Smart-tag follow-up sync — only POST if a pending augment is active for
    // this UID and Spoolman supplied at least one field that POST 1 was missing.
    if (!pendingAugment_.active) return;
    // Bound the compare to the UID buffer size so a non-null-terminated producer
    // (defensive — both buffers are 17 bytes / 16 hex chars + null) can't make
    // strcmp walk off the end.
    if (strncmp(pendingAugment_.uid, sync.spool_id, sizeof(pendingAugment_.uid) - 1) != 0) {
        pendingAugment_.active = false;
        return;
    }
    if ((int32_t)(now - pendingAugment_.expiresAtMs) >= 0) {
        pendingAugment_.active = false;
        return;
    }

    bool augments = false;
    if (pendingAugment_.wantVendor && sync.manufacturer[0] != '\0')        augments = true;
    if (pendingAugment_.wantColor && sync.color_hex[0] != '\0')             augments = true;
    if (pendingAugment_.wantHotendTemps && sync.extruder_temp > 0)          augments = true;
    if (pendingAugment_.wantBedTemp && sync.bed_temp > 0)                   augments = true;
    if (pendingAugment_.wantMainType && sync.material_name[0] != '\0')     augments = true;

    pendingAugment_.active = false;  // single-shot regardless of outcome
    if (!augments) return;

    U1FilamentInfo merged = buildFromSpoolmanSync(sync, state);
    int code = postFilamentDetectSet(channel, merged);
    Serial.printf("U1Manager: publishFromSpoolmanSync(augment) channel=%u spool=%d — HTTP %d\n",
                  (unsigned)channel, sync.spoolman_id, code);
    if (code < 0 && code != -1000 && code != -1001) {
        moonrakerBackoffUntilMs_ = millis() + MOONRAKER_BACKOFF_MS;
    } else if (code >= 0) {
        moonrakerBackoffUntilMs_ = 0;
    }
}

#else  // NATIVE_TEST

U1Manager& U1Manager::getInstance() {
    static U1Manager instance;
    return instance;
}
void U1Manager::publishFromDetection(const SpoolDetectedPayload&) {}
void U1Manager::publishFromSpoolmanSync(const SpoolmanSyncedPayload&,
                                          const CurrentSpoolState&) {}

#endif
