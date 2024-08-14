#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

#define log(log_level, ...) \
    do { \
        if ((log_level) >= CURR_LOG_LEVEL) { \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0);

#define todo() \
    do { \
        log(LOG_FETAL, "%s:%d:error: not implemented\n", __FILE__, __LINE__); \
        abort(); \
    } while (0);

#define unreachable() \
    do { \
        log(LOG_FETAL, "%s:%d:error: unreachable\n", __FILE__, __LINE__); \
        abort(); \
    } while (0);

static void* safe_realloc(void* old_ptr, size_t new_capacity) {
    void* new_ptr = realloc(old_ptr, new_capacity);
    if (!new_ptr) {
        todo();
    }
    return new_ptr;
}

static void* safe_malloc(size_t capacity) {
    void* new_ptr = malloc(capacity);
    if (!new_ptr) {
        todo();
    }
    memset(new_ptr, 0, capacity);
    return new_ptr;
}

#endif // UTIL_H
