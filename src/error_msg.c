#include "util.h"
#include "tast.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include <stdarg.h>
#include <parameters.h>

static void show_location_error(Pos pos) {
    Str_view* file_con_ = NULL;
    unwrap(file_path_to_text_tbl_lookup(&file_con_, pos.file_path));
    Str_view file_con = *file_con_;
    assert(pos.line > 0);

    if (pos.line > 1) {
        uint32_t line = 1;
        for (; file_con.count > 0 && line + 1 < pos.line; line++) {
            while (file_con.count > 0 && str_view_consume(&file_con) != '\n');
        }
        size_t count_prev = 0;
        {
            Str_view temp_file_text = file_con;
            while (file_con.count > 0 && str_view_front(temp_file_text) != '\n') {
                count_prev++;
                str_view_consume(&temp_file_text);
            }
        }

        Str_view prev_line = str_view_slice(file_con, 0, count_prev);
        fprintf(stderr, " %5"PRIu32" | "STR_VIEW_FMT"\n", pos.line - 1, str_view_print(prev_line));

        while (file_con.count > 0 && str_view_consume(&file_con) != '\n');
    }

    size_t count_curr = 0;
    {
        Str_view temp_file_text = file_con;
        while (file_con.count > 0 && str_view_front(temp_file_text) != '\n') {
            count_curr++;
            str_view_consume(&temp_file_text);
        }
    }
    Str_view curr_line = str_view_slice(file_con, 0, count_curr);
    fprintf(stderr, " %5"PRIu32" | "STR_VIEW_FMT"\n", pos.line, str_view_print(curr_line));
    while (file_con.count > 0 && str_view_consume(&file_con) != '\n');

    fprintf(stderr, "       | ");
    for (uint32_t idx = 1; idx < pos.column; idx++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");

    if (file_con.count > 0) {
        size_t count_next = 0;
        {
            Str_view temp_file_text = file_con;
            while (file_con.count > 0 && str_view_front(temp_file_text) != '\n') {
                count_next++;
                str_view_consume(&temp_file_text);
            }
        }
        Str_view next_line = str_view_slice(file_con, 0, count_next);
        fprintf(stderr, " %5"PRIu32" | "STR_VIEW_FMT"\n", pos.line + 1, str_view_print(next_line));
    }
}

__attribute__((format (printf, 6, 7)))
void msg_internal(
    const char* file, int line, LOG_LEVEL log_level, EXPECT_FAIL_TYPE msg_expect_fail_type,
    Pos pos, const char* format, ...
) {
    va_list args;
    va_start(args, format);
    bool fail_immediately = false;

    if (log_level >= LOG_ERROR) {
        fail_immediately = params.all_errors_fatal;
        error_count++;
    } else if (log_level == LOG_WARNING) {
        warning_count++;
    }

    // TODO: should log_internal be called here to reduce duplication
    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        if (pos.line < 1) {
            fprintf(stderr, "%s:", get_log_level_str(log_level));
            vfprintf(stderr, format, args);
        } else {
            fprintf(stderr, STR_VIEW_FMT":%d:%d:%s:", str_view_print(pos.file_path), pos.line, pos.column, get_log_level_str(log_level));
            vfprintf(stderr, format, args);
            show_location_error(pos);
        }
        log_internal(LOG_DEBUG, file, line, 0, "location of error\n");
    }

    if (log_level >= LOG_ERROR && params.test_expected_fail) {
        if (params.expected_fail_types.info.count <= expected_fail_count) {
            log(LOG_FATAL, "too many fails occured\n");
            exit(EXIT_CODE_FAIL);
        }
        unwrap(expected_fail_count < params.expected_fail_types.info.count && "out of bounds");
        EXPECT_FAIL_TYPE expected_expect_fail = vec_at(&params.expected_fail_types, expected_fail_count);
        assert(expected_expect_fail != EXPECT_FAIL_NONE);

        if (msg_expect_fail_type != expected_expect_fail) {
            log(
                LOG_FATAL,
                "fail type `"STR_VIEW_FMT"` occured, but `"STR_VIEW_FMT"` was expected\n",
                expect_fail_type_print(msg_expect_fail_type),
                expect_fail_type_print(expected_expect_fail)
            );
            exit(EXIT_CODE_FAIL);
        }
        assert(expected_expect_fail != EXPECT_FAIL_NONE);

        log(LOG_NOTE, "expected fail occured\n");
        expected_fail_count++;
    }

    if (fail_immediately) {
        fprintf(stderr, "%s: fail_immediately option active\n", get_log_level_str(LOG_FATAL));
        abort();
    }

    va_end(args);
}
