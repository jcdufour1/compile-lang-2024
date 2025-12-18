#ifndef LOG_INTERNAL_H
#define LOG_INTERNAL_H

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
            abort();
    }
    fprintf(stderr, "unreachable: uncovered log_level\n");
    abort();
}

static inline void log_internal_ex(
    FILE* dest,
    LOG_LEVEL log_level,
    bool print_location,
    const char* file,
    int line,
    Indent indent,
    const char* format, ...
) {
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
            abort();
        }
    }

    va_end(args);
}

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

#endif // LOG_INTERNAL_H
