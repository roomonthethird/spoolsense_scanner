#pragma once
#include <cstdint>

struct BambuKeys {
    uint8_t keys[96];
    const uint8_t* sectorKey(uint8_t sector) const { return &keys[sector * 6]; }
    const uint8_t* blockKey(uint8_t block) const { return sectorKey(block / 4); }
};

BambuKeys deriveBambuKeys(const uint8_t* uid, uint8_t uidLen);
