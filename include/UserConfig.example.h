#pragma once

/*
   Copy this file to UserConfig.h and fill in your settings.

   Example:
   cp include/UserConfig.example.h include/UserConfig.h

   Do NOT commit UserConfig.h to git because it contains
   WiFi credentials and environment-specific configuration.
*/

#define DEVICE_NAME "SpoolSenseScanner"

/* WIFI (REQUIRED) */
#define WIFI_SSID "your_wifi"
#define WIFI_PASSWORD "your_password"

/* MQTT (REQUIRED) */
#define MQTT_HOST "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define MQTT_TOPIC_PREFIX "spoolsense"

/* Optional: Spoolman (1 = enabled, 0 = disabled) */
#define SPOOLMAN_ENABLED 0
#define SPOOLMAN_URL "http://spoolman.local:7912"

/* Automation mode: 0 = Self Directed (scanner auto-deducts), 1 = Controlled by HA */
#define AUTOMATION_MODE 0

/* Optional hardware features */
#define ENABLE_LCD 0
#define ENABLE_STATUS_LED 1

/* Board selection: uncomment ONE of the following */
#define BOARD_ESP32_WROOM
// #define BOARD_ESP32_S3