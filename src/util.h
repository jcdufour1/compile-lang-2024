#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "str_view_struct.h"

#define LOG_TRACE   0
#define LOG_DEBUG   1
#define LOG_VERBOSE 2
#define LOG_NOTE    3
#define LOG_WARNING 4
#define LOG_ERROR   5
#define LOG_FETAL   6

#ifndef CURR_LOG_LEVEL
#define CURR_LOG_LEVEL LOG_TRACE
#endif // CURR_LOG_LEVEL

typedef int LOG_LEVEL;

typedef struct {
    const char* file_path;
    uint32_t line;
} Pos;

static const Pos dummy_pos = {0};

#define PUSH_PRAGMA_IGNORE_WTYPE_LIMITS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \

#define POP_PRAGMA \
    _Pragma("GCC diagnostic pop") \

static inline const char* get_log_level_str(int log_level) {
    switch (log_level) {
        case LOG_TRACE:
            return "";
        case LOG_DEBUG:
            return "debug";
        case LOG_VERBOSE:
            return "debug";
        case LOG_NOTE:
            return "note";
        case LOG_WARNING:
            return "warning";
        case LOG_ERROR:
            return "error";
        case LOG_FETAL:
            return "fetal error";
        default:
            abort();
    }
}

#define log_common(log_level, file, line, indent, ...) \
    do { \
        if ((log_level) >= CURR_LOG_LEVEL) { \
            PUSH_PRAGMA_IGNORE_WTYPE_LIMITS; \
            for (size_t idx = 0; idx < indent; idx++) { \
                fprintf(stderr, " "); \
            } \
            POP_PRAGMA; \
            fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level)); \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0);

#define log_indent(log_level, indent, ...) \
    log_common(log_level, __FILE__, __LINE__, indent, __VA_ARGS__)

#define log_file_new(log_level, file, line, ...) \
    log_common(log_level, file, line, 0, __VA_ARGS__)

// print messages that are intended for debugging
#define log(log_level, ...) \
    log_indent(log_level, 0, __VA_ARGS__);

// print messages that are intended for the user (eg. syntax errors)
#define msg(log_level, pos, ...) \
    do { \
        if ((log_level) >= LOG_ERROR) { \
            error_count++; \
        } else if ((log_level) == LOG_WARNING) { \
            warning_count++; \
        } \
        log_file_new(log_level, (pos).file_path, (pos).line, __VA_ARGS__); \
    } while (0)

#define todo() \
    do { \
        log(LOG_FETAL, "not implemented\n"); \
        abort(); \
    } while (0);

#define unreachable(...) \
    do { \
        log(LOG_FETAL, "unreachable:"); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        abort(); \
    } while (0);

#define BOOL_FMT "%s"

static inline const char* bool_print(bool condition) {
    if (condition) {
        return "true";
    }
    return "false";
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define INLINE static inline __attribute__((always_inline))

#define try(cond) \
    do { \
        if (!(cond)) { \
            log(LOG_FETAL, "condition \""); \
            fprintf(stderr, #cond); \
            fprintf(stderr, "\" failed\n"); \
            abort(); \
        } \
    } while(0);

extern uint8_t zero_block[100000000];

extern size_t error_count;
extern size_t warning_count;

extern Pos prev_pos;

#endif // UTIL_H
