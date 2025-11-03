#include <util.h>
#include <tast.h>
#include <symbol_table.h>
#include <stdarg.h>
#include <parameters.h>
#include <pos_util.h>
#include <file.h>

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

static Defered_msg defered_msg_new(
    const char* file,
    int line,
    Pos pos_actual_msg,
    Pos pos_for_sort,
    Strv actual_msg,
    size_t pos_in_defered_msgs
) {
    return (Defered_msg) {
        .file = file,
        .line = line,
        .pos_actual_msg = pos_actual_msg,
        .pos_for_sort = pos_for_sort,
        .actual_msg = actual_msg,
        .pos_in_defered_msgs = pos_in_defered_msgs
    };
}

#define QSORT_LESS_THAN (-1)
#define QSORT_EQUAL 0
#define QSORT_MORE_THAN 1

static int defered_msg_compare(const void* lhs_, const void* rhs_) {
    const Defered_msg* lhs = lhs_;
    const Defered_msg* rhs = rhs_;

    if (lhs->pos_for_sort.file_path.count < rhs->pos_for_sort.file_path.count) {
        return QSORT_LESS_THAN;
    } else if (lhs->pos_for_sort.file_path.count > rhs->pos_for_sort.file_path.count) {
        return QSORT_MORE_THAN;
    }

    // TODO: remove if condition (always run if body) when possible
    if (rhs->pos_for_sort.file_path.count != SIZE_MAX) {
        int file_result = strncmp(lhs->pos_for_sort.file_path.str, rhs->pos_for_sort.file_path.str, rhs->pos_for_sort.file_path.count);
        if (file_result < 0) {
            return QSORT_LESS_THAN;
        } else if (file_result > 0) {
            return QSORT_MORE_THAN;
        }
    }

    if (lhs->pos_for_sort.line < rhs->pos_for_sort.line) {
        return QSORT_LESS_THAN;
    } else if (lhs->pos_for_sort.line > rhs->pos_for_sort.line) {
        return QSORT_MORE_THAN;
    }

    if (lhs->pos_for_sort.column < rhs->pos_for_sort.column) {
        return QSORT_LESS_THAN;
    } else if (lhs->pos_for_sort.column > rhs->pos_for_sort.column) {
        return QSORT_MORE_THAN;
    }

    if (lhs->pos_in_defered_msgs < rhs->pos_in_defered_msgs) {
        return QSORT_LESS_THAN;
    } else if (lhs->pos_in_defered_msgs > rhs->pos_in_defered_msgs) {
        return QSORT_MORE_THAN;
    }

    unreachable("lhs->pos_in_defered_msgs should not equal rhs->pos_in_defered_msgs");
}

void msg_internal_actual_print(const char* file, int line, Pos pos, Strv actual_msg) {
    fprintf(stderr, FMT, strv_print(actual_msg));
    if (pos.line > 0) {
        show_location_error(pos);
    }

    Pos* exp_from = pos.expanded_from;
    while (exp_from) {
        assert(!pos_is_equal(*exp_from, POS_BUILTIN));
        assert(!pos_is_equal(*exp_from, (Pos) {0}));
        fprintf(
            stderr,
            FMT":%d:%d:%s:",
            strv_print(exp_from->file_path),
            exp_from->line,
            exp_from->column,
            get_log_level_str(LOG_NOTE)
        );
        fprintf(stderr, "in expansion of def\n");
        if (exp_from->line > 0) {
            show_location_error(*exp_from);
        }

        exp_from = exp_from->expanded_from;
    }

    log_internal(LOG_DEBUG, file, line, 0, "location of error\n");
}

void print_all_defered_msgs(void) {
    qsort(env.defered_msgs.buf, env.defered_msgs.info.count, sizeof(env.defered_msgs.buf[0]), defered_msg_compare);

    vec_foreach(idx, Defered_msg, curr, env.defered_msgs) {
        msg_internal_actual_print(curr.file, curr.line, curr.pos_actual_msg, curr.actual_msg);
    }

    vec_reset(&env.defered_msgs);
}

__attribute__((format (printf, 5, 6)))
void msg_internal(
    const char* file, int line, DIAG_TYPE msg_expect_fail_type,
    Pos pos, const char* format, ...
) {
    LOG_LEVEL log_level = expect_fail_type_to_curr_log_level(msg_expect_fail_type);

    if (log_level >= LOG_ERROR && env.error_count >= params.max_errors) {
        print_all_defered_msgs();
        fprintf(stderr, "%s:", get_log_level_str(LOG_FATAL));
        fprintf(stderr, "too many error messages have been printed; exiting now\n");
        local_exit(EXIT_CODE_FAIL);
    }

    va_list args;
    va_start(args, format);
    bool fail_immediately = false;

    Pos pos_for_sort = pos;
    if (log_level == LOG_NOTE && env.defered_msgs.info.count > 0) {
        pos_for_sort = vec_top(env.defered_msgs).pos_for_sort;
    }

    if (log_level >= LOG_ERROR) {
        fail_immediately = params.all_errors_fatal;
        env.error_count++;
    } else if (log_level == LOG_WARNING) {
        env.warning_count++;
    }

    va_list args_copy;
    va_copy(args_copy, args);

    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) {
        size_t buf_cap_needed = 0;

        if (1) {
            if (pos.line < 1) {
                buf_cap_needed = (size_t)snprintf(NULL, 0, "%s:", get_log_level_str(log_level));
                size_t second_count = vsnprintf(NULL, 0, format, args_copy);
                buf_cap_needed += max(buf_cap_needed, second_count);
            } else {
                buf_cap_needed = max(buf_cap_needed, (size_t)snprintf(
                    NULL,
                    0,
                    FMT":%d:%d:%s:",
                    strv_print(pos.file_path),
                    pos.line,
                    pos.column,
                    get_log_level_str(log_level)
                ));
                buf_cap_needed = max(buf_cap_needed, (size_t)vsnprintf(NULL, 0, format, args_copy));
            }
        }
        buf_cap_needed += 1;

        static String temp_buf = {0};
        temp_buf.info.count = 0;
        vec_reserve(&a_leak, &temp_buf, buf_cap_needed);
        temp_buf.info.count = buf_cap_needed;

        String actual_buf = {0};
        if (pos.line < 1) {
            snprintf(temp_buf.buf, temp_buf.info.count, "%s:", get_log_level_str(log_level));
            string_extend_cstr(&a_leak, &actual_buf, temp_buf.buf);

            vsnprintf(temp_buf.buf, temp_buf.info.count, format, args);
            string_extend_cstr(&a_leak, &actual_buf, temp_buf.buf);
        } else {
            snprintf(temp_buf.buf, temp_buf.info.count, FMT":%d:%d:%s:", strv_print(pos.file_path), pos.line, pos.column, get_log_level_str(log_level));
            string_extend_cstr(&a_leak, &actual_buf, temp_buf.buf);
            
            vsnprintf(temp_buf.buf, temp_buf.info.count, format, args);
            string_extend_cstr(&a_leak, &actual_buf, temp_buf.buf);
        }
        if (params.print_immediately) {
            msg_internal_actual_print(file, line, pos, string_to_strv(actual_buf));
        } else {
            size_t pos_in_defered_msgs = env.defered_msgs.info.count;
            vec_append(&a_leak, &env.defered_msgs, defered_msg_new(file, line, pos, pos_for_sort, string_to_strv(actual_buf), pos_in_defered_msgs));
        }
    }

    if (fail_immediately) {
        fprintf(stderr, "%s: fail_immediately option active\n", get_log_level_str(LOG_FATAL));
        abort();
    }

    va_end(args);
    va_end(args_copy);
}
