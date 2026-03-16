#pragma once

struct WifiConfig {
    const char* ssid;
    const char* password;
};

struct MqttConfig {
    const char* host;
    int port;
    const char* username;
    const char* password;
    const char* topic_prefix;
};

struct SpoolmanConfig {
    bool enabled;
    const char* base_url;
};

enum class BoardProfile {
    ESP32_WROOM,
    ESP32_S3
};

struct PeripheralConfig {
    bool lcd_enabled;
    bool status_led_enabled;
};

struct DeviceConfig {
    const char* device_name;
    BoardProfile board_profile;
    WifiConfig wifi;
    MqttConfig mqtt;
    SpoolmanConfig spoolman;
    PeripheralConfig peripherals;
};

const DeviceConfig& getDeviceConfig();