#pragma once

#include <cstdint>

// Display interface — implemented by LCDManager and TFTManager.
// ApplicationManager calls these methods without knowing which display is attached.
class DisplayI {
public:
    virtual ~DisplayI() = default;

    // Boot / status
    virtual void showText(const char* line1, const char* line2 = nullptr) = 0;
    virtual void showText4(const char* line1, const char* line2,
                           const char* line3, const char* line4) = 0;

    // Screen timeout
    virtual void setScreenTimeoutMs(uint32_t timeoutMs) = 0;
};
