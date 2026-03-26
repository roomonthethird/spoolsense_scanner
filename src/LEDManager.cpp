#include "LEDManager.h"
#include "UserConfig.h"

#include <cstdlib>
#include <cmath>

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#if defined(BOARD_ESP32_S3)
LEDManager::LEDManager()
    : _initialized(false), _taskStarted(false), _pixel(1, 0, NEO_RGB + NEO_KHZ800) {}
#else
LEDManager::LEDManager()
    : _initialized(false), _taskStarted(false), _pixel(1, 0, NEO_GRBW + NEO_KHZ800) {}
#endif

void LEDManager::begin(uint8_t pin) {
#if defined(BOARD_ESP32_S3)
    _pixel.updateType(NEO_RGB + NEO_KHZ800);
#else
    _pixel.updateType(NEO_GRBW + NEO_KHZ800);
#endif
    _pixel.setPin(pin);
    _pixel.begin();
    _pixel.setBrightness(64);
    _pixel.show();
    _initialized = true;
}

void LEDManager::startTask() {
    if (!_initialized) return;
#ifndef NATIVE_TEST
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) return;
    xTaskCreatePinnedToCore(ledTaskFunc, "LEDTask", 2048, this, 1, &_taskHandle, 1);
    _taskStarted = true;
#endif
}

// ---------------------------------------------------------------------------
// Synchronous methods — direct pixel writes used during setup() before task
// ---------------------------------------------------------------------------

void LEDManager::showBooting() {
    if (!_initialized) return;
#if defined(BOARD_ESP32_S3)
    _pixel.setPixelColor(0, _pixel.Color(255, 255, 255));  // RGB white
#else
    _pixel.setPixelColor(0, _pixel.Color(0, 0, 0, 255));   // RGBW white via W channel
#endif
    _pixel.show();
}

void LEDManager::showWifiConnected() {
    if (!_initialized) return;
    _pixel.setPixelColor(0, _pixel.Color(0, 255, 255, 0));  // cyan
    _pixel.show();
}

void LEDManager::showWifiFailed() {
    if (!_initialized) return;
    _pixel.setPixelColor(0, _pixel.Color(255, 0, 0, 0));
    _pixel.show();
}

void LEDManager::showReady() {
    if (!_initialized) return;
    _pixel.setPixelColor(0, _pixel.Color(0, 0, 255, 0));  // blue
    _pixel.show();
}

// ---------------------------------------------------------------------------
// Async state methods — set target, task applies it
// ---------------------------------------------------------------------------

void LEDManager::showOff() {
    if (!_initialized) return;
    setTarget(LEDMode::OFF, 0, 0, 0);
}

void LEDManager::showFilamentColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!_initialized) return;
    setTarget(LEDMode::SOLID, r, g, b);
}

void LEDManager::breatheFilamentColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!_initialized) return;
    setTarget(LEDMode::BREATHING, r, g, b);
}

// ---------------------------------------------------------------------------
// Flash methods — queue a flash, task runs it then restores target
// ---------------------------------------------------------------------------

void LEDManager::flashTagDetected() {
    if (!_initialized) return;
    requestFlash(FlashType::TAG_DETECTED);
}

void LEDManager::flashParseFailed() {
    if (!_initialized) return;
    requestFlash(FlashType::PARSE_FAILED);
}

void LEDManager::flashWriteSuccess() {
    if (!_initialized) return;
    requestFlash(FlashType::WRITE_SUCCESS);
}

void LEDManager::flashWriteFailure() {
    if (!_initialized) return;
    requestFlash(FlashType::WRITE_FAILURE);
}

void LEDManager::flashWarning() {
    if (!_initialized) return;
    requestFlash(FlashType::WARNING);
}

// ---------------------------------------------------------------------------
// Hex utility
// ---------------------------------------------------------------------------

void LEDManager::setFilamentColorFromHex(const char* hex) {
    if (!_initialized || !hex) return;
    if (hex[0] == '#') hex++;
    char* endptr = nullptr;
    unsigned long color = strtoul(hex, &endptr, 16);
    if (endptr == hex) return;
    showFilamentColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void LEDManager::setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
    _pixel.setPixelColor(0, _pixel.Color(r, g, b, 0));
    _pixel.show();
}

void LEDManager::setPixelOff() {
    _pixel.setPixelColor(0, _pixel.Color(0, 0, 0, 0));
    _pixel.show();
}

void LEDManager::setTarget(LEDMode mode, uint8_t r, uint8_t g, uint8_t b) {
#ifndef NATIVE_TEST
    if (_taskStarted && _mutex) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        _target.mode = mode;
        _target.r = r;
        _target.g = g;
        _target.b = b;
        xSemaphoreGive(_mutex);
        if (_taskHandle) xTaskNotifyGive(_taskHandle);
        return;
    }
#endif
    // Pre-task: write directly
    _target.mode = mode;
    _target.r = r;
    _target.g = g;
    _target.b = b;
    if (mode == LEDMode::OFF) {
        setPixelOff();
    } else {
        setPixelColor(r, g, b);
    }
}

void LEDManager::requestFlash(FlashType type) {
#ifndef NATIVE_TEST
    if (_taskStarted && _mutex) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        _flashReq.type = type;
        _flashReq.pending = true;
        xSemaphoreGive(_mutex);
        if (_taskHandle) xTaskNotifyGive(_taskHandle);
        return;
    }
#endif
    // Pre-task fallback: blocking flash
    switch (type) {
        case FlashType::TAG_DETECTED:
            for (int i = 0; i < 3; i++) {
                setPixelColor(255, 255, 255);
                delay(80);
                setPixelOff();
                delay(80);
            }
            break;
        case FlashType::PARSE_FAILED:
            setPixelColor(255, 0, 0);
            delay(100);
            setPixelOff();
            break;
        case FlashType::WRITE_SUCCESS:
            setPixelColor(0, 255, 0);
            delay(100);
            break;
        case FlashType::WRITE_FAILURE:
            setPixelColor(255, 0, 0);
            delay(100);
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// FreeRTOS task
// ---------------------------------------------------------------------------

#ifndef NATIVE_TEST

void LEDManager::ledTaskFunc(void* param) {
    static_cast<LEDManager*>(param)->ledTaskLoop();
}

void LEDManager::runFlash(FlashType type) {
    switch (type) {
        case FlashType::TAG_DETECTED:
            for (int i = 0; i < 3; i++) {
                setPixelColor(255, 255, 255);
                vTaskDelay(pdMS_TO_TICKS(80));
                setPixelOff();
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            break;
        case FlashType::PARSE_FAILED:
            setPixelColor(255, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            setPixelOff();
            break;
        case FlashType::WRITE_SUCCESS:
            setPixelColor(0, 255, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case FlashType::WRITE_FAILURE:
            setPixelColor(255, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case FlashType::WARNING:
            for (int i = 0; i < 3; i++) {
                setPixelColor(255, 180, 0);  // yellow/amber
                vTaskDelay(pdMS_TO_TICKS(150));
                setPixelOff();
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            break;
        default:
            break;
    }
}

void LEDManager::ledTaskLoop() {
    for (;;) {
        // Determine wait time based on current mode
        bool isBreathing;
        {
            xSemaphoreTake(_mutex, portMAX_DELAY);
            isBreathing = (_target.mode == LEDMode::BREATHING) && !_flashReq.pending;
            xSemaphoreGive(_mutex);
        }

        // Block until notification or 10ms tick (for breathing animation)
        ulTaskNotifyTake(pdTRUE, isBreathing ? pdMS_TO_TICKS(10) : portMAX_DELAY);

        // Read state
        xSemaphoreTake(_mutex, portMAX_DELAY);
        bool hasFlash = _flashReq.pending;
        FlashType flashType = _flashReq.type;
        if (hasFlash) _flashReq.pending = false;
        Target target = _target;
        xSemaphoreGive(_mutex);

        // Execute flash first, then re-read target
        if (hasFlash) {
            runFlash(flashType);
            xSemaphoreTake(_mutex, portMAX_DELAY);
            target = _target;
            xSemaphoreGive(_mutex);
        }

        // Apply target state
        switch (target.mode) {
            case LEDMode::OFF:
                setPixelOff();
                break;
            case LEDMode::SOLID:
                setPixelColor(target.r, target.g, target.b);
                break;
            case LEDMode::BREATHING: {
                // Advance one step of the sine-wave breath
                // 200 steps × 10ms wait = 2s per full breath cycle
                static constexpr uint16_t BREATH_STEPS = 200;
                float t = sinf((float)_breathStep / (float)BREATH_STEPS * (float)M_PI);
                float brightness = t * t;  // t² for smoother fade shape
                setPixelColor(
                    (uint8_t)(target.r * brightness),
                    (uint8_t)(target.g * brightness),
                    (uint8_t)(target.b * brightness)
                );
                _breathStep = (_breathStep + 1) % BREATH_STEPS;
                break;
            }
        }
    }
}

#endif
