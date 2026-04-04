#pragma once

#include <cstdint>
#include <cstddef>

struct OpenSpoolData {
    char protocol[16];
    char version[8];
    char material[32];
    char color_hex[8];    // "FFAABB" (no #)
    char brand[64];
    int16_t min_temp;
    int16_t max_temp;
    bool valid;
};

bool parseOpenSpool(const uint8_t* json, size_t len, OpenSpoolData& out);
