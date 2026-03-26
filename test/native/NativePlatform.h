#pragma once

// Stubs for Arduino/FreeRTOS when building native tests.
// These allow ApplicationManager, PrinterManager, etc. to compile on macOS/Linux.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// --- Arduino stubs ---
static inline unsigned long millis() {
    static unsigned long _ms = 0;
    return _ms;
}
static inline void advance_millis(unsigned long ms) {
    // Access millis() internal counter
    // For native tests we use a global
}

static inline void delay(unsigned long) {}

// Serial stub
struct SerialStub {
    template<typename T> void print(T) {}
    template<typename T> void println(T) {}
    void println() {}
    void printf(const char*, ...) __attribute__((format(printf, 2, 3)));
    void begin(int) {}
};
inline void SerialStub::printf(const char*, ...) {}
static SerialStub Serial;

// --- FreeRTOS stubs ---
typedef void* SemaphoreHandle_t;
typedef void* QueueHandle_t;
typedef void* TaskHandle_t;
typedef unsigned int UBaseType_t;
typedef int BaseType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0xFFFFFFFF

static inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, uint32_t) { return pdTRUE; }
static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
static inline QueueHandle_t xQueueCreate(UBaseType_t, UBaseType_t) { return (QueueHandle_t)1; }
static inline BaseType_t xQueueSend(QueueHandle_t, const void*, uint32_t) { return pdTRUE; }
static inline BaseType_t xQueueReceive(QueueHandle_t, void*, uint32_t) { return pdFALSE; }
static inline void vQueueDelete(QueueHandle_t) {}
static inline void vTaskDelay(uint32_t) {}

static inline int xTaskCreate(void(*)(void*), const char*, size_t, void*, UBaseType_t, TaskHandle_t*) {
    return pdTRUE;
}
static inline int xTaskCreatePinnedToCore(void(*)(void*), const char*, size_t, void*, UBaseType_t, TaskHandle_t*, int) {
    return pdTRUE;
}
