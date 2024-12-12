#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <str_view_struct.h>
#include <expected_fail_type.h>

typedef enum {
    LOG_TRACE   = 0,
    LOG_DEBUG   = 1,
    LOG_VERBOSE = 2,
    LOG_NOTE    = 3,
    LOG_WARNING = 4,
    LOG_ERROR   = 5,
    LOG_FATAL   = 6,
} LOG_LEVEL;

#ifndef CURR_LOG_LEVEL
#define CURR_LOG_LEVEL LOG_TRACE
#endif // CURR_LOG_LEVEL

typedef enum {
    EXIT_CODE_SUCCESS = 0,
    EXIT_CODE_FAIL = 1,
    EXIT_CODE_EXPECTED_FAIL = 2,
} EXIT_CODE;

struct Env_;
typedef struct Env_ Env;

typedef struct {
    const char* file_path;
    uint32_t line;
    uint32_t column;
} Pos;

#define dummy_env (&(Env){0})

static const Pos dummy_pos = {0};
static const Pos POS_BUILTIN = (Pos) {.line = UINT32_MAX};
static const Str_view dummy_file_text = {0};

// log* functions and macros print messages that are intended for debugging
static inline void log_internal(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) 
__attribute__((format (printf, 5, 6)));

#include <log_internal.h>

#define log_indent_file(...) \
    do { \
        log_internal(__VA_ARGS__); \
    } while(0) 

#define log_indent(log_level, indent, ...) \
    log_internal(log_level, __FILE__, __LINE__, indent, __VA_ARGS__)

#define log_file_new(log_level, file, line, ...) \
    log_internal(log_level, file, line, 0, __VA_ARGS__)

#define log(log_level, ...) \
    log_internal(log_level, __FILE__, __LINE__, 0, __VA_ARGS__);

// msg* functions and macros print messages that are intended for the user (eg. syntax errors)
void msg_internal(
    const char* file, int line, LOG_LEVEL log_level, EXPECT_FAIL_TYPE expected_fail_type,
    Str_view file_text, Pos pos, const char* format, ...
) __attribute__((format (printf, 7, 8)));

#define msg(log_level, expected_fail_type, file_text, pos, ...) \
    do { \
        msg_internal(__FILE__, __LINE__, log_level, expected_fail_type, file_text, pos, __VA_ARGS__); \
    } while (0)

#define todo() \
    do { \
        log(LOG_FATAL, "not implemented\n"); \
        abort(); \
    } while (0);

#define unreachable(...) \
    do { \
        log(LOG_FATAL, "unreachable:"); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        abort(); \
    } while (0)

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
            log(LOG_FATAL, "condition \""); \
            fprintf(stderr, #cond); \
            fprintf(stderr, "\" failed\n"); \
            abort(); \
        } \
    } while(0);

extern uint8_t zero_block[100000000];

extern size_t error_count;
extern size_t warning_count;
extern size_t expected_fail_count;

#endif // UTIL_H
