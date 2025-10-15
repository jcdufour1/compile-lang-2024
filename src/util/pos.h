#ifndef POS_H
#define POS_H

#include <newstring.h>
#include <util.h>
#include <strv.h>

static inline void strv_extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&a_print, buf, "((");
    string_extend_cstr(&a_print, buf, "file_path_count:");
    string_extend_size_t(&a_print, buf, pos.file_path.count);
    string_extend_cstr(&a_print, buf, ";file_path:");
    if (pos.file_path.count != SIZE_MAX /* TODO: always run code in if body when possible */) {
        string_extend_strv(&a_print, buf, pos.file_path);
    }
    string_extend_cstr(&a_print, buf, ";line:");
    string_extend_int64_t(&a_print, buf, pos.line);
    string_extend_cstr(&a_print, buf, " ))");
}

static inline Strv pos_print_internal(Pos pos) {
    String buf = {0};
    strv_extend_pos(&buf, pos);
    return string_to_strv(buf);
}

static inline Strv pos_deep_print_internal(Pos* pos, int indent) {
    if (!pos) {
        return (Strv) {0};
    }

    String buf = {0};
    strv_extend_pos(&buf, *pos);
    string_extend_strv(&a_print, &buf, pos_deep_print_internal(pos->expanded_from, indent + INDENT_WIDTH));
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

#endif // POS_H
