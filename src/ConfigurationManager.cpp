#include "ConfigurationManager.h"
#include "UserConfig.h"
#include "DeviceConfig.h"
#include <cstring>
#include <Arduino.h>

#ifndef NATIVE_TEST
#include <Preferences.h>
#endif

// NVS namespace and key names — must match spoolsense-installer nvs_keys.csv
static const char* NVS_NAMESPACE = "spoolsense";
static const char* NVS_KEY_WIFI_SSID      = "wifi_ssid";
static const char* NVS_KEY_WIFI_PASS      = "wifi_pass";
static const char* NVS_KEY_MQTT_HOST      = "mqtt_host";
static const char* NVS_KEY_MQTT_PORT      = "mqtt_port";
static const char* NVS_KEY_MQTT_USER      = "mqtt_user";
static const char* NVS_KEY_MQTT_PASS      = "mqtt_pass";
static const char* NVS_KEY_MQTT_PREFIX    = "mqtt_prefix";
static const char* NVS_KEY_SPOOLMAN_ON    = "spoolman_on";
static const char* NVS_KEY_SPOOLMAN_URL   = "spoolman_url";
static const char* NVS_KEY_AUTO_MODE      = "auto_mode";
static const char* NVS_KEY_LCD_ON         = "lcd_on";
static const char* NVS_KEY_LED_ON         = "led_on";
static const char* NVS_KEY_PRUSALINK_ON   = "prusalink_on";
static const char* NVS_KEY_PRUSALINK_URL  = "prusalink_url";
static const char* NVS_KEY_PRUSALINK_KEY  = "prusalink_key";

ConfigurationManager& ConfigurationManager::getInstance() {
    static ConfigurationManager instance;
    return instance;
}

bool ConfigurationManager::begin() {
    if (_initialized) {
        return true;
    }

    // Load compile-time defaults first, then override with NVS where present
    loadFromDeviceConfig();

#ifndef NATIVE_TEST
    if (loadFromNVS()) {
        Serial.println("ConfigurationManager: NVS config found, overrides applied");
    } else {
        Serial.println("ConfigurationManager: No NVS config, using compile-time defaults");
    }
#endif

    _initialized = true;
    return true;
}

void ConfigurationManager::loadFromDeviceConfig() {
    const DeviceConfig& cfg = getDeviceConfig();

    strncpy(_ssid, cfg.wifi.ssid, sizeof(_ssid) - 1);
    _ssid[sizeof(_ssid) - 1] = '\0';

    strncpy(_wifiPass, cfg.wifi.password, sizeof(_wifiPass) - 1);
    _wifiPass[sizeof(_wifiPass) - 1] = '\0';

    _spoolmanEnabled = cfg.spoolman.enabled;
    if (_spoolmanEnabled) {
        strncpy(_spoolmanUrl, cfg.spoolman.base_url, sizeof(_spoolmanUrl) - 1);
    } else {
        _spoolmanUrl[0] = '\0';
    }
    _spoolmanUrl[sizeof(_spoolmanUrl) - 1] = '\0';

    _pollIntervalMs = 10000;
    _lcdTimeoutMs = 15 * 60 * 1000;  // 15 mins

    const bool haConfigured = cfg.mqtt.host != nullptr && cfg.mqtt.host[0] != '\0';
    _haEnabled = haConfigured;

    if (haConfigured) {
        strncpy(_haMqttHost, cfg.mqtt.host, sizeof(_haMqttHost) - 1);
    } else {
        _haMqttHost[0] = '\0';
    }
    _haMqttHost[sizeof(_haMqttHost) - 1] = '\0';

    _haMqttPort = static_cast<uint16_t>(cfg.mqtt.port);

    strncpy(_haMqttUser, cfg.mqtt.username, sizeof(_haMqttUser) - 1);
    _haMqttUser[sizeof(_haMqttUser) - 1] = '\0';

    strncpy(_haMqttPass, cfg.mqtt.password, sizeof(_haMqttPass) - 1);
    _haMqttPass[sizeof(_haMqttPass) - 1] = '\0';

    _automationMode = cfg.automation_mode;

    // PrusaLink defaults — not in DeviceConfig, disabled by default
    _prusaLinkEnabled = false;
    _prusaLinkUrl[0] = '\0';
    _prusaLinkApiKey[0] = '\0';
}

#ifndef NATIVE_TEST
bool ConfigurationManager::loadFromNVS() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {  // read-only
        return false;
    }

    // Check if NVS has any SpoolSense config at all
    if (!prefs.isKey(NVS_KEY_WIFI_SSID)) {
        prefs.end();
        return false;
    }

    bool anyOverride = false;

    // Per-key fallback: only override if the key exists in NVS
    if (prefs.isKey(NVS_KEY_WIFI_SSID)) {
        prefs.getString(NVS_KEY_WIFI_SSID, _ssid, sizeof(_ssid));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_WIFI_PASS)) {
        prefs.getString(NVS_KEY_WIFI_PASS, _wifiPass, sizeof(_wifiPass));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_MQTT_HOST)) {
        prefs.getString(NVS_KEY_MQTT_HOST, _haMqttHost, sizeof(_haMqttHost));
        _haEnabled = (_haMqttHost[0] != '\0');
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_MQTT_PORT)) {
        _haMqttPort = prefs.getUShort(NVS_KEY_MQTT_PORT, _haMqttPort);
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_MQTT_USER)) {
        prefs.getString(NVS_KEY_MQTT_USER, _haMqttUser, sizeof(_haMqttUser));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_MQTT_PASS)) {
        prefs.getString(NVS_KEY_MQTT_PASS, _haMqttPass, sizeof(_haMqttPass));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_SPOOLMAN_ON)) {
        _spoolmanEnabled = prefs.getBool(NVS_KEY_SPOOLMAN_ON, _spoolmanEnabled);
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_SPOOLMAN_URL)) {
        prefs.getString(NVS_KEY_SPOOLMAN_URL, _spoolmanUrl, sizeof(_spoolmanUrl));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_AUTO_MODE)) {
        _automationMode = prefs.getUChar(NVS_KEY_AUTO_MODE, _automationMode);
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_PRUSALINK_ON)) {
        _prusaLinkEnabled = prefs.getBool(NVS_KEY_PRUSALINK_ON, false);
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_PRUSALINK_URL)) {
        prefs.getString(NVS_KEY_PRUSALINK_URL, _prusaLinkUrl, sizeof(_prusaLinkUrl));
        anyOverride = true;
    }
    if (prefs.isKey(NVS_KEY_PRUSALINK_KEY)) {
        prefs.getString(NVS_KEY_PRUSALINK_KEY, _prusaLinkApiKey, sizeof(_prusaLinkApiKey));
        anyOverride = true;
    }

    prefs.end();
    return anyOverride;
}
#endif

const char* ConfigurationManager::getWiFiSSID() const {
    return _ssid;
}

const char* ConfigurationManager::getWiFiPassword() const {
    return _wifiPass;
}

const char* ConfigurationManager::getSpoolmanURL() const {
    return _spoolmanUrl;
}

bool ConfigurationManager::isSpoolmanEnabled() const {
    return _spoolmanEnabled;
}

uint32_t ConfigurationManager::getPollIntervalMs() const {
    return _pollIntervalMs;
}

uint32_t ConfigurationManager::getLcdTimeoutMs() const {
    return _lcdTimeoutMs;
}

bool ConfigurationManager::getHAEnabled() const {
    return _haEnabled;
}

const char* ConfigurationManager::getHAMqttHost() const {
    return _haMqttHost;
}

uint16_t ConfigurationManager::getHAMqttPort() const {
    return _haMqttPort;
}

const char* ConfigurationManager::getHAMqttUser() const {
    return _haMqttUser;
}

const char* ConfigurationManager::getHAMqttPass() const {
    return _haMqttPass;
}

uint8_t ConfigurationManager::getAutomationMode() const {
    return _automationMode;
}

bool ConfigurationManager::isPrusaLinkEnabled() const {
    return _prusaLinkEnabled && _prusaLinkUrl[0] != '\0' && _prusaLinkApiKey[0] != '\0';
}

const char* ConfigurationManager::getPrusaLinkURL() const {
    return _prusaLinkUrl;
}

const char* ConfigurationManager::getPrusaLinkAPIKey() const {
    return _prusaLinkApiKey;
}

void ConfigurationManager::getCurrentConfig(ConfigUpdate& out) const {
    memset(&out, 0, sizeof(out));
    strncpy(out.wifi_ssid, _ssid, sizeof(out.wifi_ssid) - 1);
    strncpy(out.wifi_pass, _wifiPass, sizeof(out.wifi_pass) - 1);
    strncpy(out.mqtt_host, _haMqttHost, sizeof(out.mqtt_host) - 1);
    out.mqtt_port = _haMqttPort;
    strncpy(out.mqtt_user, _haMqttUser, sizeof(out.mqtt_user) - 1);
    strncpy(out.mqtt_pass, _haMqttPass, sizeof(out.mqtt_pass) - 1);
    out.spoolman_on = _spoolmanEnabled ? 1 : 0;
    strncpy(out.spoolman_url, _spoolmanUrl, sizeof(out.spoolman_url) - 1);
    out.auto_mode = _automationMode;
    out.prusalink_on = _prusaLinkEnabled ? 1 : 0;
    strncpy(out.prusalink_url, _prusaLinkUrl, sizeof(out.prusalink_url) - 1);
    strncpy(out.prusalink_api_key, _prusaLinkApiKey, sizeof(out.prusalink_api_key) - 1);
#if defined(ENABLE_LCD) && ENABLE_LCD
    out.lcd_enabled = 1;
#else
    out.lcd_enabled = 0;
#endif
#if defined(ENABLE_STATUS_LED) && ENABLE_STATUS_LED
    out.led_enabled = 1;
#else
    out.led_enabled = 0;
#endif
}

#ifndef NATIVE_TEST
bool ConfigurationManager::saveToNVS(const ConfigUpdate& update) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {  // read-write
        Serial.println("ConfigurationManager: Failed to open NVS for writing");
        return false;
    }

    prefs.putString(NVS_KEY_WIFI_SSID, update.wifi_ssid);
    // Only write password if non-empty (empty = keep existing)
    if (update.wifi_pass[0] != '\0') {
        prefs.putString(NVS_KEY_WIFI_PASS, update.wifi_pass);
    }
    prefs.putString(NVS_KEY_MQTT_HOST, update.mqtt_host);
    prefs.putUShort(NVS_KEY_MQTT_PORT, update.mqtt_port);
    prefs.putString(NVS_KEY_MQTT_USER, update.mqtt_user);
    if (update.mqtt_pass[0] != '\0') {
        prefs.putString(NVS_KEY_MQTT_PASS, update.mqtt_pass);
    }
    prefs.putBool(NVS_KEY_SPOOLMAN_ON, update.spoolman_on != 0);
    prefs.putString(NVS_KEY_SPOOLMAN_URL, update.spoolman_url);
    prefs.putUChar(NVS_KEY_AUTO_MODE, update.auto_mode);
    prefs.putUChar(NVS_KEY_LCD_ON, update.lcd_enabled);
    prefs.putUChar(NVS_KEY_LED_ON, update.led_enabled);
    prefs.putBool(NVS_KEY_PRUSALINK_ON, update.prusalink_on != 0);
    prefs.putString(NVS_KEY_PRUSALINK_URL, update.prusalink_url);
    if (update.prusalink_api_key[0] != '\0') {
        prefs.putString(NVS_KEY_PRUSALINK_KEY, update.prusalink_api_key);
    }

    prefs.end();
    Serial.println("ConfigurationManager: Config saved to NVS");
    return true;
}
#else
bool ConfigurationManager::saveToNVS(const ConfigUpdate&) {
    return true;  // No-op in native tests
}
#endif
