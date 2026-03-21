#include "WebServerManager.h"

#ifndef NATIVE_TEST

#include <Arduino.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "LandingHTML.h"
#include "TagWriterHTML.h"
#include "ReaderHTML.h"
#include "TigerTagWriterHTML.h"
#include "OpenTag3DWriterHTML.h"
#include "SharedCSS.h"
#include "SharedJS.h"
#include "ConfigHTML.h"
#include "OpenPrintTagLogo.h"
#include "TigerTagLogo.h"
#include "UpdateHTML.h"
#include "ConfigurationManager.h"
#include "NFCManager.h"
#include "NFCTypes.h"
#include "NFCWriteTypes.h"
#include "ApplicationManager.h"
#include "ConversionUtils.h"
#include "TigerTagParser.h"

extern "C" {
#include "openprinttag_lib.h"
}

// Tag kind enum to string for API responses
static const char* tagKindToString(TagKind kind) {
    switch (kind) {
        case TagKind::OpenPrintTag: return "OpenPrintTag";
        case TagKind::GenericUidTag: return "GenericUidTag";
        case TagKind::TigerTag:     return "TigerTag";
        case TagKind::OpenTag3D:    return "OpenTag3D";
        case TagKind::BambuTag:     return "BambuTag";
        case TagKind::BlankTag:     return "BlankTag";
        default:                    return "Unsupported";
    }
}

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

bool WebServerManager::begin(uint16_t port) {
    // mDNS — reachable as http://spoolsense.local
    if (MDNS.begin("spoolsense")) {
        MDNS.addService("http", "tcp", port);
        Serial.println("WebServerManager: mDNS started (spoolsense.local)");
    } else {
        Serial.println("WebServerManager: mDNS failed — reachable by IP only");
    }

    // Pages
    _server.on("/",                    HTTP_GET, [this]() { handleLanding(); });
    _server.on("/reader",              HTTP_GET, [this]() { handleReader(); });
    _server.on("/writer/openprinttag", HTTP_GET, [this]() { handleOpenPrintTagWriter(); });
    _server.on("/writer/tigertag",     HTTP_GET, [this]() { handleTigerTagWriter(); });
    _server.on("/writer/opentag3d",    HTTP_GET, [this]() { handleOpenTag3DWriter(); });

    // Static assets
    _server.on("/css/shared.css",      HTTP_GET, [this]() { handleSharedCSS(); });
    _server.on("/js/shared.js",        HTTP_GET, [this]() { handleSharedJS(); });
    _server.on("/img/openprinttag.png", HTTP_GET, [this]() { handleOpenPrintTagLogo(); });
    _server.on("/img/tigertag.png",    HTTP_GET, [this]() { handleTigerTagLogo(); });

    // Update page
    _server.on("/update",              HTTP_GET, [this]() { handleUpdatePage(); });
    _server.on("/config",              HTTP_GET, [this]() { handleConfigPage(); });

    // API
    _server.on("/api/version",         HTTP_GET, [this]() { handleApiVersion(); });
    _server.on("/api/upload-firmware",  HTTP_POST,
        [this]() { handleApiUploadFirmwareComplete(); },
        [this]() { handleApiUploadFirmwareChunk(); });
    _server.on("/api/update-from-url", HTTP_POST, [this]() { handleApiUpdateFromUrl(); });
    _server.on("/api/ota-status",      HTTP_GET,  [this]() { handleApiOtaStatus(); });
    _server.on("/api/config",          HTTP_GET,  [this]() { handleApiGetConfig(); });
    _server.on("/api/config",          HTTP_POST, [this]() { handleApiPostConfig(); });
    _server.on("/api/status",          HTTP_GET,  [this]() { handleApiStatus(); });
    _server.on("/api/write-tag",       HTTP_POST, [this]() { handleApiWriteTag(); });
    _server.on("/api/format-tag",      HTTP_POST, [this]() { handleApiFormatTag(); });
    _server.on("/api/write-tigertag",  HTTP_POST, [this]() { handleApiWriteTigerTag(); });
    _server.on("/api/write-opentag3d", HTTP_POST, [this]() { handleApiWriteOpenTag3D(); });

    // Allow browser preflight requests (CORS) so the page can be tested
    // from a local file during development.
    _server.onNotFound([this]() {
        if (_server.method() == HTTP_OPTIONS) {
            _server.sendHeader("Access-Control-Allow-Origin", "*");
            _server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
            _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
            _server.send(204);
        } else {
            _server.send(404, "text/plain", "Not found");
        }
    });

    _server.begin();
    _initialized = true;
    Serial.printf("WebServerManager: HTTP server started on port %u\n", port);
    return true;
}

void WebServerManager::handleClient() {
    if (_initialized) {
        _server.handleClient();
    }
}

// ---------------------------------------------------------------------------
// Page handlers
// ---------------------------------------------------------------------------

void WebServerManager::handleLanding() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", LANDING_HTML);
}

void WebServerManager::handleReader() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", READER_HTML);
}

void WebServerManager::handleOpenPrintTagWriter() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", TAG_WRITER_HTML);
}

void WebServerManager::handleTigerTagWriter() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", TIGERTAG_WRITER_HTML);
}

void WebServerManager::handleOpenTag3DWriter() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", OPENTAG3D_WRITER_HTML);
}

void WebServerManager::handleSharedCSS() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Cache-Control", "public, max-age=86400");
    _server.send_P(200, "text/css", SHARED_CSS);
}

void WebServerManager::handleSharedJS() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Cache-Control", "public, max-age=86400");
    _server.send_P(200, "application/javascript", SHARED_JS);
}

void WebServerManager::handleOpenPrintTagLogo() {
    _server.sendHeader("Cache-Control", "public, max-age=86400");
    _server.send_P(200, "image/png", reinterpret_cast<const char*>(OPENPRINTTAG_LOGO_PNG), OPENPRINTTAG_LOGO_PNG_LEN);
}

void WebServerManager::handleTigerTagLogo() {
    _server.sendHeader("Cache-Control", "public, max-age=86400");
    _server.send_P(200, "image/png", reinterpret_cast<const char*>(TIGERTAG_LOGO_PNG), TIGERTAG_LOGO_PNG_LEN);
}

void WebServerManager::handleUpdatePage() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", UPDATE_HTML);
}

void WebServerManager::handleConfigPage() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", CONFIG_HTML);
}

// ---------------------------------------------------------------------------
// API: Config
// ---------------------------------------------------------------------------

void WebServerManager::handleApiGetConfig() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    ConfigUpdate cfg;
    ConfigurationManager::getInstance().getCurrentConfig(cfg);

    StaticJsonDocument<512> doc;
    doc["wifi_ssid"] = cfg.wifi_ssid;
    doc["wifi_pass_set"] = (cfg.wifi_pass[0] != '\0');
    doc["mqtt_host"] = cfg.mqtt_host;
    doc["mqtt_port"] = cfg.mqtt_port;
    doc["mqtt_user"] = cfg.mqtt_user;
    doc["mqtt_pass_set"] = (cfg.mqtt_pass[0] != '\0');
    doc["spoolman_on"] = cfg.spoolman_on;
    doc["spoolman_url"] = cfg.spoolman_url;
    doc["auto_mode"] = cfg.auto_mode;
    doc["lcd_enabled"] = cfg.lcd_enabled;
    doc["led_enabled"] = cfg.led_enabled;

    String body;
    serializeJson(doc, body);
    _server.send(200, "application/json", body);
}

void WebServerManager::handleApiPostConfig() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }

    ConfigUpdate update;
    memset(&update, 0, sizeof(update));

    strncpy(update.wifi_ssid, doc["wifi_ssid"] | "", sizeof(update.wifi_ssid) - 1);
    strncpy(update.wifi_pass, doc["wifi_pass"] | "", sizeof(update.wifi_pass) - 1);
    strncpy(update.mqtt_host, doc["mqtt_host"] | "", sizeof(update.mqtt_host) - 1);
    update.mqtt_port = doc["mqtt_port"] | (uint16_t)1883;
    strncpy(update.mqtt_user, doc["mqtt_user"] | "", sizeof(update.mqtt_user) - 1);
    strncpy(update.mqtt_pass, doc["mqtt_pass"] | "", sizeof(update.mqtt_pass) - 1);
    update.spoolman_on = doc["spoolman_on"] | (uint8_t)0;
    strncpy(update.spoolman_url, doc["spoolman_url"] | "", sizeof(update.spoolman_url) - 1);
    update.auto_mode = doc["auto_mode"] | (uint8_t)0;
    update.lcd_enabled = doc["lcd_enabled"] | (uint8_t)0;
    update.led_enabled = doc["led_enabled"] | (uint8_t)0;

    if (update.wifi_ssid[0] == '\0') {
        sendError(400, "WiFi SSID is required");
        return;
    }

    if (!ConfigurationManager::getInstance().saveToNVS(update)) {
        sendError(500, "Failed to save config");
        return;
    }

    Serial.println("WebServerManager: Config saved, rebooting in 3 seconds...");
    _server.send(200, "application/json", "{\"success\":true}");
    delay(3000);
    ESP.restart();
}

// ---------------------------------------------------------------------------
// API: Version
// ---------------------------------------------------------------------------

void WebServerManager::handleApiVersion() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<128> doc;
    doc["version"] = FIRMWARE_VERSION;
#ifdef BOARD_ESP32_S3
    doc["board"] = "esp32s3zero";
#else
    doc["board"] = "esp32dev";
#endif
    String body;
    serializeJson(doc, body);
    _server.send(200, "application/json", body);
}

// ---------------------------------------------------------------------------
// API: Firmware Upload (OTA)
// ---------------------------------------------------------------------------

void WebServerManager::handleApiUploadFirmwareChunk() {
    HTTPUpload& upload = _server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA: Upload start: %s\n", upload.filename.c_str());
        // Pause NFC scan task during upload to prevent interference
        NFCManager::getInstance().pauseScanTask();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("OTA: Success, %u bytes written\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println("OTA: Upload aborted");
        Update.end();
        NFCManager::getInstance().resumeScanTask();
    }
}

void WebServerManager::handleApiUploadFirmwareComplete() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    if (Update.hasError()) {
        NFCManager::getInstance().resumeScanTask();
        _server.send(500, "application/json", "{\"success\":false,\"error\":\"Update failed\"}");
    } else {
        _server.send(200, "application/json", "{\"success\":true}");
        Serial.println("OTA: Rebooting...");
        delay(1000);
        ESP.restart();
    }
}

// ---------------------------------------------------------------------------
// API: Update from URL (ESP32 downloads .bin from GitHub)
// ---------------------------------------------------------------------------

void WebServerManager::handleApiUpdateFromUrl() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    if (_otaState == OtaState::DOWNLOADING || _otaState == OtaState::FLASHING) {
        sendError(409, "Update already in progress");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }

    const char* url = doc["url"] | "";
    if (strlen(url) == 0) {
        sendError(400, "Missing url");
        return;
    }

    // Store URL and kick off background task
    strncpy(_otaUrl, url, sizeof(_otaUrl) - 1);
    _otaState = OtaState::DOWNLOADING;
    _otaProgress = 0;
    _otaError[0] = '\0';

    Serial.printf("OTA: Free heap before task: %u\n", ESP.getFreeHeap());
    xTaskCreatePinnedToCore(otaDownloadTask, "OTATask", 24576, this, 2, nullptr, 0);

    _server.send(200, "application/json", "{\"success\":true,\"status\":\"started\"}");
}

void WebServerManager::otaDownloadTask(void* param) {
    WebServerManager* self = static_cast<WebServerManager*>(param);

    Serial.printf("OTA: Downloading from %s\n", self->_otaUrl);

    // Pause NFC during OTA
    NFCManager::getInstance().pauseScanTask();

    WiFiClientSecure secureClient;
    secureClient.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(30000);
    http.begin(secureClient, self->_otaUrl);
    http.addHeader("Accept", "application/octet-stream");
    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("OTA: Download failed, HTTP %d\n", httpCode);
        http.end();
        NFCManager::getInstance().resumeScanTask();
        snprintf(self->_otaError, sizeof(self->_otaError), "Download failed: HTTP %d", httpCode);
        self->_otaState = OtaState::FAILED;
        vTaskDelete(nullptr);
        return;
    }

    int contentLength = http.getSize();
    Serial.printf("OTA: Content length: %d bytes\n", contentLength);
    Serial.printf("OTA: Free heap before Update.begin: %u\n", ESP.getFreeHeap());

    self->_otaState = OtaState::FLASHING;

    if (!Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN)) {
        Serial.println("OTA: Update.begin() failed");
        Update.printError(Serial);
        http.end();
        NFCManager::getInstance().resumeScanTask();
        strncpy(self->_otaError, "Update.begin failed", sizeof(self->_otaError));
        self->_otaState = OtaState::FAILED;
        vTaskDelete(nullptr);
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[1024];
    size_t written = 0;

    while (http.connected() && (contentLength <= 0 || written < (size_t)contentLength)) {
        size_t available = stream->available();
        if (available) {
            size_t toRead = (available > sizeof(buf)) ? sizeof(buf) : available;
            size_t bytesRead = stream->readBytes(buf, toRead);
            if (bytesRead > 0) {
                Update.write(buf, bytesRead);
                written += bytesRead;
                if (contentLength > 0) {
                    self->_otaProgress = (uint8_t)((written * 100) / contentLength);
                }
            }
        }
        vTaskDelay(1);
    }

    http.end();

    if (Update.end(true)) {
        Serial.printf("OTA: Success, %u bytes written\n", written);
        self->_otaProgress = 100;
        self->_otaState = OtaState::SUCCESS;
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP.restart();
    } else {
        Serial.println("OTA: Update.end() failed");
        Update.printError(Serial);
        NFCManager::getInstance().resumeScanTask();
        strncpy(self->_otaError, "Update verification failed", sizeof(self->_otaError));
        self->_otaState = OtaState::FAILED;
    }

    vTaskDelete(nullptr);
}

void WebServerManager::handleApiOtaStatus() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    StaticJsonDocument<128> doc;

    switch (_otaState) {
        case OtaState::IDLE:        doc["state"] = "idle"; break;
        case OtaState::DOWNLOADING: doc["state"] = "downloading"; break;
        case OtaState::FLASHING:    doc["state"] = "flashing"; break;
        case OtaState::SUCCESS:     doc["state"] = "success"; break;
        case OtaState::FAILED:      doc["state"] = "failed"; doc["error"] = _otaError; break;
    }
    doc["progress"] = _otaProgress;

    String body;
    serializeJson(doc, body);
    _server.send(200, "application/json", body);
}

// ---------------------------------------------------------------------------
// API: Status
// ---------------------------------------------------------------------------

void WebServerManager::handleApiStatus() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    CurrentSpoolState state;
    StaticJsonDocument<1024> doc;

    if (NFCManager::getInstance().getCurrentSpoolState(state) && state.present) {
        doc["present"] = true;
        doc["uid"] = state.spool_id;
        doc["tag_data_valid"] = state.tag_data_valid;
        doc["tag_kind"] = tagKindToString(state.kind);

        if (state.kind == TagKind::TigerTag) {
            // TigerTag — include parsed TigerTag data
            TigerTagData tt;
            if (NFCManager::getInstance().getLastTigerTagData(tt) && tt.valid) {
                JsonObject ttObj = doc.createNestedObject("tigertag");
                ttObj["material_id"] = tt.material_id;
                ttObj["material_name"] = tt.material_name;
                ttObj["brand_id"] = tt.brand_id;
                ttObj["brand_name"] = tt.brand_name;
                ttObj["weight_g"] = tt.weight_g;
                ttObj["diameter_mm"] = tt.diameter_mm;
                ttObj["aspect1_name"] = tt.aspect1_name;
                ttObj["aspect2_name"] = tt.aspect2_name;

                char colorHex[8];
                snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X",
                         tt.color_r, tt.color_g, tt.color_b);
                ttObj["color_hex"] = colorHex;

                if (tt.nozzle_temp_min > 0) ttObj["nozzle_temp_min"] = tt.nozzle_temp_min;
                if (tt.nozzle_temp_max > 0) ttObj["nozzle_temp_max"] = tt.nozzle_temp_max;
                if (tt.bed_temp_min > 0) ttObj["bed_temp_min"] = tt.bed_temp_min;
                if (tt.bed_temp_max > 0) ttObj["bed_temp_max"] = tt.bed_temp_max;
                if (tt.dry_temp > 0) ttObj["dry_temp"] = tt.dry_temp;
                if (tt.dry_time_hours > 0) ttObj["dry_time_hours"] = tt.dry_time_hours;
            }
        } else if (state.tag_data_valid) {
            // OpenPrintTag — include OPT fields
            uint8_t mat_type = 0;
            opt_get_material_type(&state.tag_data, &mat_type);
            doc["material_type"] = mat_type;
            doc["material_name"] = materialTypeToString(mat_type);

            uint8_t color[4] = {0};
            if (opt_get_primary_color(&state.tag_data, color) == OPT_OK) {
                char colorHex[8];
                snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X",
                         color[0], color[1], color[2]);
                doc["color"] = colorHex;
            } else {
                doc["color"] = (const char*)nullptr;
            }

            char manufacturer[33] = {0};
            opt_get_brand_name(&state.tag_data, manufacturer, sizeof(manufacturer));
            doc["manufacturer"] = manufacturer;

            float full_weight = 0.0f, consumed = 0.0f;
            opt_get_actual_full_weight(&state.tag_data, &full_weight);
            opt_get_consumed_weight(&state.tag_data, &consumed);
            doc["remaining_g"] = full_weight - consumed;
            doc["initial_weight_g"] = full_weight;

            int32_t spoolman_id = -1;
            opt_get_gp_spoolman_id(&state.tag_data, &spoolman_id);
            doc["spoolman_id"] = spoolman_id;

            float density = 0.0f;
            if (opt_get_density(&state.tag_data, &density) == OPT_OK && density > 0.0f)
                doc["density"] = density;

            float diameter = 0.0f;
            if (opt_get_filament_diameter(&state.tag_data, &diameter) == OPT_OK && diameter > 0.0f)
                doc["diameter_mm"] = diameter;

            char mat_name_custom[33] = {0};
            if (opt_get_material_name(&state.tag_data, mat_name_custom, sizeof(mat_name_custom)) == OPT_OK
                    && mat_name_custom[0] != '\0')
                doc["material_name"] = mat_name_custom;

            int16_t t = 0;
            if (opt_get_min_print_temp(&state.tag_data, &t) == OPT_OK && t != 0) doc["min_print_temp"] = t;
            if (opt_get_max_print_temp(&state.tag_data, &t) == OPT_OK && t != 0) doc["max_print_temp"] = t;
            if (opt_get_preheat_temp(&state.tag_data, &t) == OPT_OK && t != 0)   doc["preheat_temp"] = t;
            if (opt_get_min_bed_temp(&state.tag_data, &t) == OPT_OK && t != 0)   doc["min_bed_temp"] = t;
            if (opt_get_max_bed_temp(&state.tag_data, &t) == OPT_OK && t != 0)   doc["max_bed_temp"] = t;
        }
    } else {
        doc["present"] = false;
        doc["tag_data_valid"] = false;
    }

    String body;
    serializeJson(doc, body);
    _server.send(200, "application/json", body);
}

// ---------------------------------------------------------------------------
// API: Write OpenPrintTag
// ---------------------------------------------------------------------------

void WebServerManager::handleApiWriteTag() {
    Serial.println("WebServerManager: POST /api/write-tag received");
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }

    // Parse color
    uint8_t color[4] = {0};
    const char* colorStr = doc["color"] | "";
    if (!parseHexColor(colorStr, color)) {
        // Non-fatal — color stays zero if not provided or invalid
    }

    // Parse material type — accept either integer or string name
    uint8_t mat_type = 0;
    if (doc["material_type"].is<int>()) {
        mat_type = doc["material_type"].as<uint8_t>();
    } else {
        mat_type = materialTypeFromString(doc["material_type"] | "PLA");
    }

    const char* uid        = doc["uid"] | "";
    const char* mfr        = doc["manufacturer"] | "";
    float initial_weight_g = doc["initial_weight_g"] | 0.0f;
    float remaining_g      = doc["remaining_g"] | 0.0f;
    int32_t spoolman_id    = doc["spoolman_id"] | -1;

    float consumed_g = initial_weight_g - remaining_g;
    if (consumed_g < 0.0f) consumed_g = 0.0f;

    uint32_t base_id = millis();
    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.suppress_sync = 1;  // batch write — suppress individual Spoolman syncs
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);

    // 1. Material type
    req.request_id = base_id;
    req.type = NFCWriteType::CHANGE_FILAMENT_TYPE;
    req.data.new_material_type = mat_type;
    NFCManager::getInstance().enqueueWrite(req);

    // 2. Initial weight
    req.request_id = base_id + 1;
    req.type = NFCWriteType::SET_INITIAL_WEIGHT;
    req.data.consumed_weight = initial_weight_g;  // consumed_weight field reused for initial
    NFCManager::getInstance().enqueueWrite(req);

    // 3. Color
    req.request_id = base_id + 2;
    req.type = NFCWriteType::CHANGE_COLOR;
    memcpy(req.data.new_color, color, 4);
    NFCManager::getInstance().enqueueWrite(req);

    // 4. Manufacturer
    req.request_id = base_id + 3;
    req.type = NFCWriteType::SET_BRAND_NAME;
    strncpy(req.data.brand_name, mfr, sizeof(req.data.brand_name) - 1);
    NFCManager::getInstance().enqueueWrite(req);

    // 5. Consumed weight (sets remaining)
    req.request_id = base_id + 4;
    req.type = NFCWriteType::SET_CONSUMED_WEIGHT;
    req.data.consumed_weight = consumed_g;
    NFCManager::getInstance().enqueueWrite(req);

    // 6. Spoolman ID (optional)
    if (spoolman_id > 0) {
        req.request_id = base_id + 5;
        req.type = NFCWriteType::WRITE_SPOOLMAN_ID;
        req.data.spoolman_id = spoolman_id;
        NFCManager::getInstance().enqueueWrite(req);
    }

    // 7. Density (optional)
    float density = doc["density"] | 0.0f;
    if (density > 0.0f) {
        req.request_id = base_id + 6;
        req.type = NFCWriteType::SET_DENSITY;
        req.data.float_value = density;
        NFCManager::getInstance().enqueueWrite(req);
    }

    // 8. Diameter (optional)
    float diameter_mm = doc["diameter_mm"] | 0.0f;
    if (diameter_mm > 0.0f) {
        req.request_id = base_id + 7;
        req.type = NFCWriteType::SET_DIAMETER;
        req.data.float_value = diameter_mm;
        NFCManager::getInstance().enqueueWrite(req);
    }

    // 9. Custom material name (optional)
    const char* mat_name = doc["material_name"] | "";
    if (mat_name[0] != '\0') {
        req.request_id = base_id + 8;
        req.type = NFCWriteType::SET_MATERIAL_NAME;
        strncpy(req.data.material_name, mat_name, sizeof(req.data.material_name) - 1);
        NFCManager::getInstance().enqueueWrite(req);
    }

    // 10-14. Temperatures (optional, 0 = not set)
    struct { const char* key; NFCWriteType type; uint32_t id; } temps[] = {
        { "min_print_temp", NFCWriteType::SET_MIN_PRINT_TEMP, base_id + 9  },
        { "max_print_temp", NFCWriteType::SET_MAX_PRINT_TEMP, base_id + 10 },
        { "preheat_temp",   NFCWriteType::SET_PREHEAT_TEMP,   base_id + 11 },
        { "min_bed_temp",   NFCWriteType::SET_MIN_BED_TEMP,   base_id + 12 },
        { "max_bed_temp",   NFCWriteType::SET_MAX_BED_TEMP,   base_id + 13 },
    };
    for (const auto& entry : temps) {
        int v = doc[entry.key] | 0;
        if (v != 0) {
            req.request_id = entry.id;
            req.type = entry.type;
            req.data.temp_celsius = static_cast<int16_t>(v);
            NFCManager::getInstance().enqueueWrite(req);
        }
    }

    _server.send(200, "application/json", "{\"success\":true}");
}

// ---------------------------------------------------------------------------
// API: Format Tag
// ---------------------------------------------------------------------------

void WebServerManager::handleApiFormatTag() {
    Serial.println("WebServerManager: POST /api/format-tag received");
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    // Optional body: {"uid": "..."} to validate tag before formatting
    char uid[17] = {0};
    if (_server.hasArg("plain") && _server.arg("plain").length() > 2) {
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, _server.arg("plain")) == DeserializationError::Ok) {
            const char* u = doc["uid"] | "";
            strncpy(uid, u, sizeof(uid) - 1);
        }
    }

    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.request_id = millis();
    req.type = NFCWriteType::FORMAT_NEW;
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);
    NFCManager::getInstance().enqueueWrite(req);

    _server.send(200, "application/json", "{\"success\":true}");
}

// ---------------------------------------------------------------------------
// API: Write TigerTag
// ---------------------------------------------------------------------------

void WebServerManager::handleApiWriteTigerTag() {
    Serial.println("WebServerManager: POST /api/write-tigertag received");
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }

    const char* uid = doc["uid"] | "";

    // Assemble 40-byte TigerTag binary layout (32 bytes data + 8 bytes padding)
    uint8_t payload[40];
    memset(payload, 0, sizeof(payload));

    // Version ID — V1.0 Maker (bytes 0-3)
    payload[0] = 0x5B; payload[1] = 0xF5; payload[2] = 0x92; payload[3] = 0x64;

    // Product ID — Maker/Offline (bytes 4-7)
    payload[4] = 0xFF; payload[5] = 0xFF; payload[6] = 0xFF; payload[7] = 0xFF;

    // Material ID (big-endian uint16, bytes 8-9)
    uint16_t materialId = doc["material_id"] | (uint16_t)38219;
    payload[8] = (materialId >> 8) & 0xFF;
    payload[9] = materialId & 0xFF;

    // Aspect IDs (bytes 10-11)
    payload[10] = doc["aspect1_id"] | (uint8_t)255;
    payload[11] = doc["aspect2_id"] | (uint8_t)255;

    // Type ID (byte 12) — 0x8E = Filament
    payload[12] = 0x8E;

    // Diameter ID (byte 13) — 56 = 1.75mm
    payload[13] = doc["diameter_id"] | (uint8_t)56;

    // Brand ID (big-endian uint16, bytes 14-15)
    uint16_t brandId = doc["brand_id"] | (uint16_t)65535;
    payload[14] = (brandId >> 8) & 0xFF;
    payload[15] = brandId & 0xFF;

    // Color RGBA (bytes 16-19)
    payload[16] = doc["color_r"] | (uint8_t)255;
    payload[17] = doc["color_g"] | (uint8_t)255;
    payload[18] = doc["color_b"] | (uint8_t)255;
    payload[19] = doc["color_a"] | (uint8_t)255;

    // Weight (big-endian 3 bytes, bytes 20-22)
    uint32_t weightG = doc["weight_g"] | (uint32_t)1000;
    payload[20] = (weightG >> 16) & 0xFF;
    payload[21] = (weightG >> 8) & 0xFF;
    payload[22] = weightG & 0xFF;

    // Unit ID (byte 23) — 21 = grams
    payload[23] = 21;

    // Nozzle temps (big-endian uint16, bytes 24-27)
    uint16_t nozzleMin = doc["nozzle_min"] | (uint16_t)0;
    uint16_t nozzleMax = doc["nozzle_max"] | (uint16_t)0;
    payload[24] = (nozzleMin >> 8) & 0xFF;
    payload[25] = nozzleMin & 0xFF;
    payload[26] = (nozzleMax >> 8) & 0xFF;
    payload[27] = nozzleMax & 0xFF;

    // Dry temp/time (bytes 28-29)
    payload[28] = doc["dry_temp"] | (uint8_t)0;
    payload[29] = doc["dry_time"] | (uint8_t)0;

    // Bed temps (bytes 30-31)
    payload[30] = doc["bed_min"] | (uint8_t)0;
    payload[31] = doc["bed_max"] | (uint8_t)0;

    // Enqueue write request
    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.request_id = millis();
    req.type = NFCWriteType::WRITE_TIGERTAG;
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);
    memcpy(req.data.tigertag_data, payload, 40);
    NFCManager::getInstance().enqueueWrite(req);

    _server.send(200, "application/json", "{\"success\":true}");
}

// ---------------------------------------------------------------------------
// API: Write OpenTag3D
// ---------------------------------------------------------------------------

void WebServerManager::handleApiWriteOpenTag3D() {
    Serial.println("WebServerManager: POST /api/write-opentag3d received");
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        sendError(400, "Invalid JSON");
        return;
    }

    const char* uid = doc["uid"] | "";

    opentag3d_t ot3d;
    memset(&ot3d, 0, sizeof(ot3d));

    ot3d.tag_version = doc["tag_version"] | (uint16_t)OT3D_SUPPORTED_VERSION;

    const char* baseMat = doc["base_material"] | "PLA";
    strncpy(ot3d.base_material, baseMat, sizeof(ot3d.base_material) - 1);

    const char* modifiers = doc["material_modifiers"] | "";
    strncpy(ot3d.material_modifiers, modifiers, sizeof(ot3d.material_modifiers) - 1);

    const char* mfr = doc["manufacturer"] | "";
    strncpy(ot3d.manufacturer, mfr, sizeof(ot3d.manufacturer) - 1);

    const char* colorName = doc["color_name"] | "";
    strncpy(ot3d.color_name, colorName, sizeof(ot3d.color_name) - 1);

    ot3d.color_rgba[0][0] = doc["color_r"] | (uint8_t)0;
    ot3d.color_rgba[0][1] = doc["color_g"] | (uint8_t)0;
    ot3d.color_rgba[0][2] = doc["color_b"] | (uint8_t)0;
    ot3d.color_rgba[0][3] = doc["color_a"] | (uint8_t)255;

    ot3d.diameter_um = doc["diameter_um"] | (uint16_t)1750;
    ot3d.target_weight_g = doc["target_weight_g"] | (uint16_t)1000;

    uint16_t printTemp = doc["print_temp_c"] | (uint16_t)0;
    uint16_t bedTemp = doc["bed_temp_c"] | (uint16_t)0;
    ot3d.print_temp_encoded = (uint8_t)(printTemp / 5);
    ot3d.bed_temp_encoded = (uint8_t)(bedTemp / 5);

    ot3d.density_ugcm3 = doc["density_ugcm3"] | (uint16_t)0;
    ot3d.transmission_distance = doc["transmission_distance"] | (uint16_t)0;

    if (doc.containsKey("serial_number") || doc.containsKey("min_print_temp_c")) {
        ot3d.has_extended = 1;

        const char* serial = doc["serial_number"] | "";
        strncpy(ot3d.serial_number, serial, sizeof(ot3d.serial_number) - 1);

        const char* url = doc["online_url"] | "";
        strncpy(ot3d.online_url, url, sizeof(ot3d.online_url) - 1);

        ot3d.manufacture_year = doc["manufacture_year"] | (uint16_t)0;
        ot3d.manufacture_month = doc["manufacture_month"] | (uint8_t)0;
        ot3d.manufacture_day = doc["manufacture_day"] | (uint8_t)0;

        ot3d.empty_spool_weight_g = doc["empty_spool_weight_g"] | (uint16_t)0;
        ot3d.measured_filament_weight_g = doc["measured_filament_weight_g"] | (uint16_t)0;
        ot3d.measured_filament_length_m = doc["measured_filament_length_m"] | (uint16_t)0;

        uint16_t maxDryTemp = doc["max_dry_temp_c"] | (uint16_t)0;
        ot3d.max_dry_temp_encoded = (uint8_t)(maxDryTemp / 5);
        ot3d.dry_time_hours = doc["dry_time_hours"] | (uint8_t)0;

        uint16_t minPrint = doc["min_print_temp_c"] | (uint16_t)0;
        uint16_t maxPrint = doc["max_print_temp_c"] | (uint16_t)0;
        uint16_t minBed = doc["min_bed_temp_c"] | (uint16_t)0;
        uint16_t maxBed = doc["max_bed_temp_c"] | (uint16_t)0;
        ot3d.min_print_temp_encoded = (uint8_t)(minPrint / 5);
        ot3d.max_print_temp_encoded = (uint8_t)(maxPrint / 5);
        ot3d.min_bed_temp_encoded = (uint8_t)(minBed / 5);
        ot3d.max_bed_temp_encoded = (uint8_t)(maxBed / 5);

        ot3d.min_volumetric_speed = doc["min_volumetric_speed"] | (uint8_t)0;
        ot3d.max_volumetric_speed = doc["max_volumetric_speed"] | (uint8_t)0;
        ot3d.target_volumetric_speed = doc["target_volumetric_speed"] | (uint8_t)0;
    }

    NFCWriteRequest req;
    memset(&req, 0, sizeof(req));
    req.request_id = millis();
    req.type = NFCWriteType::WRITE_OPENTAG3D;
    strncpy(req.expected_spool_id, uid, sizeof(req.expected_spool_id) - 1);

    if (!NFCManager::getInstance().enqueueRawWrite(req, (const uint8_t*)&ot3d, sizeof(ot3d))) {
        sendError(503, "Write queue full or busy");
        return;
    }

    _server.send(200, "application/json", "{\"success\":true}");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void WebServerManager::sendError(int code, const char* msg) {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    char body[96];
    snprintf(body, sizeof(body), "{\"success\":false,\"error\":\"%s\"}", msg);
    _server.send(code, "application/json", body);
}

#endif // NATIVE_TEST
