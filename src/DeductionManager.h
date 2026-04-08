#pragma once

#include "NFCTypes.h"

// DeductionManager — stores pending filament usage deductions in NVS and applies
// them to writable NFC tags on scan. Middleware sends deductions via MQTT after
// prints; scanner stores them persistently and writes to tag when next scanned.

class DeductionManager {
public:
    static DeductionManager& getInstance();

    // Accumulate a deduction for a UID. Adds to any existing pending amount.
    void storePending(const char* uid, float grams);

    // Apply pending deduction to tag if one exists. Enqueues the appropriate
    // write type and clears NVS on success. Returns grams deducted (0 if none).
    float applyIfPending(const char* uid, TagKind kind);

    // Read pending amount without clearing (diagnostics)
    float getPending(const char* uid);

    // Clear pending deduction after successful tag write
    void clearPending(const char* uid);

private:
    DeductionManager() = default;
    DeductionManager(const DeductionManager&) = delete;
    DeductionManager& operator=(const DeductionManager&) = delete;

    // Truncate UID to 15 chars for NVS key limit (16 including null terminator).
    // 7-byte UIDs (14 hex chars) fit fully. 8-byte UIDs (16 hex chars) lose last char.
    void makeNvsKey(const char* uid, char* out, size_t outSize);
};
