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

void log_common(LOG_LEVEL log_level, const char* file, int line, int indent, const char* format, ...) {
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

void msg(LOG_LEVEL log_level, EXPECT_FAIL_TYPE expected_fail_type, Pos pos, const char* format, ...) {
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
    }

    log(LOG_DEBUG, "YES %d\n", expected_fail_type);
    if (params.expected_fail_types.info.count > expected_fail_count) {
        EXPECT_FAIL_TYPE expect_fail = vec_at(&params.expected_fail_types, expected_fail_count);
        assert(expect_fail != EXPECT_FAIL_TYPE_NONE);
        if (params.test_expected_fail && expected_fail_type == expect_fail) {
            assert(expected_fail_count == 0);
            assert(expect_fail != EXPECT_FAIL_TYPE_NONE);

            log(LOG_NOTE, "expected fail occured\n");
            expected_fail_count++;
        }
    }

    if (fail_immediately) {
        fprintf(stderr, "%s: fail_immediately option active\n", get_log_level_str(LOG_FETAL));
        abort();
    }

    va_end(args);
}
