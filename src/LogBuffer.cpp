#include "LogBuffer.h"
#include <stdarg.h>
#include <cstring>

LogBuffer::LogBuffer() {
    mutex_ = xSemaphoreCreateMutex();
    memset(buffer_, 0, BUFFER_SIZE);
}

LogBuffer& LogBuffer::getInstance() {
    static LogBuffer instance;
    return instance;
}

void LogBuffer::append(const char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer_[head_] = data[i];
        head_ = (head_ + 1) % BUFFER_SIZE;
        if (count_ < BUFFER_SIZE) count_++;
    }
}

void LogBuffer::logPrintf(const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    if (len <= 0) return;
    if ((size_t)len >= sizeof(temp)) len = sizeof(temp) - 1;

    // Write to Serial
    Serial.print(temp);

    // Write to ring buffer under mutex
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        append(temp, len);
        xSemaphoreGive(mutex_);
    }
}

size_t LogBuffer::getLog(char* out, size_t outSize) {
    if (!out || outSize == 0) return 0;

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
        size_t available = (count_ < outSize - 1) ? count_ : outSize - 1;
        // Oldest data starts at (head_ - count_) wrapped
        size_t start = (head_ + BUFFER_SIZE - count_) % BUFFER_SIZE;

        for (size_t i = 0; i < available; i++) {
            out[i] = buffer_[(start + i) % BUFFER_SIZE];
        }
        out[available] = '\0';
        xSemaphoreGive(mutex_);
        return available;
    }
    out[0] = '\0';
    return 0;
}

void LogBuffer::clear() {
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(50)) == pdTRUE) {
        head_ = 0;
        count_ = 0;
        xSemaphoreGive(mutex_);
    }
}
