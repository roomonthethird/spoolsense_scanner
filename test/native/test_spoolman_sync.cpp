// Native tests for Spoolman spool lookup logic
// Tests parseSpoolIdByUuid with real Spoolman JSON responses
// Build with: cd test/native && make

#include "platform/NativePlatform.h"
#include <cstdio>
#include <cstring>
#include <cassert>

// Pull in ArduinoJson (available via PlatformIO deps)
// For native test, use the header-only version
#include <ArduinoJson.h>

// ─── Extract testable functions from SpoolmanManager.cpp ─────────────────────
// These are static functions in SpoolmanManager.cpp. We duplicate them here
// for isolated testing without pulling in HTTPClient/WiFi/FreeRTOS dependencies.

static bool matchesUuid(const char* storedUuid, const char* uuid) {
    if (storedUuid == nullptr || uuid == nullptr) {
        return false;
    }
    if (strcmp(storedUuid, uuid) == 0) {
        return true;
    }
    const size_t uuidLen = strlen(uuid);
    const size_t storedLen = strlen(storedUuid);
    if (storedLen != uuidLen + 2) {
        return false;
    }
    return storedUuid[0] == '"' &&
           strncmp(storedUuid + 1, uuid, uuidLen) == 0 &&
           storedUuid[uuidLen + 1] == '"' &&
           storedUuid[uuidLen + 2] == '\0';
}

static bool parseSpoolIdByUuid(const char* jsonText, const char* uuid, int& outId) {
    outId = -1;

    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, jsonText);
    if (err) {
        return false;
    }

    // Handle both array (list endpoint) and single object (by-id endpoint)
    JsonArray spools;
    if (doc.is<JsonArray>()) {
        spools = doc.as<JsonArray>();
    } else if (doc.is<JsonObject>()) {
        JsonObject spool = doc.as<JsonObject>();
        int id = spool["id"] | -1;
        const char* nfcId = spool["extra"]["nfc_id"] | "";
        if (id >= 0 && matchesUuid(nfcId, uuid)) {
            outId = id;
            return true;
        }
        return false;
    } else {
        return false;
    }

    for (JsonObject spool : spools) {
        int id = spool["id"] | -1;
        const char* nfcId = spool["extra"]["nfc_id"] | "";
        if (id >= 0 && matchesUuid(nfcId, uuid)) {
            outId = id;
            return true;
        }
    }
    return false;
}

// ─── Test fixtures ───────────────────────────────────────────────────────────

// Real Spoolman response: single spool with nested filament→vendor
static const char* FIXTURE_SINGLE_SPOOL = R"({
    "id": 31,
    "registered": "2026-03-18T20:46:01Z",
    "filament": {
        "id": 33,
        "registered": "2026-03-18T20:47:31Z",
        "name": "PLA",
        "vendor": {
            "id": 6,
            "registered": "2026-02-28T19:06:43Z",
            "name": "Generic",
            "empty_spool_weight": 1000.0,
            "extra": {}
        },
        "material": "PLA",
        "density": 1.24,
        "diameter": 1.75,
        "weight": 1000.0,
        "spool_weight": 1000.0,
        "color_hex": "EEFF00",
        "extra": {}
    },
    "remaining_weight": 998.0,
    "initial_weight": 1000.0,
    "used_weight": 2.0,
    "archived": false,
    "extra": {
        "nfc_id": "\"04ECA4AB8F6180\""
    }
})";

// Real Spoolman response: array of spools filtered by filament
static const char* FIXTURE_SPOOL_ARRAY = R"([
    {
        "id": 31,
        "registered": "2026-03-18T20:46:01Z",
        "filament": {
            "id": 33,
            "registered": "2026-03-18T20:47:31Z",
            "name": "PLA",
            "vendor": {
                "id": 6,
                "name": "Generic",
                "extra": {}
            },
            "material": "PLA",
            "density": 1.24,
            "diameter": 1.75,
            "extra": {}
        },
        "remaining_weight": 998.0,
        "archived": false,
        "extra": {
            "nfc_id": "\"04ECA4AB8F6180\""
        }
    }
])";

// Multiple spools in array — tests that the correct one is matched
static const char* FIXTURE_MULTIPLE_SPOOLS = R"([
    {
        "id": 29,
        "filament": {
            "id": 27,
            "vendor": { "id": 1, "name": "Sunlu", "extra": {} },
            "material": "ABS",
            "extra": {}
        },
        "remaining_weight": 600.0,
        "archived": false,
        "extra": {
            "nfc_id": "\"DAD4E374080104E0\""
        }
    },
    {
        "id": 31,
        "filament": {
            "id": 33,
            "vendor": { "id": 6, "name": "Generic", "extra": {} },
            "material": "PLA",
            "extra": {}
        },
        "remaining_weight": 998.0,
        "archived": false,
        "extra": {
            "nfc_id": "\"04ECA4AB8F6180\""
        }
    }
])";

// Spool with no nfc_id extra field
static const char* FIXTURE_NO_NFC_ID = R"([
    {
        "id": 20,
        "filament": {
            "id": 20,
            "vendor": { "id": 3, "name": "Bambu", "extra": {} },
            "material": "PLA",
            "extra": {}
        },
        "remaining_weight": 500.0,
        "archived": false,
        "extra": {}
    }
])";

// Spool with unquoted nfc_id (direct string, no escaped quotes)
static const char* FIXTURE_UNQUOTED_NFC_ID = R"([
    {
        "id": 42,
        "filament": {
            "id": 10,
            "vendor": { "id": 2, "name": "eSun", "extra": {} },
            "material": "PETG",
            "extra": {}
        },
        "remaining_weight": 750.0,
        "archived": false,
        "extra": {
            "nfc_id": "AABBCCDD11223344"
        }
    }
])";

// Old-style dashed UID format
static const char* FIXTURE_DASHED_UID = R"([
    {
        "id": 5,
        "filament": {
            "id": 5,
            "vendor": { "id": 1, "name": "Sunlu", "extra": {} },
            "material": "PLA",
            "extra": {}
        },
        "remaining_weight": 800.0,
        "archived": false,
        "extra": {
            "nfc_id": "\"04-BE-5F-A8-8F-61-80\""
        }
    }
])";

// ─── Tests ───────────────────────────────────────────────────────────────────

static int passed = 0, failed = 0;
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    name(); \
    printf("PASS\n"); \
    passed++; \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s at %s:%d\n", msg, __FILE__, __LINE__); \
        failed++; \
        return; \
    } \
} while(0)

void test_single_spool_lookup() {
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_SINGLE_SPOOL, "04ECA4AB8F6180", id);
    ASSERT(found, "should find spool");
    ASSERT(id == 31, "should return spool id 31");
}

void test_single_spool_wrong_uuid() {
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_SINGLE_SPOOL, "DEADBEEF12345678", id);
    ASSERT(!found, "should not find spool with wrong UUID");
    ASSERT(id == -1, "id should be -1");
}

void test_array_spool_lookup() {
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_SPOOL_ARRAY, "04ECA4AB8F6180", id);
    ASSERT(found, "should find spool in array");
    ASSERT(id == 31, "should return spool id 31");
}

void test_multiple_spools_finds_correct_one() {
    int id = -1;
    // Look for the TigerTag spool
    bool found = parseSpoolIdByUuid(FIXTURE_MULTIPLE_SPOOLS, "04ECA4AB8F6180", id);
    ASSERT(found, "should find TigerTag spool");
    ASSERT(id == 31, "should return id 31, not 29");

    // Look for the OpenPrintTag spool
    id = -1;
    found = parseSpoolIdByUuid(FIXTURE_MULTIPLE_SPOOLS, "DAD4E374080104E0", id);
    ASSERT(found, "should find OpenPrintTag spool");
    ASSERT(id == 29, "should return id 29");
}

void test_nested_ids_not_confused() {
    // The critical test: filament.id=33 and vendor.id=6 must NOT
    // be returned as the spool ID. Only the top-level id=31 is correct.
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_SINGLE_SPOOL, "04ECA4AB8F6180", id);
    ASSERT(found, "should find spool");
    ASSERT(id == 31, "should return spool id 31, not filament id 33 or vendor id 6");
    ASSERT(id != 33, "must not return filament id");
    ASSERT(id != 6, "must not return vendor id");
}

void test_no_nfc_id_field() {
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_NO_NFC_ID, "04ECA4AB8F6180", id);
    ASSERT(!found, "should not find spool without nfc_id");
}

void test_unquoted_nfc_id() {
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_UNQUOTED_NFC_ID, "AABBCCDD11223344", id);
    ASSERT(found, "should find spool with unquoted nfc_id");
    ASSERT(id == 42, "should return id 42");
}

void test_empty_array() {
    int id = -1;
    bool found = parseSpoolIdByUuid("[]", "04ECA4AB8F6180", id);
    ASSERT(!found, "should not find spool in empty array");
}

void test_invalid_json() {
    int id = -1;
    bool found = parseSpoolIdByUuid("not json at all", "04ECA4AB8F6180", id);
    ASSERT(!found, "should return false for invalid JSON");
}

void test_matches_uuid_direct() {
    ASSERT(matchesUuid("04ECA4AB8F6180", "04ECA4AB8F6180"), "exact match");
    ASSERT(matchesUuid("\"04ECA4AB8F6180\"", "04ECA4AB8F6180"), "quoted match");
    ASSERT(!matchesUuid("DEADBEEF", "04ECA4AB8F6180"), "different UUID");
    ASSERT(!matchesUuid("", "04ECA4AB8F6180"), "empty stored");
    ASSERT(!matchesUuid(nullptr, "04ECA4AB8F6180"), "null stored");
    ASSERT(!matchesUuid("04ECA4AB8F6180", nullptr), "null uuid");
}

void test_dashed_uid_no_match() {
    // Dashed UIDs from older spools should NOT match undashed scanner UIDs
    int id = -1;
    bool found = parseSpoolIdByUuid(FIXTURE_DASHED_UID, "04BE5FA88F6180", id);
    ASSERT(!found, "dashed UID should not match undashed (different format)");
}

int main() {
    printf("\nSpoolman Sync Tests\n");
    printf("================================\n\n");

    RUN_TEST(test_single_spool_lookup);
    RUN_TEST(test_single_spool_wrong_uuid);
    RUN_TEST(test_array_spool_lookup);
    RUN_TEST(test_multiple_spools_finds_correct_one);
    RUN_TEST(test_nested_ids_not_confused);
    RUN_TEST(test_no_nfc_id_field);
    RUN_TEST(test_unquoted_nfc_id);
    RUN_TEST(test_empty_array);
    RUN_TEST(test_invalid_json);
    RUN_TEST(test_matches_uuid_direct);
    RUN_TEST(test_dashed_uid_no_match);

    printf("\n=== Results: %d/%d passed ===\n\n", passed, passed + failed);
    return failed > 0 ? 1 : 0;
}
