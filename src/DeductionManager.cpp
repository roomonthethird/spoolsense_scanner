// DeductionManager.cpp — Pending filament deduction storage (NVS) and tag write-back.
// Middleware sends usage deductions via MQTT after prints. Scanner stores them in NVS
// and applies to writable tags (OpenPrintTag, OpenTag3D) on the next scan.

#include "DeductionManager.h"

#ifndef NATIVE_TEST
  #include <Preferences.h>
  #include <Arduino.h>
  #include "NFCManager.h"
  #include "opentag3d_lib.h"
#endif

DeductionManager& DeductionManager::getInstance() {
    static DeductionManager instance;
    return instance;
}

void DeductionManager::makeNvsKey(const char* uid, char* out, size_t outSize) {
    // NVS key limit: 15 usable chars (16 including null terminator)
    size_t maxLen = (outSize - 1 < 15) ? outSize - 1 : 15;
    strncpy(out, uid, maxLen);
    out[maxLen] = '\0';
}

void DeductionManager::storePending(const char* uid, float grams) {
#ifndef NATIVE_TEST
    char key[16];
    makeNvsKey(uid, key, sizeof(key));

    Preferences prefs;
    prefs.begin("deductions", false);  // RW mode
    float current = prefs.getFloat(key, 0.0f);
    float total = current + grams;
    prefs.putFloat(key, total);
    prefs.end();

    Serial.printf("DeductionManager: Stored %.1fg pending for %s (total: %.1fg)\n", grams, uid, total);
#endif
}

float DeductionManager::getPending(const char* uid) {
#ifndef NATIVE_TEST
    char key[16];
    makeNvsKey(uid, key, sizeof(key));

    Preferences prefs;
    prefs.begin("deductions", true);  // RO mode
    float value = prefs.getFloat(key, 0.0f);
    prefs.end();
    return value;
#else
    return 0.0f;
#endif
}

void DeductionManager::clearPending(const char* uid) {
#ifndef NATIVE_TEST
    char key[16];
    makeNvsKey(uid, key, sizeof(key));

    Preferences prefs;
    prefs.begin("deductions", false);
    prefs.remove(key);
    prefs.end();

    Serial.printf("DeductionManager: Cleared pending deduction for %s\n", uid);
#endif
}

// ── Apply logic ─────────────────────────────────────────────

static float applyOpenPrintTag(const char* uid, float pending) {
#ifndef NATIVE_TEST
    CurrentSpoolState state;
    if (!NFCManager::getInstance().getCurrentSpoolState(state)) return 0.0f;
    if (!state.tag_data_valid) return 0.0f;

    float fullWeight = 0.0f, consumed = 0.0f;
    opt_get_actual_full_weight(&state.tag_data, &fullWeight);
    opt_get_consumed_weight(&state.tag_data, &consumed);
    float remaining = fullWeight - consumed;
    if (remaining < 0) remaining = 0;

    // Clamp: don't deduct more than what's on the tag
    float deduction = (pending > remaining) ? remaining : pending;

    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.request_id = NFCManager::getInstance().generateRequestId();
    req.type = NFCWriteType::REMOVE_WEIGHT;
    req.data.grams_to_remove = deduction;
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);

    NFCManager::getInstance().enqueueWrite(req);

    Serial.printf("DeductionManager: Applied %.1fg deduction to OpenPrintTag %s (%.1fg remaining)\n",
                  deduction, uid, remaining - deduction);
    return deduction;
#else
    return 0.0f;
#endif
}

static float applyOpenTag3D(const char* uid, float pending) {
#ifndef NATIVE_TEST
    opentag3d_t ot3d;
    if (!NFCManager::getInstance().getLastOpenTag3DData(ot3d)) {
        Serial.printf("DeductionManager: No cached OpenTag3D data for %s\n", uid);
        return 0.0f;
    }

    // Use measured weight if available, otherwise target weight
    float remaining = (ot3d.measured_filament_weight_g > 0)
                      ? ot3d.measured_filament_weight_g
                      : (float)ot3d.target_weight_g;

    float deduction = (pending > remaining) ? remaining : pending;

    // Subtract from the weight field the tag uses
    if (ot3d.measured_filament_weight_g > 0) {
        ot3d.measured_filament_weight_g -= (uint16_t)deduction;
    } else {
        int newWeight = (int)ot3d.target_weight_g - (int)deduction;
        ot3d.target_weight_g = (newWeight > 0) ? (uint16_t)newWeight : 0;
    }

    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.request_id = NFCManager::getInstance().generateRequestId();
    req.type = NFCWriteType::WRITE_OPENTAG3D;
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);

    NFCManager::getInstance().enqueueRawWrite(req, (const uint8_t*)&ot3d, sizeof(ot3d));

    Serial.printf("DeductionManager: Applied %.1fg deduction to OpenTag3D %s (%.1fg remaining)\n",
                  deduction, uid, remaining - deduction);
    return deduction;
#else
    return 0.0f;
#endif
}

float DeductionManager::applyIfPending(const char* uid, TagKind kind) {
    float pending = getPending(uid);
    if (pending <= 0.0f) return 0.0f;

    float deducted = 0.0f;
    switch (kind) {
        case TagKind::OpenPrintTag:
            deducted = applyOpenPrintTag(uid, pending);
            break;
        case TagKind::OpenTag3D:
            deducted = applyOpenTag3D(uid, pending);
            break;
        default:
            // Tag can't accept weight writes — clear stale deduction
            Serial.printf("DeductionManager: Tag type %d does not support weight writes — clearing\n", (int)kind);
            break;
    }

    clearPending(uid);
    return deducted;
}
