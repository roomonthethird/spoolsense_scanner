#pragma once

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "IPrinterStrategy.h"

enum class PrinterState {
    IDLE,
    TRACKING
};

class PrinterManager {
public:
    static PrinterManager& getInstance();

    void begin();
    void poll();
    void startPollingTask();
    void setStrategy(IPrinterStrategy* strategy);
    bool isConnected() const;
    PrinterState getState() const { return state_; }

private:
    PrinterManager() = default;
    PrinterManager(const PrinterManager&) = delete;
    PrinterManager& operator=(const PrinterManager&) = delete;

    static void pollingTaskFunc(void* param);

    void handleJobDetected(int jobId, float totalFilamentG);
    void resolveAndSendJobEnd(int jobId, float progressPercent, bool allowDeferred, const char* reason);
    void handleJobDisappeared();
    void checkFilamentMismatch();

    PrinterState state_ = PrinterState::IDLE;
    int currentJobId_ = -1;
    float currentJobTotalFilamentG_ = 0.0f;
    float lastProgressPercent_ = 0.0f;
    uint8_t missingJobPollCount_ = 0;
    bool mismatchWarned_ = false;

    IPrinterStrategy* strategy_ = nullptr;

    TaskHandle_t pollingTaskHandle_ = nullptr;
    static constexpr size_t POLLING_TASK_STACK = 6144;
    static constexpr UBaseType_t POLLING_TASK_PRIORITY = 1;
    static constexpr uint8_t JOB_MISSING_GRACE = 2;
};
