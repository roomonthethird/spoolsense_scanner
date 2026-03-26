#include "PrusaLinkStrategy.h"
#include "ConfigurationManager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

static constexpr size_t URL_BUF = 192;

static bool buildUrl(char* out, size_t sz, const char* base, const char* path) {
    int n = snprintf(out, sz, "%s%s", base, path);
    return n > 0 && static_cast<size_t>(n) < sz;
}

// ---------------------------------------------------------------------------
// update() — called each poll cycle
// ---------------------------------------------------------------------------

void PrusaLinkStrategy::update() {
    // Reset transient state
    connected_ = false;
    hasJob_ = false;
    jobId_ = -1;
    progress_ = 0.0f;
    totalFilamentG_ = 0.0f;
    jobState_[0] = '\0';
    expectedFilamentType_[0] = '\0';
    expectedNozzleTemp_ = 0.0f;
    expectedBedTemp_ = 0.0f;

    bool mutexHeld = false;
    if (httpMutex_) {
        if (xSemaphoreTake(httpMutex_, pdMS_TO_TICKS(10000)) != pdTRUE) {
            Serial.println("PrusaLink: Could not acquire HTTP mutex");
            return;
        }
        mutexHeld = true;
    }

    // Fetch printer info once (MMU, nozzle diameter)
    if (!infoFetched_) {
        fetchInfo();
    }

    // Fetch status first, then job details if active
    if (fetchStatus() && hasJob_) {
        fetchJob();
    }

    if (mutexHeld) {
        xSemaphoreGive(httpMutex_);
    }
}

// ---------------------------------------------------------------------------
// fetchStatus() — GET /api/v1/status
// ---------------------------------------------------------------------------

bool PrusaLinkStrategy::fetchStatus() {
    auto& cfg = ConfigurationManager::getInstance();
    const char* baseUrl = cfg.getPrusaLinkURL();
    const char* apiKey = cfg.getPrusaLinkAPIKey();

    if (baseUrl[0] == '\0' || apiKey[0] == '\0') return false;

    char url[URL_BUF];
    if (!buildUrl(url, sizeof(url), baseUrl, "/api/v1/status")) return false;

    HTTPClient http;
    http.useHTTP10(true);
    http.setReuse(false);
    http.setTimeout(8000);
    http.begin(url);
    http.addHeader("X-Api-Key", apiKey);

    int code = http.GET();
    if (code != 200) {
        if (code > 0) {
            Serial.printf("PrusaLink: /status HTTP %d\n", code);
        }
        http.end();
        return false;
    }

    connected_ = true;

    // Parse with filter — only extract job.id, job.progress, printer temps
    StaticJsonDocument<64> filter;
    filter["job"]["id"] = true;
    filter["job"]["progress"] = true;
    filter["printer"]["temp_nozzle"] = true;
    filter["printer"]["target_nozzle"] = true;
    filter["printer"]["temp_bed"] = true;
    filter["printer"]["target_bed"] = true;
    filter["printer"]["state"] = true;

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        Serial.printf("PrusaLink: /status parse error: %s\n", err.c_str());
        return false;
    }

    JsonObject job = doc["job"];
    if (!job.isNull() && job.containsKey("id")) {
        jobId_ = job["id"].as<int>();
        progress_ = job["progress"] | 0.0f;
        hasJob_ = true;
    }

    return true;
}

// ---------------------------------------------------------------------------
// fetchInfo() — GET /api/v1/info (called once on first successful connection)
// ---------------------------------------------------------------------------

void PrusaLinkStrategy::fetchInfo() {
    auto& cfg = ConfigurationManager::getInstance();
    const char* baseUrl = cfg.getPrusaLinkURL();
    const char* apiKey = cfg.getPrusaLinkAPIKey();

    char url[URL_BUF];
    if (!buildUrl(url, sizeof(url), baseUrl, "/api/v1/info")) return;

    HTTPClient http;
    http.useHTTP10(true);
    http.setReuse(false);
    http.setTimeout(5000);
    http.begin(url);
    http.addHeader("X-Api-Key", apiKey);

    int code = http.GET();
    if (code != 200) {
        http.end();
        return;  // will retry next cycle
    }

    StaticJsonDocument<64> filter;
    filter["mmu"] = true;
    filter["nozzle_diameter"] = true;
    filter["name"] = true;

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (err) return;

    hasMMU_ = doc["mmu"] | false;
    nozzleDiameter_ = doc["nozzle_diameter"] | 0.0f;
    const char* name = doc["name"] | "Unknown";

    Serial.printf("PrusaLink: Printer '%s' — nozzle=%.1fmm mmu=%s\n",
                  name, nozzleDiameter_, hasMMU_ ? "yes" : "no");

    infoFetched_ = true;
}

// ---------------------------------------------------------------------------
// fetchJob() — GET /api/v1/job
// ---------------------------------------------------------------------------

bool PrusaLinkStrategy::fetchJob() {
    auto& cfg = ConfigurationManager::getInstance();
    const char* baseUrl = cfg.getPrusaLinkURL();
    const char* apiKey = cfg.getPrusaLinkAPIKey();

    char url[URL_BUF];
    if (!buildUrl(url, sizeof(url), baseUrl, "/api/v1/job")) return false;

    HTTPClient http;
    http.useHTTP10(true);
    http.setReuse(false);
    http.setTimeout(8000);
    http.begin(url);
    http.addHeader("X-Api-Key", apiKey);

    int code = http.GET();
    if (code == 204) {
        // No active job
        http.end();
        return true;
    }
    if (code != 200) {
        Serial.printf("PrusaLink: /job HTTP %d\n", code);
        http.end();
        return false;
    }

    // Parse with filter — extract state, file.meta fields + per-tool arrays
    StaticJsonDocument<256> filter;
    filter["state"] = true;
    filter["file"]["meta"]["filament used [g]"] = true;
    filter["file"]["meta"]["filament_type"] = true;
    filter["file"]["meta"]["temperature"] = true;
    filter["file"]["meta"]["bed_temperature"] = true;
    filter["file"]["meta"]["material_name"] = true;
    filter["file"]["meta"]["filament used [g] per tool"] = true;
    filter["file"]["meta"]["filament_type per tool"] = true;

    StaticJsonDocument<768> doc;
    DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        Serial.printf("PrusaLink: /job parse error: %s\n", err.c_str());
        return false;
    }

    // Job state
    const char* state = doc["state"] | "";
    strncpy(jobState_, state, sizeof(jobState_) - 1);
    jobState_[sizeof(jobState_) - 1] = '\0';

    // Filament usage from gcode metadata
    JsonObject meta = doc["file"]["meta"];
    if (!meta.isNull()) {
        totalFilamentG_ = meta["filament used [g]"] | 0.0f;

        // Pre-print validation data
        const char* filType = meta["filament_type"] | "";
        strncpy(expectedFilamentType_, filType, sizeof(expectedFilamentType_) - 1);
        expectedFilamentType_[sizeof(expectedFilamentType_) - 1] = '\0';

        expectedNozzleTemp_ = meta["temperature"] | 0.0f;
        expectedBedTemp_ = meta["bed_temperature"] | 0.0f;

        // Per-tool arrays (XL multi-head)
        toolCount_ = 0;
        JsonArray filPerTool = meta["filament used [g] per tool"];
        if (!filPerTool.isNull()) {
            for (JsonVariant v : filPerTool) {
                if (toolCount_ >= MAX_TOOLS) break;
                filamentPerTool_[toolCount_] = v.as<float>();
                toolCount_++;
            }
        }

        JsonArray typePerTool = meta["filament_type per tool"];
        if (!typePerTool.isNull()) {
            int idx = 0;
            for (JsonVariant v : typePerTool) {
                if (idx >= MAX_TOOLS) break;
                const char* t = v.as<const char*>();
                if (t) {
                    strncpy(filamentTypePerTool_[idx], t, sizeof(filamentTypePerTool_[idx]) - 1);
                    filamentTypePerTool_[idx][sizeof(filamentTypePerTool_[idx]) - 1] = '\0';
                }
                idx++;
            }
        }

        if (toolCount_ > 1) {
            Serial.printf("PrusaLink: Multi-tool job detected (%d tools)\n", toolCount_);
            for (int i = 0; i < toolCount_; i++) {
                Serial.printf("  Tool %d: %.2fg %s\n", i, filamentPerTool_[i],
                              filamentTypePerTool_[i][0] ? filamentTypePerTool_[i] : "?");
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// fetchDeferredFilament() — retry job API after print ends
// ---------------------------------------------------------------------------

float PrusaLinkStrategy::fetchDeferredFilament(int expectedJobId) {
    if (expectedJobId < 0) return 0.0f;

    auto& cfg = ConfigurationManager::getInstance();
    const char* baseUrl = cfg.getPrusaLinkURL();
    const char* apiKey = cfg.getPrusaLinkAPIKey();

    bool mutexHeld = false;
    if (httpMutex_) {
        if (xSemaphoreTake(httpMutex_, pdMS_TO_TICKS(10000)) != pdTRUE) return 0.0f;
        mutexHeld = true;
    }

    float result = 0.0f;

    // Retry up to 3 times with backoff — job metadata may appear after completion
    for (int attempt = 1; attempt <= 3; attempt++) {
        if (attempt > 1) {
            vTaskDelay(pdMS_TO_TICKS(attempt * 1000));
        }

        char url[URL_BUF];
        snprintf(url, sizeof(url), "%s/api/v1/job/%d", baseUrl, expectedJobId);

        HTTPClient http;
        http.useHTTP10(true);
        http.setReuse(false);
        http.setTimeout(8000);
        http.begin(url);
        http.addHeader("X-Api-Key", apiKey);

        int code = http.GET();
        if (code == 200) {
            StaticJsonDocument<64> filter;
            filter["file"]["meta"]["filament used [g]"] = true;

            StaticJsonDocument<256> doc;
            if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))) {
                float g = doc["file"]["meta"]["filament used [g]"] | 0.0f;
                if (g > 0.0f) {
                    result = g;
                    Serial.printf("PrusaLink: Deferred filament for job %d: %.2fg\n", expectedJobId, result);
                    http.end();
                    break;
                }
            }
        }
        http.end();
    }

    if (mutexHeld) {
        xSemaphoreGive(httpMutex_);
    }
    return result;
}
