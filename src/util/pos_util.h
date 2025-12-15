#ifndef POS_UTIL_H
#define POS_UTIL_H

#include <local_string.h>
#include <util.h>
#include <strv.h>

static inline void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&a_temp, buf, "((");
    string_extend_cstr(&a_temp, buf, ";file_path:");
    string_extend_strv(&a_temp, buf, pos.file_path);
    string_extend_cstr(&a_temp, buf, ";line:");
    string_extend_int64_t(&a_temp, buf, pos.line);
    string_extend_cstr(&a_temp, buf, ";column:");
    string_extend_int64_t(&a_temp, buf, pos.column);
    string_extend_cstr(&a_temp, buf, " ))");
}

static inline Strv pos_print_internal(Pos pos) {
    String buf = {0};
    extend_pos(&buf, pos);
    return string_to_strv(buf);
}

static inline Strv pos_deep_print_internal(Pos* pos, int indent) {
    if (!pos) {
        return (Strv) {0};
    }

    String buf = {0};
    extend_pos(&buf, *pos);
    string_extend_strv(&a_temp, &buf, pos_deep_print_internal(pos->expanded_from, indent + INDENT_WIDTH));
    return string_to_strv(buf);
}

#define pos_print(pos) \
    strv_print(pos_print_internal(pos))

#define pos_deep_print(pos) \
    strv_print(pos_deep_print_internal(pos, 0))

static inline void pos_expanded_from_append(Pos* pos, Pos* new_exp_from) {
    Pos* curr = pos;
    while (curr->expanded_from) {
        curr = curr->expanded_from;
    }
    assert(!curr->expanded_from);
    curr->expanded_from = new_exp_from;
}

static inline size_t pos_expanded_from_count(Pos* pos) {
    size_t count = 0;
    Pos* curr = pos;
    while (curr->expanded_from) {
        count++;
        curr = curr->expanded_from;
    }
    return count;
}

static inline bool pos_is_equal(Pos a, Pos b) {
    return strv_is_equal(a.file_path, b.file_path) && a.line == b.line && a.column == b.column;
}

static inline bool pos_is_recursion(Pos pos) {
    Pos* exp_from = pos.expanded_from;
    while (exp_from) {
        if (exp_from && pos_is_equal(pos, *exp_from)) {
            return true;
        }

        exp_from = exp_from->expanded_from;
    }
    return false;
}

#endif // POS_UTIL_H
