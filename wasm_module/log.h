// log.h - Simple logging macro with timestamp
#ifndef LOG_H
#define LOG_H

#include <chrono>
#include <cstdio>

// Global start time to track program start
static const auto program_start = std::chrono::steady_clock::now();

// Get milliseconds since program started
inline unsigned long get_time_ms() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - program_start);
    return duration.count() / 1000;
}

// TRACE macro that prints timestamp and formatted message
#define TRACE(fmt, ...) \
    do { \
        printf("[%6lums] " fmt "\n", get_time_ms(), ##__VA_ARGS__); \
        fflush(stdout); \
    } while(0)

#endif // LOG_H
