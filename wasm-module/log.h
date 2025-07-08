// log.h - Simple logging macro with timestamp
#ifndef LOG_H
#define LOG_H

#include "imp_export.h"

#define TRACE(fmt, ...) \
    do { \
        char __buffer[256]; \
        snprintf(__buffer, sizeof(__buffer), fmt, ##__VA_ARGS__); \
        _log_func(__buffer, sizeof(__buffer)); \
    } while(0)


void WASM_IMPORT(_log_func)(const char* buf, int buf_len);

#endif // LOG_H
