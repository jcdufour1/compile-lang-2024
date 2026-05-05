#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <strv_struct.h>
#include <diag_type.h>
#include <assert.h>

typedef enum {
    LOG_NEVER = 0,
    LOG_TRACE,
    LOG_DEBUG,
    LOG_VERBOSE,
    LOG_INFO,
    LOG_NOTE,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,

    // for static asserts
    LOG_COUNT,
} LOG_LEVEL;

// TODO: make this more cross platform
#define breakpoint() \
    do { \
        log(LOG_DEBUG, "breakpoint trap\n"); \
        __asm__("int3"); \
    } while(0)

#define QSORT_LESS_THAN (-1)
#define QSORT_EQUAL 0
#define QSORT_MORE_THAN 1

#ifndef MIN_LOG_LEVEL
#define MIN_LOG_LEVEL LOG_TRACE
#endif // CURR_LOG_LEVEL

typedef enum {
    EXIT_CODE_SUCCESS = 0,
    EXIT_CODE_FAIL,
} EXIT_CODE;

typedef uint32_t Indent;

struct Env_;
typedef struct Env_ Env;

struct Uast_def_;
typedef struct Uast_def_ Uast_def;

// TODO: try to eventually store only uint64_t directly in Pos to improve memory usage?
typedef struct Pos_ {
    Strv file_path;
    uint32_t line;
    uint32_t column;

    // NULL means that that is nothing that this expression was expanded from
    struct Pos_* expanded_from;
} Pos;

#define POS_BUILTIN ((Pos) {.file_path = MOD_PATH_BUILTIN})

// log* functions and macros print messages that are intended for debugging
static inline void log_internal(LOG_LEVEL log_level, const char* file, int line, Indent indent, const char* format, ...) 
__attribute__((format (printf, 5, 6)));

#define log_indent_file(...) \
    do { \
        log_internal(__VA_ARGS__); \
    } while(0) 

#define log_indent(log_level, indent, ...) \
    log_internal(log_level, __FILE__, __LINE__, indent, __VA_ARGS__)

#define log_file_new(log_level, file, line, ...) \
    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) { \
        log_internal(log_level, file, line, 0, __VA_ARGS__); \
    }

#define log(log_level, ...) \
    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) { \
        log_internal(log_level, __FILE__, __LINE__, 0, __VA_ARGS__); \
    }

#define todo() \
    do { \
        log(LOG_FATAL, "not implemented\n"); \
        local_abort(); \
    } while (0);

#define unreachable(...) \
    do { \
        log(LOG_FATAL, "unreachable:"); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        local_abort(); \
    } while (0)

#define fallthrough \
    do { \
    } while(0); \
    __attribute__((fallthrough));

// suppress warnings for variable declaration after label (and allow pre-c23 compilers)
#define do_nothing() \
    do { \
    } while(0)

// TODO
//static inline Strv bool_print(bool condition) {
//    if (condition) {
//        return sv("true");
//    }
//    return sv("false");
//}

#define array_count(array) (sizeof(array)/sizeof((array)[0]))

#define array_at(array, index) \
    (unwrap((index) < array_count(array) && "out of bounds"), (array)[index])

#define array_at_ref(array, index) \
    (unwrap((index) < array_count(array) && "out of bounds"), &(array)[index])

#define INLINE static inline __attribute__((always_inline))

#define NEVER_RETURN __attribute__((noreturn))

#define WSWITCH_ENUM_IGNORE_START \
    _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")

#define WIMPLICIT_FALLTHROUGH_IGNORE_START \
    _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"")

#define WSIGN_CONVERSION_IGNORE_START \
    _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"")

#ifdef OWN_WERROR
#   define WSWITCH_ENUM_IGNORE_END \
        _Pragma("GCC diagnostic error \"-Wswitch-enum\"")
#   define WIMPLICIT_FALLTHROUGH_IGNORE_END \
        _Pragma("GCC diagnostic error \"-Wimplicit-fallthrough\"")
#   define WSIGN_CONVERSION_IGNORE_END \
        _Pragma("GCC diagnostic error \"-Wsign-conversion\"")
#else
#   define WSWITCH_ENUM_IGNORE_END \
        _Pragma("GCC diagnostic warning \"-Wswitch-enum\"")
#   define WIMPLICIT_FALLTHROUGH_IGNORE_END \
        _Pragma("GCC diagnostic warning \"-Wimplicit-fallthrough\"")
#   define WSIGN_CONVERSION_IGNORE_END \
        _Pragma("GCC diagnostic warning \"-Wsign-conversion\"")
#endif // OWN_WERROR

#define local_abort() \
    do { \
        log_internal(LOG_FATAL, __FILE__, __LINE__, 0, "aborting\n"); \
        abort(); \
    } while (0)

static inline void unwrap_internal(bool cond, const char* cond_text, const char* file, int line) {
    if (!(cond)) {
        log_internal(LOG_FATAL, file, line, 0, "condition \"%s\" failed\n", cond_text);
        local_abort();
    }
}

#define unwrap(cond) unwrap_internal(cond, #cond, __FILE__, __LINE__)

extern Env env;

// TODO: move this?
typedef size_t Scope_id;

#ifndef INDENT_WIDTH
#define INDENT_WIDTH 2
#endif // INDENT_WIDTH

// TODO: consider if this is the ideal place
#define SCOPE_TOP_LEVEL 0
#define SCOPE_NOT SIZE_MAX

#ifdef _WIN32
#   define PATH_SEP_CHAR '\\'
#   define PATH_SEP "\\"
#else
#   define PATH_SEP_CHAR '/'
#   define PATH_SEP "/"
#endif // _WIN32

#define MOD_PATH_STD (sv("std"PATH_SEP))
// MOD_PATH_BUILTIN is used mostly for symbols generated by the compiler itself
#define MOD_PATH_BUILTIN (sv("builtin"))
// MOD_PATH_RUNTIME is used for standard library functions that are used by the compiler itself
#define MOD_PATH_RUNTIME (sv("std"PATH_SEP"runtime"))
#define MOD_PATH_PRELUDE (sv("std"PATH_SEP"prelude"))
#define MOD_PATH_OF_MOD_PATHS (sv("std"PATH_SEP"does_not_exist"PATH_SEP"mod_paths"))
#define MOD_PATH_ARRAYS (sv("std"PATH_SEP"does_not_exist"PATH_SEP"arrays"))
#define MOD_PATH_LOAD_SCOPES (sv("std"PATH_SEP"does_not_exist"PATH_SEP"load_scopes"))
#define MOD_PATH_AUX_ALIASES (sv("std"PATH_SEP"does_not_exist"PATH_SEP"aux_aliases"))

#define MOD_PATH_EXTERN_C ((Strv) {0})

#define MOD_ALIAS_BUILTIN name_new(MOD_PATH_BUILTIN, sv("mod_aliases"), (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL)
#define MOD_ALIAS_TOP_LEVEL name_new(MOD_PATH_BUILTIN, sv("mod_aliases_top_level"), (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL)
#define MOD_ALIAS_PRELUDE name_new(MOD_PATH_BUILTIN, sv("mod_aliases_prelude"), (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL)

#define EXTERN_C_OWN_PREFIX "own"

// TODO: move this?
#define BUILTIN_DEFS_COUNT 4

#define FMT "%.*s"

// TODO: give better name
#define thing(expr) #expr

#ifdef _WIN32
#   define SIZE_T_FMT "%I64u"
#else
#   define SIZE_T_FMT "%zu"
#endif // _WIN32

#define MOD_PATH_COMMAND_LINE sv("std"PATH_SEP"does_not_exist"PATH_SEP"cmd")

#define DEFAULT_BUILD_DIR "own_build"

extern LOG_LEVEL params_log_level;

#define LOG_GREEN "\033[1;32m"
#define LOG_BLUE "\033[1;34m"
#define LOG_YELLOW "\033[1;33m"
#define LOG_RED "\033[1;31m"
#define LOG_NORMAL "\033[0;39m"

static inline const char* get_log_level_str(LOG_LEVEL log_level) {
    switch (log_level) {
        case LOG_NEVER:
            return ""; // TODO
        case LOG_TRACE:
            return "trace";
        case LOG_DEBUG:
            return "debug";
        case LOG_VERBOSE:
            return "verbose";
        case LOG_INFO:
            return LOG_GREEN"info"LOG_NORMAL;
        case LOG_NOTE:
            return LOG_BLUE"note"LOG_NORMAL;
        case LOG_WARNING:
            return LOG_YELLOW"warning"LOG_NORMAL;
        case LOG_ERROR:
            return LOG_RED"error"LOG_NORMAL;
        case LOG_FATAL:
            return LOG_RED"fatal error"LOG_NORMAL;
        case LOG_COUNT:
            fprintf(stderr, "unreachable: uncovered log_level\n");
            local_abort();
    }
    fprintf(stderr, "unreachable: uncovered log_level\n");
    local_abort();
}

__attribute__((format (printf, 7, 8)))
static inline void log_internal_ex(
    FILE* dest,
    LOG_LEVEL log_level,
    bool print_location,
    const char* file,
    int line,
    Indent indent,
    const char* format, ...
) {
    // TODO: figure out why log_internal_ex does not seem to work for dest other than stderr
    va_list args;
    va_start(args, format);

    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        for (Indent idx = 0; idx < indent; idx++) {
            fprintf(dest, " ");
        }
        if (print_location) {
            fprintf(dest, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        }
        if (vfprintf(dest, format, args) < 0) {
            // TODO: consider if abort should be done here
            fprintf(stderr, "unreachable: vfprintf failed. destination file stream may have been opened for reading instead of writing\n");
            local_abort();
        }
    }

    va_end(args);
}

__attribute__((format (printf, 5, 6)))
static inline void log_internal(LOG_LEVEL log_level, const char* file, int line, Indent indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        for (Indent idx = 0; idx < indent; idx++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    va_end(args);
}

// log_internal_custom_file
__attribute__((format (printf, 6, 7)))
static inline void log_internal_custom_dest(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Indent indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        for (Indent idx = 0; idx < indent; idx++) {
            fprintf(dest, " ");
        }
        fprintf(dest, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(dest, format, args);
    }

    va_end(args);
}



typedef struct {
    uint64_t value;
} Bytes;

static inline Bytes bytes_new(uint64_t value) {
    return (Bytes) {.value = value};
}

static inline Bytes bytes_add(Bytes lhs, Bytes rhs) {
    return bytes_new(lhs.value + rhs.value);
}

static inline Bytes bytes_subtract(Bytes lhs, Bytes rhs) {
    return bytes_new(lhs.value - rhs.value);
}

static inline Bytes bytes_multiply(Bytes lhs, Bytes rhs) {
    return bytes_new(lhs.value*rhs.value);
}

static inline Bytes bytes_modulo(Bytes lhs, Bytes rhs) {
    return bytes_new(lhs.value%rhs.value);
}

static inline Bytes bytes_max(Bytes lhs, Bytes rhs) {
    return lhs.value > rhs.value ? bytes_new(lhs.value) : bytes_new(rhs.value);
}

static inline bool bytes_is_greater_than(Bytes lhs, Bytes rhs) {
    return lhs.value > rhs.value;
}

typedef struct {
    uint64_t value;
} Bits;

#define bits_new_macro(num) ((Bits) {.value = num})
static inline Bits bits_new(uint64_t value) {
    return bits_new_macro(value);
}

static inline bool bits_is_equal(Bits a, Bits b) {
    return a.value == b.value;
}

static inline bool bits_is_less_than(Bits lhs, Bits rhs) {
    return lhs.value < rhs.value;
}

static inline bool bits_is_greater(Bits lhs, Bits rhs) {
    return lhs.value > rhs.value;
}

static inline bool bits_is_less_or_equal(Bits lhs, Bits rhs) {
    return lhs.value <= rhs.value;
}

static inline void bits_decrement(Bits* a) {
    a->value--;
}

#endif // UTIL_H
