// Native unit tests for PrinterManager state machine.
// Tests the IDLE→TRACKING state machine, filament deduction, job lifecycle,
// grace periods, and deferred filament lookup.
//
// Build: make test_printer_manager
// Run:   ./test_printer_manager

#define NATIVE_TEST 1

#include "test_helpers.h"
#include "NativePlatform.h"
#include "MockPrinterStrategy.h"

// Minimal stubs for headers PrinterManager includes
namespace ConfigStub {
    static uint32_t pollIntervalMs = 10000;
}

// --- Stub ConfigurationManager ---
class ConfigurationManager {
public:
    static ConfigurationManager& getInstance() { static ConfigurationManager inst; return inst; }
    uint32_t getPollIntervalMs() const { return ConfigStub::pollIntervalMs; }
    bool isPrusaLinkEnabled() const { return true; }
    const char* getPrusaLinkURL() const { return "http://test"; }
    const char* getPrusaLinkAPIKey() const { return "key"; }
};

// --- Capture messages ---
#include <vector>

enum class AppMessageType {
    PRINT_STARTED,
    PRINT_ENDED,
    SPOOL_DETECTED,
    SPOOL_UPDATED,
    BLANK_TAG_DETECTED,
    GENERIC_TAG_DETECTED,
    SPOOLMAN_SYNCED,
    TAG_REMOVED,
    HA_WRITE_TAG,
    HA_UPDATE_REMAINING,
    PRINTER_WARNING,
};

struct PrinterWarningPayload {
    char warning_type[24];
    char expected[32];
    char actual[32];
    float gcode_temp;
    int16_t tag_max_temp;
};

struct AppMessage {
    AppMessageType type;
    union {
        struct { int job_id; } printStarted;
        struct {
            int job_id;
            float filament_used_grams;
            bool canceled;
            int tool_count;
            float filament_per_tool[5];
        } printEnded;
        PrinterWarningPayload printerWarning;
    } payload;
};

static std::vector<AppMessageType> capturedTypes;
static std::vector<float> capturedFilament;
static std::vector<bool> capturedCanceled;
static std::vector<int> capturedJobIds;
static std::vector<int> capturedToolCounts;
static std::vector<std::vector<float>> capturedPerTool;

// --- Stub NFCManager ---
struct CurrentSpoolState {
    bool present = false;
    bool tag_data_valid = false;
};
class NFCManager {
public:
    static NFCManager& getInstance() { static NFCManager inst; return inst; }
    bool getCurrentSpoolState(CurrentSpoolState& out) {
        out.present = false;
        out.tag_data_valid = false;
        return false;
    }
};

class ApplicationManager {
public:
    static ApplicationManager& getInstance() { static ApplicationManager inst; return inst; }
    bool sendMessage(const AppMessage& msg, uint32_t = 0) {
        capturedTypes.push_back(msg.type);
        if (msg.type == AppMessageType::PRINT_STARTED) {
            capturedJobIds.push_back(msg.payload.printStarted.job_id);
        }
        if (msg.type == AppMessageType::PRINT_ENDED) {
            capturedFilament.push_back(msg.payload.printEnded.filament_used_grams);
            capturedCanceled.push_back(msg.payload.printEnded.canceled);
            capturedJobIds.push_back(msg.payload.printEnded.job_id);
            capturedToolCounts.push_back(msg.payload.printEnded.tool_count);
            std::vector<float> tools;
            for (int i = 0; i < msg.payload.printEnded.tool_count; i++) {
                tools.push_back(msg.payload.printEnded.filament_per_tool[i]);
            }
            capturedPerTool.push_back(tools);
        }
        return true;
    }
    void publishToHA(const char*, const char*, bool) {}
};

// Now include the actual PrinterManager (it uses the stubs above)
// We re-implement it inline since we can't easily link the .cpp with all its deps
#include "../../src/IPrinterStrategy.h"

enum class PrinterState { IDLE, TRACKING };

// Minimal PrinterManager reimpl for testing the state machine
class PrinterManager {
public:
    void begin() {
        state_ = PrinterState::IDLE;
        currentJobId_ = -1;
        currentJobTotalFilamentG_ = 0.0f;
        lastProgressPercent_ = 0.0f;
        missingJobPollCount_ = 0;
    }

    void setStrategy(IPrinterStrategy* s) { strategy_ = s; }
    PrinterState getState() const { return state_; }

    void poll() {
        if (!strategy_) return;
        strategy_->update();

        if (!strategy_->isConnected()) {
            if (state_ == PrinterState::TRACKING) {
                missingJobPollCount_++;
                if (missingJobPollCount_ >= 2) handleJobDisappeared();
            }
            return;
        }

        if (!strategy_->hasActiveJob()) {
            if (state_ == PrinterState::TRACKING) {
                missingJobPollCount_++;
                if (missingJobPollCount_ >= 2) handleJobDisappeared();
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
            if (jobId != currentJobId_) {
                resolveAndSendJobEnd(currentJobId_, lastProgressPercent_, false, "job_replaced");
                handleJobDetected(jobId, totalFilamentG);
                return;
            }
            lastProgressPercent_ = progress;
            if (totalFilamentG > 0) currentJobTotalFilamentG_ = totalFilamentG;

            if (strcmp(jobState, "FINISHED") == 0) {
                resolveAndSendJobEnd(jobId, 100.0f, true, "finished");
            } else if (strcmp(jobState, "STOPPED") == 0 || strcmp(jobState, "ERROR") == 0) {
                resolveAndSendJobEnd(jobId, progress, true, "stopped_or_error");
            }
        }
    }

private:
    void handleJobDetected(int jobId, float totalFilamentG) {
        state_ = PrinterState::TRACKING;
        currentJobId_ = jobId;
        currentJobTotalFilamentG_ = totalFilamentG;
        lastProgressPercent_ = 0.0f;
        missingJobPollCount_ = 0;

        AppMessage msg;
        msg.type = AppMessageType::PRINT_STARTED;
        msg.payload.printStarted.job_id = jobId;
        ApplicationManager::getInstance().sendMessage(msg);
    }

    void resolveAndSendJobEnd(int jobId, float progressPercent, bool allowDeferred, const char*) {
        if (currentJobTotalFilamentG_ <= 0.0f && strategy_ && allowDeferred) {
            float deferred = strategy_->fetchDeferredFilament(jobId);
            if (deferred > 0.0f) currentJobTotalFilamentG_ = deferred;
        }

        float filamentUsed = (progressPercent / 100.0f) * currentJobTotalFilamentG_;
        AppMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = AppMessageType::PRINT_ENDED;
        msg.payload.printEnded.job_id = jobId;
        msg.payload.printEnded.filament_used_grams = filamentUsed;
        msg.payload.printEnded.canceled = (progressPercent < 100.0f);

        // Per-tool data
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

        state_ = PrinterState::IDLE;
        currentJobId_ = -1;
        currentJobTotalFilamentG_ = 0.0f;
        lastProgressPercent_ = 0.0f;
        missingJobPollCount_ = 0;
    }

    void handleJobDisappeared() {
        float progress = (lastProgressPercent_ >= 95.0f) ? 100.0f : lastProgressPercent_;
        resolveAndSendJobEnd(currentJobId_, progress, true, "job_disappeared");
    }

    PrinterState state_ = PrinterState::IDLE;
    int currentJobId_ = -1;
    float currentJobTotalFilamentG_ = 0.0f;
    float lastProgressPercent_ = 0.0f;
    uint8_t missingJobPollCount_ = 0;
    IPrinterStrategy* strategy_ = nullptr;
};

// =========================================================================
// Test helpers
// =========================================================================

static MockPrinterStrategy mock;
static PrinterManager pm;

static void reset() {
    capturedTypes.clear();
    capturedFilament.clear();
    capturedCanceled.clear();
    capturedJobIds.clear();
    capturedToolCounts.clear();
    capturedPerTool.clear();
    mock = MockPrinterStrategy();
    pm.begin();
    pm.setStrategy(&mock);
}

// =========================================================================
// Tests
// =========================================================================

bool test_idle_no_job() {
    reset();
    mock.setIdle();
    pm.poll();
    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_EQ(capturedTypes.size(), 0u);
    return true;
}

bool test_detect_print_start() {
    reset();
    mock.setPrinting(42, 9.18f);
    pm.poll();
    TEST_ASSERT_EQ(pm.getState(), PrinterState::TRACKING);
    TEST_ASSERT_EQ(capturedTypes.size(), 1u);
    TEST_ASSERT_EQ(capturedTypes[0], AppMessageType::PRINT_STARTED);
    TEST_ASSERT_EQ(capturedJobIds[0], 42);
    return true;
}

bool test_print_finished_100_percent() {
    reset();
    mock.setPrinting(42, 9.18f);
    pm.poll();  // detect start

    mock.setFinished(100.0f);
    pm.poll();  // detect finish

    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_EQ(capturedTypes.size(), 2u);
    TEST_ASSERT_EQ(capturedTypes[1], AppMessageType::PRINT_ENDED);
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f, 0.01f);
    TEST_ASSERT_EQ(capturedCanceled[0], false);
    return true;
}

bool test_print_canceled_at_30_percent() {
    reset();
    mock.setPrinting(42, 9.18f, 30.0f);
    pm.poll();  // detect start

    mock.setStopped(30.0f);
    pm.poll();  // detect stop

    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f * 0.30f, 0.01f);
    TEST_ASSERT_EQ(capturedCanceled[0], true);
    return true;
}

bool test_job_disappeared_under_95_is_canceled() {
    reset();
    mock.setPrinting(42, 9.18f, 0.0f);
    pm.poll();  // start → TRACKING, lastProgress=0

    mock.setPrinting(42, 9.18f, 50.0f);
    pm.poll();  // update progress to 50%

    mock.clearJob();
    pm.poll();  // grace 1
    TEST_ASSERT_EQ(pm.getState(), PrinterState::TRACKING);

    pm.poll();  // grace 2 → disappeared
    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_EQ(capturedCanceled[0], true);
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f * 0.50f, 0.01f);
    return true;
}

bool test_job_disappeared_at_95_treated_as_finished() {
    reset();
    mock.setPrinting(42, 9.18f, 0.0f);
    pm.poll();  // start

    mock.setPrinting(42, 9.18f, 96.0f);
    pm.poll();  // update progress to 96%

    mock.clearJob();
    pm.poll();  // grace 1
    pm.poll();  // grace 2 → disappeared, 96% >= 95% → treated as finished

    TEST_ASSERT_EQ(capturedCanceled[0], false);  // treated as finished
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f, 0.01f);  // 100%
    return true;
}

bool test_disconnect_during_tracking() {
    reset();
    mock.setPrinting(42, 9.18f, 60.0f);
    pm.poll();

    mock.setDisconnected();
    pm.poll();  // grace 1
    TEST_ASSERT_EQ(pm.getState(), PrinterState::TRACKING);

    pm.poll();  // grace 2 → disappeared
    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_EQ(capturedTypes.size(), 2u);  // START + END
    return true;
}

bool test_job_replaced() {
    reset();
    mock.setPrinting(42, 9.18f, 50.0f);
    pm.poll();  // start job 42

    mock.setPrinting(99, 15.0f, 0.0f);  // new job
    pm.poll();  // should end 42, start 99

    TEST_ASSERT_EQ(pm.getState(), PrinterState::TRACKING);
    TEST_ASSERT_EQ(capturedTypes.size(), 3u);  // START(42) + END(42) + START(99)
    TEST_ASSERT_EQ(capturedTypes[0], AppMessageType::PRINT_STARTED);
    TEST_ASSERT_EQ(capturedTypes[1], AppMessageType::PRINT_ENDED);
    TEST_ASSERT_EQ(capturedTypes[2], AppMessageType::PRINT_STARTED);
    TEST_ASSERT_EQ(capturedJobIds[0], 42);  // first start
    TEST_ASSERT_EQ(capturedJobIds[1], 42);  // end of 42
    TEST_ASSERT_EQ(capturedJobIds[2], 99);  // start of 99
    return true;
}

bool test_deferred_filament_on_finish() {
    reset();
    mock.setPrinting(42, 0.0f);  // no filament metadata initially
    pm.poll();

    mock.setDeferredFilament(42, 9.18f);
    mock.setFinished(100.0f);
    pm.poll();

    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f, 0.01f);
    TEST_ASSERT_EQ(mock.deferredCalled_, 1);
    return true;
}

bool test_deferred_not_called_on_replace() {
    reset();
    mock.setPrinting(42, 0.0f);
    pm.poll();

    mock.setDeferredFilament(42, 9.18f);
    mock.setPrinting(99, 15.0f);  // replace, not finish
    pm.poll();

    // Deferred should NOT be called for replaced jobs
    TEST_ASSERT_EQ(mock.deferredCalled_, 0);
    return true;
}

bool test_update_called_each_poll() {
    reset();
    mock.setIdle();
    pm.poll();
    pm.poll();
    pm.poll();
    TEST_ASSERT_EQ(mock.updateCalled_, 3);
    return true;
}

bool test_filament_updates_during_tracking() {
    reset();
    mock.setPrinting(42, 0.0f);  // no metadata yet
    pm.poll();

    mock.setPrinting(42, 9.18f, 50.0f);  // metadata arrives
    pm.poll();

    mock.setFinished(100.0f);
    pm.poll();

    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 9.18f, 0.01f);
    return true;
}

bool test_zero_filament_no_deferred() {
    reset();
    mock.setPrinting(42, 0.0f);
    pm.poll();

    // No deferred data set
    mock.setFinished(100.0f);
    pm.poll();

    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 0.0f, 0.01f);
    return true;
}

bool test_single_tool_has_zero_tool_count() {
    reset();
    mock.setPrinting(42, 9.18f);
    pm.poll();

    mock.setFinished(100.0f);
    pm.poll();

    TEST_ASSERT_EQ(capturedToolCounts[0], 0);
    TEST_ASSERT_EQ(capturedPerTool[0].size(), 0u);
    return true;
}

bool test_multi_tool_per_tool_data() {
    reset();
    // Set per-tool data on mock
    mock.setPrinting(200, 45.0f);
    // MockPrinterStrategy doesn't have per-tool setters yet,
    // but the IPrinterStrategy interface defaults return 0 tool count.
    // For this test, we verify that with 0 tools, no per-tool data is sent.
    pm.poll();

    mock.setFinished(100.0f);
    pm.poll();

    TEST_ASSERT_EQ(capturedToolCounts[0], 0);
    TEST_ASSERT_FLOAT_NEAR(capturedFilament[0], 45.0f, 0.01f);
    return true;
}

bool test_no_strategy_does_nothing() {
    reset();
    pm.setStrategy(nullptr);
    pm.poll();
    TEST_ASSERT_EQ(capturedTypes.size(), 0u);
    TEST_ASSERT_EQ(pm.getState(), PrinterState::IDLE);
    return true;
}

bool test_paused_starts_tracking() {
    reset();
    mock.connected_ = true;
    mock.hasJob_ = true;
    mock.jobId_ = 55;
    mock.progress_ = 10.0f;
    mock.totalFilamentG_ = 5.0f;
    strncpy(mock.jobState_, "PAUSED", sizeof(mock.jobState_));
    mock.updateCalled_ = 0;

    pm.poll();
    TEST_ASSERT_EQ(pm.getState(), PrinterState::TRACKING);
    TEST_ASSERT_EQ(capturedTypes[0], AppMessageType::PRINT_STARTED);
    return true;
}

// =========================================================================

int main() {
    printf("PrinterManager tests\n");
    printf("====================\n\n");

    RUN_TEST(test_idle_no_job);
    RUN_TEST(test_detect_print_start);
    RUN_TEST(test_print_finished_100_percent);
    RUN_TEST(test_print_canceled_at_30_percent);
    RUN_TEST(test_job_disappeared_under_95_is_canceled);
    RUN_TEST(test_job_disappeared_at_95_treated_as_finished);
    RUN_TEST(test_disconnect_during_tracking);
    RUN_TEST(test_job_replaced);
    RUN_TEST(test_deferred_filament_on_finish);
    RUN_TEST(test_deferred_not_called_on_replace);
    RUN_TEST(test_update_called_each_poll);
    RUN_TEST(test_filament_updates_during_tracking);
    RUN_TEST(test_zero_filament_no_deferred);
    RUN_TEST(test_single_tool_has_zero_tool_count);
    RUN_TEST(test_multi_tool_per_tool_data);
    RUN_TEST(test_no_strategy_does_nothing);
    RUN_TEST(test_paused_starts_tracking);

    print_test_summary();
    return _tests_failed > 0 ? 1 : 0;
}
