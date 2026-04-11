#pragma once
#include <stdint.h>

static const uint8_t MAX_TRAYS = 16;

struct TrayData {
    uint8_t tray_index;    // 0-15 AMS, 16-23 AMS-HT, 254/255 external
    uint8_t ams_id;        // for optional visual grouping later
    char material[8];      // "PLA", "PETG", "ASA", etc.
    uint8_t color[3];      // RGB
    uint16_t weight_g;     // remaining weight in grams
    bool populated;        // true if tray has a spool
    char uid[17];          // SpoolSense NFC tag UID hex (up to 16 chars + null)
    int32_t spoolman_id;   // Spoolman spool ID (-1 if unknown)
};

struct TrayDashboardState {
    TrayData trays[MAX_TRAYS];
    uint8_t tray_count;    // number of populated trays
    bool has_data;         // false until first cmd/tray_update received
};
