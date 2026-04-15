#pragma once
#include <cstdint>

struct BambuTagData {
    bool valid = false;
    char material_variant[16] = {};
    char filament_type[16] = {};
    uint8_t color_r = 0, color_g = 0, color_b = 0, color_a = 0;
    uint16_t weight_g = 0;
    float diameter_mm = 0.0f;
    uint16_t drying_temp = 0;
    uint16_t drying_time = 0;
    uint16_t bed_temp = 0;
    uint16_t hotend_min = 0;
    uint16_t hotend_max = 0;
    char production_date[20] = {};
    uint32_t filament_length_m = 0;
    uint8_t color_extended[16] = {};
};

static constexpr uint8_t BAMBU_BLOCKS[] = { 1, 2, 4, 5, 6, 13, 14, 16 };
static constexpr uint8_t BAMBU_BLOCK_COUNT = 8;

bool parseBambuBlocks(const uint8_t blocks[][16], BambuTagData& out);
