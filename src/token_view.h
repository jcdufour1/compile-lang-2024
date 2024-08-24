#ifndef TOKEN_VIEW_H
#define TOKEN_VIEW_H

#include <stddef.h>
#include "token.h"
#include "assert.h"

typedef struct {
    const Token* tokens;
    size_t count;
} Tk_view;

static inline Token tk_view_at(Tk_view token_view, size_t idx) {
    assert(token_view.count > idx);
    return token_view.tokens[idx];
}

static inline Token tk_view_front(Tk_view token_view) {
    return tk_view_at(token_view, 0);
}

static inline Tk_view tk_view_chop_on_cond(
    Tk_view* tk_view,
    bool (*should_continue)(const Token* /* previous token */, const Token* /* current token */)
) {
    Tk_view new_tk_view;
    for (size_t idx = 0; tk_view->count > idx; idx++) {
        const Token* prev_token = idx > 0 ? (&tk_view->tokens[idx]) : (NULL);
        if (!should_continue(prev_token, &tk_view->tokens[idx])) {
            new_tk_view.tokens = tk_view->tokens;
            new_tk_view.count = idx;
            tk_view->tokens += idx;
            tk_view->count -= idx;
            return new_tk_view;
        }
    }
    todo();
}

// delim itself is left in the input token_view
static inline Tk_view tk_view_chop_on_type_delim(Tk_view* token_view, TOKEN_TYPE delim) {
    Tk_view new_token_view;
    for (size_t idx = 0; token_view->count > idx; idx++) {
        if (token_view->tokens[idx].type == delim) {
            new_token_view.tokens = token_view->tokens;
            new_token_view.count = idx;
            token_view->tokens += idx;
            token_view->count -= idx;
            return new_token_view;
        }
    }

    // delim not found
    todo();
}

static inline Tk_view tk_view_chop_count(Tk_view* token_view, size_t count) {
    if (token_view->count < count) {
        unreachable();
    }
    Tk_view new = {token_view->tokens, count};
    token_view->tokens += count;
    token_view->count -= count;
    return new;
}

static inline Tk_view tk_view_chop_count_at_most(Tk_view* token_view, size_t count) {
    return tk_view_chop_count(token_view, MIN(token_view->count, count));
}

static inline Token tk_view_chop_front(Tk_view* token_view) {
    return tk_view_front(tk_view_chop_count(token_view, 1));
}

static inline Str_view tk_view_print_internal(Tk_view token_view) {
    static String buf = {0};
    string_set_to_zero_len(&buf);

    for (size_t idx = 0; idx < token_view.count; idx++) {
        string_extend_strv(&buf, token_print_internal(token_view.tokens[idx]));
        string_extend_cstr(&buf, ";    ");
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

#define TK_VIEW_FMT STR_VIEW_FMT

#define tk_view_print(token_view__dfjasdjf) str_view_print(tk_view_print_internal(token_view__dfjasdjf))

#endif // TOKEN_VIEW_H
