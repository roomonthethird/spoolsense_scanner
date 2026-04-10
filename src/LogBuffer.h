#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Circular log buffer that captures Serial output for web UI viewing.
// Thread-safe — safe to call from any FreeRTOS task.
class LogBuffer {
public:
    static LogBuffer& getInstance();

    // Write formatted output to both Serial and the ring buffer
    void logPrintf(const char* format, ...) __attribute__((format(printf, 2, 3)));

    // Copy current buffer contents into a pre-allocated char array.
    // Returns number of bytes written (excluding null terminator).
    size_t getLog(char* out, size_t outSize);

    void clear();

private:
    static constexpr size_t BUFFER_SIZE = 4096;

    char buffer_[BUFFER_SIZE];
    size_t head_ = 0;
    size_t count_ = 0;
    SemaphoreHandle_t mutex_;

    LogBuffer();
    LogBuffer(const LogBuffer&) = delete;
    LogBuffer& operator=(const LogBuffer&) = delete;
    void append(const char* data, size_t len);
};

#endif // LOG_BUFFER_H
