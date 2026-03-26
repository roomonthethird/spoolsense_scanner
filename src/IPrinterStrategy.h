#pragma once

#include <cstdint>

/// Abstract interface for printer link strategies (PrusaLink, OctoPrint, etc.)
/// Each implementation polls a specific printer API and exposes job state.
class IPrinterStrategy {
public:
    virtual ~IPrinterStrategy() = default;

    /// Poll the printer API and update internal state.
    /// Called each cycle by PrinterManager's FreeRTOS task.
    virtual void update() = 0;

    // --- Job status ---
    virtual bool hasActiveJob() const = 0;
    virtual int getJobId() const = 0;
    virtual float getProgress() const = 0;
    virtual float getTotalFilamentGrams() const = 0;

    /// Job state string: "PRINTING", "PAUSED", "FINISHED", "STOPPED", "ERROR", or ""
    virtual const char* getJobState() const = 0;

    virtual bool isConnected() const = 0;

    // --- Gcode metadata (available after job detected) ---
    virtual const char* getExpectedFilamentType() const { return ""; }
    virtual float getExpectedNozzleTemp() const { return 0.0f; }
    virtual float getExpectedBedTemp() const { return 0.0f; }

    // --- Per-tool data (XL multi-head, MMU) ---
    static constexpr int MAX_TOOLS = 5;
    virtual int getToolCount() const { return 0; }
    virtual float getFilamentForTool(int idx) const { (void)idx; return 0.0f; }
    virtual const char* getFilamentTypeForTool(int idx) const { (void)idx; return ""; }
    virtual bool hasMMU() const { return false; }

    /// Attempt to fetch filament data after print completes (bgcode fallback, etc.)
    virtual float fetchDeferredFilament(int expectedJobId) {
        (void)expectedJobId;
        return 0.0f;
    }
};
