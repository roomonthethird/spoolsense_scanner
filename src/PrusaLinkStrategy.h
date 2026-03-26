#pragma once

#include "IPrinterStrategy.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class PrusaLinkStrategy : public IPrinterStrategy {
public:
    void setHttpMutex(SemaphoreHandle_t mutex) { httpMutex_ = mutex; }
    void update() override;

    bool hasActiveJob() const override { return hasJob_; }
    int getJobId() const override { return jobId_; }
    float getProgress() const override { return progress_; }
    float getTotalFilamentGrams() const override { return totalFilamentG_; }
    const char* getJobState() const override { return jobState_; }
    bool isConnected() const override { return connected_; }

    const char* getExpectedFilamentType() const override { return expectedFilamentType_; }
    float getExpectedNozzleTemp() const override { return expectedNozzleTemp_; }
    float getExpectedBedTemp() const override { return expectedBedTemp_; }

    int getToolCount() const override { return toolCount_; }
    float getFilamentForTool(int idx) const override {
        return (idx >= 0 && idx < toolCount_) ? filamentPerTool_[idx] : 0.0f;
    }
    const char* getFilamentTypeForTool(int idx) const override {
        return (idx >= 0 && idx < toolCount_) ? filamentTypePerTool_[idx] : "";
    }
    bool hasMMU() const override { return hasMMU_; }

    float fetchDeferredFilament(int expectedJobId) override;

private:
    bool fetchStatus();
    bool fetchJob();
    void fetchInfo();

    // Connection state
    bool connected_ = false;
    bool infoFetched_ = false;

    // Status fields
    bool hasJob_ = false;
    int jobId_ = -1;
    float progress_ = 0.0f;

    // Job fields
    float totalFilamentG_ = 0.0f;
    char jobState_[16] = {0};

    // Gcode metadata for pre-print validation
    char expectedFilamentType_[32] = {0};
    float expectedNozzleTemp_ = 0.0f;
    float expectedBedTemp_ = 0.0f;

    // Per-tool data for XL multi-head
    int toolCount_ = 0;
    float filamentPerTool_[MAX_TOOLS] = {0};
    char filamentTypePerTool_[MAX_TOOLS][16] = {{0}};

    // Printer info (fetched once)
    bool hasMMU_ = false;
    float nozzleDiameter_ = 0.0f;

    SemaphoreHandle_t httpMutex_ = nullptr;
};
