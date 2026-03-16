#include "WebServerManager.h"

#ifndef NATIVE_TEST

#include <Arduino.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "TagWriterHTML.h"
#include "NFCManager.h"
#include "NFCTypes.h"
#include "NFCWriteTypes.h"
#include "ApplicationManager.h"
#include "ConversionUtils.h"

extern "C" {
#include "openprinttag_lib.h"
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

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status",     HTTP_GET,  [this]() { handleApiStatus(); });
    _server.on("/api/write-tag",  HTTP_POST, [this]() { handleApiWriteTag(); });
    _server.on("/api/format-tag", HTTP_POST, [this]() { handleApiFormatTag(); });

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
// Route handlers
// ---------------------------------------------------------------------------

void WebServerManager::handleRoot() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send_P(200, "text/html", TAG_WRITER_HTML);
}

void WebServerManager::handleApiStatus() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");

    CurrentSpoolState state;
    StaticJsonDocument<768> doc;

    if (NFCManager::getInstance().getCurrentSpoolState(state) && state.present) {
        doc["present"] = true;
        doc["uid"] = state.spool_id;
        doc["tag_data_valid"] = state.tag_data_valid;

        if (state.tag_data_valid) {
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

void WebServerManager::handleApiWriteTag() {
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

void WebServerManager::handleApiFormatTag() {
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
// Helpers
// ---------------------------------------------------------------------------

void WebServerManager::sendError(int code, const char* msg) {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    char body[96];
    snprintf(body, sizeof(body), "{\"success\":false,\"error\":\"%s\"}", msg);
    _server.send(code, "application/json", body);
}

#endif // NATIVE_TEST
