#ifndef LOG_INTERNAL_H
#define LOG_INTERNAL_H

static inline const char* get_log_level_str(int log_level) {
    switch (log_level) {
        case LOG_TRACE:
            return "trace";
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
        case LOG_FATAL:
            return "fatal error";
        default:
            abort();
    }
}

static inline void log_internal(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (log_level >= CURR_LOG_LEVEL) {
        for (int idx = 0; idx < indent; idx++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    va_end(args);
}

#endif // LOG_INTERNAL_H
