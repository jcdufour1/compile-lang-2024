#include <util.h>
#include <tast.h>
#include <symbol_table.h>
#include <stdarg.h>
#include <parameters.h>

static void show_location_error(Pos pos) {
    Strv* file_con_ = NULL;
    unwrap(file_path_to_text_tbl_lookup(&file_con_, pos.file_path));
    Strv file_con = *file_con_;
    unwrap(pos.line > 0);

    if (pos.line > 1) {
        uint32_t line = 1;
        for (; file_con.count > 0 && line + 1 < pos.line; line++) {
            while (file_con.count > 0 && strv_consume(&file_con) != '\n');
        }
        size_t count_prev = 0;
        {
            Strv temp_file_text = file_con;
            while (file_con.count > 0 && strv_front(temp_file_text) != '\n') {
                count_prev++;
                strv_consume(&temp_file_text);
            }
        }

        Strv prev_line = strv_slice(file_con, 0, count_prev);
        fprintf(stderr, " %5"PRIu32" | "FMT"\n", pos.line - 1, strv_print(prev_line));

        while (file_con.count > 0 && strv_consume(&file_con) != '\n');
    }

    size_t count_curr = 0;
    {
        Strv temp_file_text = file_con;
        while (file_con.count > 0 && strv_front(temp_file_text) != '\n') {
            count_curr++;
            strv_consume(&temp_file_text);
        }
    }
    Strv curr_line = strv_slice(file_con, 0, count_curr);
    fprintf(stderr, " %5"PRIu32" | "FMT"\n", pos.line, strv_print(curr_line));
    while (file_con.count > 0 && strv_consume(&file_con) != '\n');

    fprintf(stderr, "       | ");
    for (uint32_t idx = 1; idx < pos.column; idx++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");

    if (file_con.count > 0) {
        size_t count_next = 0;
        {
            Strv temp_file_text = file_con;
            while (file_con.count > 0 && strv_front(temp_file_text) != '\n') {
                count_next++;
                strv_consume(&temp_file_text);
            }
        }
        Strv next_line = strv_slice(file_con, 0, count_next);
        fprintf(stderr, " %5"PRIu32" | "FMT"\n", pos.line + 1, strv_print(next_line));
    }
}

__attribute__((format (printf, 5, 6)))
void msg_internal(
    const char* file, int line, DIAG_TYPE msg_expect_fail_type,
    Pos pos, const char* format, ...
) {
    va_list args;
    va_start(args, format);
    bool fail_immediately = false;
    LOG_LEVEL log_level = expect_fail_type_to_curr_log_level(msg_expect_fail_type);

    if (log_level >= LOG_ERROR) {
        fail_immediately = params.all_errors_fatal;
        error_count++;
    } else if (log_level == LOG_WARNING) {
        warning_count++;
    }

    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        if (pos.line < 1) {
            fprintf(stderr, "%s:", get_log_level_str(log_level));
            vfprintf(stderr, format, args);
        } else {
            fprintf(stderr, FMT":%d:%d:%s:", strv_print(pos.file_path), pos.line, pos.column, get_log_level_str(log_level));
            vfprintf(stderr, format, args);
            show_location_error(pos);
        }
        log_internal(LOG_DEBUG, file, line, 0, "location of error\n");
    }

    if (fail_immediately) {
        fprintf(stderr, "%s: fail_immediately option active\n", get_log_level_str(LOG_FATAL));
        abort();
    }

    va_end(args);
}
