#include "ConfigurationManager.h"
#include "DeviceConfig.h"
#include <cstring>
#include <Arduino.h>

ConfigurationManager& ConfigurationManager::getInstance() {
    static ConfigurationManager instance;
    return instance;
}

bool ConfigurationManager::begin() {
    if (_initialized) {
        return true;
    }

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

    _initialized = true;
    Serial.println("ConfigurationManager: Initialized from compile-time config");
    return true;
}

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
