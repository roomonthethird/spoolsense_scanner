#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <cstdint>

#define DEVICE_VERSION "0.76 BETA"

class ConfigurationManager {
public:
    static ConfigurationManager& getInstance();

    bool begin();  // Load compile-time config from DeviceConfig

    const char* getWiFiSSID() const;
    const char* getWiFiPassword() const;
    const char* getSpoolmanURL() const;
    bool isSpoolmanEnabled() const;
    uint32_t getPollIntervalMs() const;
    uint32_t getLcdTimeoutMs() const;

    // Home Assistant / MQTT configuration
    bool getHAEnabled() const;
    const char* getHAMqttHost() const;
    uint16_t getHAMqttPort() const;
    const char* getHAMqttUser() const;
    const char* getHAMqttPass() const;
    uint8_t getAutomationMode() const;

private:
    ConfigurationManager() = default;
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    // In-memory cache (loaded from DeviceConfig at boot)
    char _ssid[64];
    char _wifiPass[64];
    char _spoolmanUrl[128];
    bool _spoolmanEnabled;
    uint32_t _pollIntervalMs;
    uint32_t _lcdTimeoutMs;

    // Home Assistant / MQTT config
    bool _haEnabled;
    char _haMqttHost[128];
    uint16_t _haMqttPort;
    char _haMqttUser[64];
    char _haMqttPass[64];
    uint8_t _automationMode;

    bool _initialized = false;
};

#endif // CONFIGURATION_MANAGER_H
