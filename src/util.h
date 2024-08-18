#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LOG_TRACE   0
#define LOG_DEBUG   1
#define LOG_VERBOSE 2
#define LOG_NOTE    3
#define LOG_WARNING 4
#define LOG_ERROR   5
#define LOG_FETAL   6

#define CURR_LOG_LEVEL LOG_DEBUG
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

#define log(log_level, ...) \
    log_indent(log_level, 0, __VA_ARGS__);

#define log_file(file, line, log_level, ...) \
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

#define todo() \
    do { \
        log(LOG_FETAL, "not implemented\n"); \
        abort(); \
    } while (0);

#define unreachable() \
    do { \
        log(LOG_FETAL, "unreachable\n"); \
        abort(); \
    } while (0);

static void* safe_realloc(void* old_ptr, size_t new_count_items, size_t size_each_item) {
    size_t new_capacity = new_count_items*size_each_item;
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        todo();
    }
    return new_ptr;
}

static void* safe_malloc(size_t count_items, size_t size_each_item) {
    size_t capacity = count_items*size_each_item;
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        todo();
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#define BOOL_FMT "%s"

static inline const char* bool_print(bool condition) {
    if (condition) {
        return "true";
    }
    return "false";
}

#endif // UTIL_H
