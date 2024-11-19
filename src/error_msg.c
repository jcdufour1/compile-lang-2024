#include "util.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include <stdarg.h>

static const char* get_log_level_str(int log_level) {
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

void log_internal(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int idx = 0; idx < indent; idx++) {
        fprintf(stderr, " ");
    }
    if (log_level >= CURR_LOG_LEVEL) {
        fprintf(stderr, "%s:%d:%s:", file, line, get_log_level_str(log_level));
        vfprintf(stderr, format, args);
    }

    va_end(args);
}

static Str_view get_curr_line(Str_view file_text, Pos pos) {
    uint32_t line = 1;
    for (; file_text.count > 0 && line < pos.line; line++) {
        while (file_text.count > 0 && str_view_consume(&file_text) != '\n');
    }

    size_t count = 0;
    {
        Str_view temp_file_text = file_text;
        while (file_text.count > 0 && str_view_front(temp_file_text) != '\n') {
            count++;
            str_view_consume(&temp_file_text);
        }
    }

    Str_view curr_line = str_view_slice(file_text, 0, count);
    return curr_line;
}

void msg_internal(
    const char* file, int line, LOG_LEVEL log_level, EXPECT_FAIL_TYPE msg_expect_fail_type,
    Str_view file_text, Pos pos, const char* format, ...
) {
    va_list args;
    va_start(args, format);
    bool fail_immediately = false;

    if (log_level >= LOG_ERROR) {
        fail_immediately = params.all_errors_fetal;
        error_count++;
    } else if (log_level == LOG_WARNING) {
        warning_count++;
    }

    if (log_level >= CURR_LOG_LEVEL) {
        fprintf(stderr, "%s:%d:%d:%s:", pos.file_path, pos.line, pos.column, get_log_level_str(log_level));
        vfprintf(stderr, format, args);

        fprintf(stderr, " %5"PRIu32" | "STR_VIEW_FMT"\n", pos.line, str_view_print(get_curr_line(file_text, pos)));
        fprintf(stderr, "       | ");
        for (uint32_t idx = 1; idx < pos.column; idx++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^");
        fprintf(stderr, "\n");

        log_internal(LOG_DEBUG, file, line, 0, "location of error\n");
    }

    if (log_level >= LOG_ERROR && params.test_expected_fail) {
        if (params.expected_fail_types.info.count <= expected_fail_count) {
            log(LOG_FETAL, "too many fails occured\n");
            exit(EXIT_CODE_FAIL);
        }
        assert(expected_fail_count < params.expected_fail_types.info.count && "out of bounds");
        EXPECT_FAIL_TYPE expected_expect_fail = vec_at(&params.expected_fail_types, expected_fail_count);
        assert(expected_expect_fail != EXPECT_FAIL_TYPE_NONE);

        if (msg_expect_fail_type != expected_expect_fail) {
            log(LOG_FETAL, "incorrect fail type occured\n");
            exit(EXIT_CODE_FAIL);
        }
        assert(expected_expect_fail != EXPECT_FAIL_TYPE_NONE);

        log(LOG_NOTE, "expected fail occured\n");
        expected_fail_count++;
    }

    if (fail_immediately) {
        fprintf(stderr, "%s: fail_immediately option active\n", get_log_level_str(LOG_FETAL));
        abort();
    }

    va_end(args);
}
