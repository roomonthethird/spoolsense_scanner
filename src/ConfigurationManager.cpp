#include "ConfigurationManager.h"
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
