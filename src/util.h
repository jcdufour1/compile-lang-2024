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

#define PUSH_PRAGMA_IGNORE_WTYPE_LIMITS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \

#define POP_PRAGMA \
    _Pragma("GCC diagnostic pop") \

#define log_indent(log_level, indent, ...) \
    do { \
        if ((log_level) >= CURR_LOG_LEVEL) { \
            PUSH_PRAGMA_IGNORE_WTYPE_LIMITS; \
            for (size_t idx = 0; idx < indent; idx++) { \
                fprintf(stderr, " "); \
            } \
            POP_PRAGMA; \
            switch ((log_level)) { \
                case LOG_FETAL: \
                    /* fallthrough */ \
                case LOG_ERROR: \
                    fprintf(stderr, "%s:%d:error:", __FILE__, __LINE__); \
                    break; \
                default: \
                    fprintf(stderr, "%s:%d:", __FILE__, __LINE__); \
                    break; \
            } \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0);

#define log_file_new(log_level, file, line, ...) \
    do { \
        if ((log_level) >= CURR_LOG_LEVEL) { \
            switch ((log_level)) { \
                case LOG_FETAL: \
                    /* fallthrough */ \
                case LOG_ERROR: \
                    fprintf(stderr, "%s:%d:error:", file, line); \
                    break; \
                default: \
                    fprintf(stderr, "%s:%d:", file, line); \
                    break; \
            } \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0);

// print messages that are intended for debugging
#define log(log_level, ...) \
    log_indent(log_level, 0, __VA_ARGS__);

// print messages that are intended for the user (eg. syntax errors)
#define msg(log_level, file, line, ...) \
    log_file_new(log_level, file, line, __VA_ARGS__);

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

#endif // UTIL_H
