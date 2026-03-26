#include "PrinterManager.h"
#include "ConfigurationManager.h"
#include "ApplicationManager.h"
#include "NFCManager.h"
#include "NFCTypes.h"
#include <Arduino.h>
#include <cstring>

extern "C" {
#include "openprinttag_lib.h"
}

PrinterManager& PrinterManager::getInstance() {
    static PrinterManager instance;
    return instance;
}

void PrinterManager::begin() {
    state_ = PrinterState::IDLE;
    currentJobId_ = -1;
    currentJobTotalFilamentG_ = 0.0f;
    lastProgressPercent_ = 0.0f;
    missingJobPollCount_ = 0;
    mismatchWarned_ = false;
    Serial.println("PrinterManager: Initialized");
}

void PrinterManager::startPollingTask() {
    if (pollingTaskHandle_) {
        Serial.println("PrinterManager: Polling task already running");
        return;
    }

    xTaskCreate(
        pollingTaskFunc,
        "PrinterPoll",
        POLLING_TASK_STACK,
        this,
        POLLING_TASK_PRIORITY,
        &pollingTaskHandle_
    );
    Serial.println("PrinterManager: Polling task started");
}

void PrinterManager::pollingTaskFunc(void* param) {
    auto* self = static_cast<PrinterManager*>(param);
    auto& config = ConfigurationManager::getInstance();

    while (true) {
        self->poll();
        vTaskDelay(pdMS_TO_TICKS(config.getPollIntervalMs()));
    }
}

void PrinterManager::setStrategy(IPrinterStrategy* strat) {
    strategy_ = strat;
}

bool PrinterManager::isConnected() const {
    return strategy_ && strategy_->isConnected();
}

// ---------------------------------------------------------------------------
// poll() — main state machine, called each cycle
// ---------------------------------------------------------------------------

void PrinterManager::poll() {
    if (!strategy_) return;

    strategy_->update();

    if (!strategy_->isConnected()) {
        if (state_ == PrinterState::TRACKING) {
            missingJobPollCount_++;
            if (missingJobPollCount_ >= JOB_MISSING_GRACE) {
                Serial.printf("PrinterManager: Job %d lost after disconnect\n", currentJobId_);
                handleJobDisappeared();
            }
        }
        return;
    }

    if (!strategy_->hasActiveJob()) {
        if (state_ == PrinterState::TRACKING) {
            missingJobPollCount_++;
            if (missingJobPollCount_ >= JOB_MISSING_GRACE) {
                Serial.printf("PrinterManager: Job %d missing from API\n", currentJobId_);
                handleJobDisappeared();
            }
        }
        return;
    }

    missingJobPollCount_ = 0;

    int jobId = strategy_->getJobId();
    float progress = strategy_->getProgress();
    float totalFilamentG = strategy_->getTotalFilamentGrams();
    const char* jobState = strategy_->getJobState();

    if (state_ == PrinterState::IDLE) {
        if (strcmp(jobState, "PRINTING") == 0 || strcmp(jobState, "PAUSED") == 0) {
            handleJobDetected(jobId, totalFilamentG);
        }
    } else if (state_ == PrinterState::TRACKING) {
        // Job replaced?
        if (jobId != currentJobId_) {
            resolveAndSendJobEnd(currentJobId_, lastProgressPercent_, false, "job_replaced");
            handleJobDetected(jobId, totalFilamentG);
            return;
        }

        lastProgressPercent_ = progress;

        if (totalFilamentG > 0) {
            currentJobTotalFilamentG_ = totalFilamentG;
        }

        if (strcmp(jobState, "FINISHED") == 0) {
            resolveAndSendJobEnd(jobId, 100.0f, true, "finished");
        } else if (strcmp(jobState, "STOPPED") == 0 || strcmp(jobState, "ERROR") == 0) {
            resolveAndSendJobEnd(jobId, progress, true, "stopped_or_error");
        }
    }
}

// ---------------------------------------------------------------------------
// handleJobDetected — IDLE → TRACKING
// ---------------------------------------------------------------------------

void PrinterManager::handleJobDetected(int jobId, float totalFilamentG) {
    state_ = PrinterState::TRACKING;
    currentJobId_ = jobId;
    currentJobTotalFilamentG_ = totalFilamentG;
    lastProgressPercent_ = 0.0f;
    missingJobPollCount_ = 0;
    mismatchWarned_ = false;

    AppMessage msg;
    msg.type = AppMessageType::PRINT_STARTED;
    msg.payload.printStarted.job_id = jobId;
    ApplicationManager::getInstance().sendMessage(msg);

    Serial.printf("PrinterManager: Tracking job %d (filament: %.2fg)\n", jobId, totalFilamentG);

    if (totalFilamentG <= 0) {
        Serial.printf("PrinterManager: WARNING — no filament metadata for job %d\n", jobId);
    }

    // Check filament match on first detection
    checkFilamentMismatch();
}

// ---------------------------------------------------------------------------
// resolveAndSendJobEnd — TRACKING → IDLE
// ---------------------------------------------------------------------------

void PrinterManager::resolveAndSendJobEnd(int jobId, float progressPercent,
                                           bool allowDeferred, const char* reason) {
    bool canceled = (progressPercent < 100.0f);

    Serial.printf("PrinterManager: Resolving job %d reason=%s progress=%.1f%% filament=%.2fg\n",
                  jobId, reason, progressPercent, currentJobTotalFilamentG_);

    // Try deferred filament lookup if we have no data
    if (currentJobTotalFilamentG_ <= 0.0f && strategy_ && allowDeferred) {
        float deferred = strategy_->fetchDeferredFilament(jobId);
        if (deferred > 0.0f) {
            currentJobTotalFilamentG_ = deferred;
            Serial.printf("PrinterManager: Deferred filament for job %d: %.2fg\n", jobId, deferred);
        }
    }

    float filamentUsed = (progressPercent / 100.0f) * currentJobTotalFilamentG_;

    AppMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = AppMessageType::PRINT_ENDED;
    msg.payload.printEnded.job_id = jobId;
    msg.payload.printEnded.filament_used_grams = filamentUsed;
    msg.payload.printEnded.canceled = canceled;

    // Populate per-tool data (XL multi-head)
    msg.payload.printEnded.tool_count = 0;
    if (strategy_ && strategy_->getToolCount() > 0) {
        int tc = strategy_->getToolCount();
        if (tc > 5) tc = 5;
        msg.payload.printEnded.tool_count = tc;
        for (int i = 0; i < tc; i++) {
            msg.payload.printEnded.filament_per_tool[i] =
                (progressPercent / 100.0f) * strategy_->getFilamentForTool(i);
        }
    }

    ApplicationManager::getInstance().sendMessage(msg);

    Serial.printf("PrinterManager: Job %d %s — %.2fg used\n",
                  jobId, canceled ? "canceled" : "finished", filamentUsed);

    state_ = PrinterState::IDLE;
    currentJobId_ = -1;
    currentJobTotalFilamentG_ = 0.0f;
    lastProgressPercent_ = 0.0f;
    missingJobPollCount_ = 0;
    mismatchWarned_ = false;
}

void PrinterManager::handleJobDisappeared() {
    float progress = (lastProgressPercent_ >= 95.0f) ? 100.0f : lastProgressPercent_;
    Serial.printf("PrinterManager: Job %d disappeared at %.1f%% — treating as %s\n",
                  currentJobId_, lastProgressPercent_,
                  progress >= 100.0f ? "finished" : "canceled");
    resolveAndSendJobEnd(currentJobId_, progress, true, "job_disappeared");
}

// ---------------------------------------------------------------------------
// checkFilamentMismatch — compare gcode metadata vs NFC tag
// ---------------------------------------------------------------------------

void PrinterManager::checkFilamentMismatch() {
    if (!strategy_ || mismatchWarned_) return;

    const char* expected = strategy_->getExpectedFilamentType();
    if (expected[0] == '\0') return;  // no metadata available

    // Get current spool from NFCManager
    CurrentSpoolState spool;
    if (!NFCManager::getInstance().getCurrentSpoolState(spool)) return;
    if (!spool.present || !spool.tag_data_valid) return;

    // Get material type from tag
    uint8_t tagMaterial = 0;
    opt_get_material_type(&spool.tag_data, &tagMaterial);
    if (tagMaterial == 0) return;  // unknown material on tag

    // Map tag material type to string for comparison
    const char* tagMaterialStr = nullptr;
    switch (tagMaterial) {
        case 1: tagMaterialStr = "PLA"; break;
        case 2: tagMaterialStr = "PETG"; break;
        case 3: tagMaterialStr = "ABS"; break;
        case 4: tagMaterialStr = "ASA"; break;
        case 5: tagMaterialStr = "TPU"; break;
        case 6: tagMaterialStr = "PA"; break;   // Nylon
        case 7: tagMaterialStr = "PC"; break;
        case 8: tagMaterialStr = "PVA"; break;
        case 9: tagMaterialStr = "HIPS"; break;
        case 10: tagMaterialStr = "PP"; break;
        default: return;  // unknown type, skip check
    }

    // Case-insensitive prefix match (gcode may say "PLA" or "PETG CF", tag says "PLA")
    if (strncasecmp(expected, tagMaterialStr, strlen(tagMaterialStr)) != 0) {
        mismatchWarned_ = true;
        Serial.printf("PrinterManager: FILAMENT MISMATCH — gcode expects '%s', tag has '%s'\n",
                      expected, tagMaterialStr);

        AppMessage warn;
        warn.type = AppMessageType::PRINTER_WARNING;
        memset(&warn.payload.printerWarning, 0, sizeof(warn.payload.printerWarning));
        strncpy(warn.payload.printerWarning.warning_type, "filament_mismatch",
                sizeof(warn.payload.printerWarning.warning_type) - 1);
        strncpy(warn.payload.printerWarning.expected, expected,
                sizeof(warn.payload.printerWarning.expected) - 1);
        strncpy(warn.payload.printerWarning.actual, tagMaterialStr,
                sizeof(warn.payload.printerWarning.actual) - 1);
        ApplicationManager::getInstance().sendMessage(warn);
    } else {
        Serial.printf("PrinterManager: Filament OK — gcode '%s' matches tag '%s'\n",
                      expected, tagMaterialStr);
    }

    // Temperature range check
    float gcodeNozzle = strategy_->getExpectedNozzleTemp();
    if (gcodeNozzle > 0.0f && spool.tag_data_valid) {
        int16_t tagMaxTemp = 0;
        opt_get_max_print_temp(&spool.tag_data, &tagMaxTemp);
        if (tagMaxTemp > 0 && gcodeNozzle > static_cast<float>(tagMaxTemp) + 10.0f) {
            Serial.printf("PrinterManager: TEMP WARNING — gcode %.0fC exceeds tag max %dC\n",
                          gcodeNozzle, tagMaxTemp);

            AppMessage warn;
            warn.type = AppMessageType::PRINTER_WARNING;
            memset(&warn.payload.printerWarning, 0, sizeof(warn.payload.printerWarning));
            strncpy(warn.payload.printerWarning.warning_type, "temp_exceeds_max",
                    sizeof(warn.payload.printerWarning.warning_type) - 1);
            warn.payload.printerWarning.gcode_temp = gcodeNozzle;
            warn.payload.printerWarning.tag_max_temp = tagMaxTemp;
            ApplicationManager::getInstance().sendMessage(warn);
        }
    }
}
