#ifndef LOG_INTERNAL_H
#define LOG_INTERNAL_H

extern LOG_LEVEL params_log_level;

#define LOG_GREEN "\033[1;32m"
#define LOG_BLUE "\033[1;34m"
#define LOG_YELLOW "\033[1;33m"
#define LOG_RED "\033[1;31m"
#define LOG_NORMAL "\033[0;39m"

static inline const char* get_log_level_str(int log_level) {
    switch (log_level) {
        case LOG_TRACE:
            return "trace";
        case LOG_DEBUG:
            return "debug";
        case LOG_VERBOSE:
            return "debug";
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
    }
    fprintf(stderr, "unreachable: uncovered log_level\n");
    abort();
}

static inline void log_internal(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (log_level >= CURR_LOG_LEVEL && log_level >= params_log_level) {
        for (int idx = 0; idx < indent; idx++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    va_end(args);
}

#endif // LOG_INTERNAL_H
