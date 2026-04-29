#ifndef U1_MANAGER_H
#define U1_MANAGER_H

#include <cstdint>

// Forward declarations to keep this header lean — full definitions live in
// ApplicationManager.h (payload types) and NFCTypes.h (CurrentSpoolState).
struct SpoolDetectedPayload;
struct SpoolmanSyncedPayload;
struct CurrentSpoolState;

// U1Manager — Snapmaker U1 direct-mode bridge.
//
// Pushes scanner results to the U1's external filament-detection endpoint
// (paxx12 Extended Firmware with "Filament Detection: External" enabled).
// Reads u1_enabled / u1_channel / moonraker_url from ConfigurationManager
// each call; everything else is internal state.
//
// Two entry points cover the full integration:
//   publishFromDetection()    — smart tags (OpenSpool, OPT, TigerTag, OT3D, Bambu)
//   publishFromSpoolmanSync() — generic UID tags (NFC+) and smart-tag augment
//
// Both no-op when U1 integration is disabled in NVS, so callers can invoke
// unconditionally without gating.
//
// Threading: must be called only from the ApplicationManager dispatch loop
// (handleSpoolDetected / handleSpoolmanSynced). Internal state
// (moonrakerBackoffUntilMs_, pendingAugment_) is not mutex-protected — the
// dispatch loop is single-threaded so no synchronization is needed in the
// current design. Cross-thread invocation would require revisiting both
// fields, not just adding atomics around millis() math.
class U1Manager {
public:
    static U1Manager& getInstance();

    // Smart tag scan path. Builds U1 info from on-tag data + per-material
    // defaults, POSTs to /printer/filament_detect/set, and (if anything is
    // still missing and Spoolman is configured) registers a pending augment
    // for publishFromSpoolmanSync to fill in later.
    void publishFromDetection(const SpoolDetectedPayload& payload);

    // Spoolman sync result path.
    //  - For generic UID tags (is_uid_lookup): single POST using lookup result.
    //  - For smart tags (write update): if a pending augment was registered
    //    by publishFromDetection, merges Spoolman data over the prior POST
    //    and re-publishes; otherwise no-op.
    void publishFromSpoolmanSync(const SpoolmanSyncedPayload& sync,
                                  const CurrentSpoolState& state);

private:
    U1Manager() = default;
    U1Manager(const U1Manager&) = delete;
    U1Manager& operator=(const U1Manager&) = delete;

    // Reachability backoff — set after a transport failure so an offline U1
    // doesn't block the dispatch loop on every subsequent scan.
    uint32_t moonrakerBackoffUntilMs_ = 0;
    static constexpr uint32_t MOONRAKER_BACKOFF_MS = 30000;

    // Pending-augment tracking for smart tags that POSTed incomplete data and
    // are waiting on a Spoolman sync to fill missing fields.
    struct PendingAugment {
        bool active = false;
        char uid[17] = {};
        uint32_t expiresAtMs = 0;
        // What was missing in POST 1 — drives whether Spoolman data is worth a POST 2.
        bool wantVendor = false;
        bool wantMainType = false;
        bool wantColor = false;
        bool wantHotendTemps = false;
        bool wantBedTemp = false;
    };
    PendingAugment pendingAugment_ = {};
    static constexpr uint32_t PENDING_AUGMENT_TTL_MS = 30000;
};

#endif // U1_MANAGER_H
