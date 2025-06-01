#ifndef STR_VIEW_COL_H
#define STR_VIEW_COL_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "strv.h"
#include "strv_struct.h"

typedef struct {
    Strv base;
} Strv_col;

static inline Strv_col strv_col_slice(Strv_col strv, size_t start, size_t count) {
    return (Strv_col) {.base = strv_slice(strv.base, start, count)};
}

static inline void strv_col_advance_pos(Pos* pos, char ch) {
    if (ch == '\n') {
        pos->line++;
        pos->column = 0;
    } else {
        pos->column++;
    }
}

static inline char strv_col_front(Strv_col strv) {
    return strv_front(strv.base);
}

static inline char strv_col_consume(Pos* pos, Strv_col* strv) {
    strv_col_advance_pos(pos, strv_col_front(*strv));
    return strv_consume(&strv->base);
}

static inline Strv_col strv_col_consume_count(Pos* pos, Strv_col* strv, size_t count) {
    Strv temp = strv->base;
    for (size_t idx = 0; idx < count; idx++) {
        strv_col_consume(pos, strv);
    }
    return (Strv_col) {.base = strv_consume_count(&temp, count)};
}

static inline bool strv_col_try_consume_while(
    Strv_col* result,
    Pos* pos,
    Strv_col* strv,
    bool (*should_continue)(char /* previous char */, char /* current char */)
) {
    Strv* base = &strv->base;
    for (size_t idx = 0; base->count > idx; idx++) {
        char prev_char = idx > 0 ? (strv_at(*base, idx)) : (0);
        if (!should_continue(prev_char, strv_at(*base, idx))) {
            result->base.str = base->str;
            result->base.count = idx;
            base->str += idx;
            base->count -= idx;
            return true;
        }
        strv_col_advance_pos(pos, strv_at(*base, idx));
    }
    return false;
}

static inline Strv_col strv_col_consume_while(
    Pos* pos,
    Strv_col* strv,
    bool (*should_continue)(char /* previous char */, char /* current char */)
) {
    Strv_col result = {0};
    if (strv_col_try_consume_while(&result, pos, strv, should_continue)) {
        return result;
    }
    unreachable("condition is never met");
}

static inline Strv_col strv_col_consume_until(Pos* pos, Strv_col* strv, char delim) {
    Strv_col new_strv;
    Strv* base = &strv->base;
    for (size_t idx = 0; base->count > idx; idx++) {
        if (strv_at(*base, idx) == delim) {
            new_strv.base.str = base->str;
            new_strv.base.count = idx;
            base->str += idx;
            base->count -= idx;
            return new_strv;
        }
        strv_col_advance_pos(pos, strv_at(*base, idx));
    }
    unreachable("delim not found");
}

// return true when match
static inline bool strv_col_cstr_is_equal(Strv_col strv, const char* cstr) {
    return strv_cstr_is_equal(strv.base, cstr);
}

static inline int strv_col_cmp(Strv_col a, Strv_col b) {
    return strv_cmp(a.base, b.base);
}

static inline bool strv_col_is_equal(Strv_col a, Strv_col b) {
    return strv_is_equal(a.base, b.base);
}

static inline bool strv_col_try_consume(Pos* pos, Strv_col* strv, char ch) {
    if (strv_try_consume(&strv->base, ch)) {
        strv_col_advance_pos(pos, ch);
        return true;
    }
    return false;
}

#define strv_col_print(strv) strv_print((strv).base)

#endif // STR_VIEW_COL_H
