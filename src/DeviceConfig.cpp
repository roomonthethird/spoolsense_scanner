

#include "DeviceConfig.h"
#include "UserConfig.h"

static const DeviceConfig kDeviceConfig = {
    .device_name = DEVICE_NAME,

    .board_profile =
#if BOARD_PROFILE == BOARD_PROFILE_ESP32_S3
        BoardProfile::ESP32_S3,
#else
        BoardProfile::ESP32_WROOM,
#endif

    .wifi = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    },

    .mqtt = {
        .host = MQTT_HOST,
        .port = MQTT_PORT,
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD,
        .topic_prefix = MQTT_TOPIC_PREFIX,
    },

    .spoolman = {
        .enabled = SPOOLMAN_ENABLED,
        .base_url = SPOOLMAN_URL,
    },

    .peripherals = {
        .lcd_enabled = ENABLE_LCD,
        .status_led_enabled = ENABLE_STATUS_LED,
    }
};

const DeviceConfig& getDeviceConfig()
{
    return kDeviceConfig;
}